// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeLanDiscoverySubsystem.h"

#include "Containers/StringConv.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "IPAddress.h"
#include "MyBondageLife.h"
#include "SocketSubsystem.h"
#include "Sockets.h"

namespace
{
	/** Packet tag, so unrelated UDP traffic on the port is ignored. */
	const TCHAR* RoomPacketTag = TEXT("MBL1");

	/** Longest datagram we are willing to read. */
	constexpr int32 ReceiveBufferSize = 1024;

	/** Strips characters that would break the pipe delimited packet. */
	FString SanitizeField(const FString& In)
	{
		FString Result = In;
		Result.ReplaceInline(TEXT("|"), TEXT("/"));
		Result.ReplaceInline(TEXT("\r"), TEXT(" "));
		Result.ReplaceInline(TEXT("\n"), TEXT(" "));
		return Result;
	}
}

void UMyBondageLifeLanDiscoverySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	EnsureSockets();

	TickHandle = FTSTicker::GetCoreTicker().AddTicker(TEXT("MyBondageLifeLanDiscovery"), 0.0f,
		[this](float DeltaSeconds) { return HandleTick(DeltaSeconds); });
}

void UMyBondageLifeLanDiscoverySubsystem::Deinitialize()
{
	if (TickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		TickHandle.Reset();
	}

	DestroySockets();

	Super::Deinitialize();
}

void UMyBondageLifeLanDiscoverySubsystem::EnsureSockets()
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		return;
	}

	if (!ListenSocket)
	{
		ListenSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("MyBondageLifeLanListen"), true);
		if (ListenSocket)
		{
			ListenSocket->SetNonBlocking(true);
			// Let several instances on one machine bind the same port while testing.
			ListenSocket->SetReuseAddr(true);
			ListenSocket->SetBroadcast(true);

			TSharedRef<FInternetAddr> ListenAddress = SocketSubsystem->CreateInternetAddr();
			ListenAddress->SetAnyAddress();
			ListenAddress->SetPort(DiscoveryPort);

			if (!ListenSocket->Bind(*ListenAddress))
			{
				UE_LOG(LogMyBondageLife, Warning,
					TEXT("LAN discovery: failed to bind UDP port %d. Room discovery will not work."),
					DiscoveryPort);
				SocketSubsystem->DestroySocket(ListenSocket);
				ListenSocket = nullptr;
			}
		}
	}

	if (!SendSocket)
	{
		SendSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("MyBondageLifeLanSend"), true);
		if (SendSocket)
		{
			SendSocket->SetNonBlocking(true);
			SendSocket->SetBroadcast(true);
			SendSocket->SetReuseAddr(true);
		}
	}
}

void UMyBondageLifeLanDiscoverySubsystem::DestroySockets()
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

	if (ListenSocket)
	{
		ListenSocket->Close();
		if (SocketSubsystem)
		{
			SocketSubsystem->DestroySocket(ListenSocket);
		}
		ListenSocket = nullptr;
	}

	if (SendSocket)
	{
		SendSocket->Close();
		if (SocketSubsystem)
		{
			SocketSubsystem->DestroySocket(SendSocket);
		}
		SendSocket = nullptr;
	}
}

bool UMyBondageLifeLanDiscoverySubsystem::HandleTick(float DeltaSeconds)
{
	EnsureSockets();
	PumpIncoming();

	if (bAnnouncing)
	{
		// Keep the announced player count in sync with the live listen server.
		UGameInstance* GameInstance = GetGameInstance();
		const UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
		if (World)
		{
			int32 PlayerCount = 0;
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				++PlayerCount;
			}

			PlayerCount = FMath::Max(PlayerCount, 1);
			if (PlayerCount != AnnouncedRoom.CurrentPlayers)
			{
				AnnouncedRoom.CurrentPlayers = PlayerCount;
				UE_LOG(LogMyBondageLife, Log, TEXT("LAN discovery: room '%s' now announces %d/%d players"),
					*AnnouncedRoom.RoomName, AnnouncedRoom.CurrentPlayers, AnnouncedRoom.MaxPlayers);
			}
		}

		TimeUntilNextAnnounce -= DeltaSeconds;
		if (TimeUntilNextAnnounce <= 0.0)
		{
			SendAnnouncement();
			TimeUntilNextAnnounce = AnnounceIntervalSeconds;
		}
	}

	PruneStaleRooms();

	return true;
}

void UMyBondageLifeLanDiscoverySubsystem::StartAnnouncing(const FMyBondageLifeRoomInfo& Room)
{
	AnnouncedRoom = Room;
	bAnnouncing = true;
	TimeUntilNextAnnounce = 0.0;

	UE_LOG(LogMyBondageLife, Log, TEXT("LAN discovery: announcing room '%s' at %s"),
		*AnnouncedRoom.RoomName, *AnnouncedRoom.GetEndpoint());

	SendAnnouncement();
}

void UMyBondageLifeLanDiscoverySubsystem::StopAnnouncing()
{
	if (bAnnouncing)
	{
		UE_LOG(LogMyBondageLife, Log, TEXT("LAN discovery: stopped announcing room '%s'"), *AnnouncedRoom.RoomName);
	}
	bAnnouncing = false;
}

void UMyBondageLifeLanDiscoverySubsystem::SetAnnouncedPlayerCount(int32 CurrentPlayers)
{
	AnnouncedRoom.CurrentPlayers = FMath::Max(CurrentPlayers, 1);
}

void UMyBondageLifeLanDiscoverySubsystem::ForceAnnounce()
{
	if (bAnnouncing)
	{
		TimeUntilNextAnnounce = 0.0;
		SendAnnouncement();
	}
}

void UMyBondageLifeLanDiscoverySubsystem::SendAnnouncement()
{
	if (!bAnnouncing || !SendSocket)
	{
		return;
	}

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		return;
	}

	const FString Payload = SerializeRoom(AnnouncedRoom);
	FTCHARToUTF8 Utf8Payload(*Payload);
	int32 BytesSent = 0;

	// Global broadcast reaches other machines, loopback covers several instances
	// running on this one (tests, editor plus a standalone client).
	TSharedRef<FInternetAddr> BroadcastAddress = SocketSubsystem->CreateInternetAddr();
	BroadcastAddress->SetBroadcastAddress();
	BroadcastAddress->SetPort(DiscoveryPort);
	SendSocket->SendTo(reinterpret_cast<const uint8*>(Utf8Payload.Get()), Utf8Payload.Length(), BytesSent, *BroadcastAddress);

	TSharedRef<FInternetAddr> LoopbackAddress = SocketSubsystem->CreateInternetAddr();
	LoopbackAddress->SetLoopbackAddress();
	LoopbackAddress->SetPort(DiscoveryPort);
	SendSocket->SendTo(reinterpret_cast<const uint8*>(Utf8Payload.Get()), Utf8Payload.Length(), BytesSent, *LoopbackAddress);
}

void UMyBondageLifeLanDiscoverySubsystem::PumpIncoming()
{
	if (!ListenSocket)
	{
		return;
	}

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		return;
	}

	TArray<uint8> Buffer;
	Buffer.SetNumUninitialized(ReceiveBufferSize);

	TSharedRef<FInternetAddr> SenderAddress = SocketSubsystem->CreateInternetAddr();
	const double Now = FPlatformTime::Seconds();

	for (;;)
	{
		int32 BytesRead = 0;
		if (!ListenSocket->RecvFrom(Buffer.GetData(), Buffer.Num(), BytesRead, *SenderAddress))
		{
			break;
		}

		if (BytesRead <= 0)
		{
			break;
		}

		const FString Payload = FString(FUTF8ToTCHAR(reinterpret_cast<const ANSICHAR*>(Buffer.GetData()), BytesRead));

		FMyBondageLifeRoomInfo Room;
		if (!DeserializeRoom(Payload, Room))
		{
			continue;
		}

		// Our own beacon, bouncing back through the loopback/broadcast path.
		if (bAnnouncing && Room.RoomId == AnnouncedRoom.RoomId)
		{
			continue;
		}

		if (Room.HostAddress.IsEmpty())
		{
			Room.HostAddress = SenderAddress->ToString(false);
		}

		const bool bNewRoom = !RoomsById.Contains(Room.RoomId);
		RoomsById.Add(Room.RoomId, Room);
		LastSeenById.Add(Room.RoomId, Now);

		if (bNewRoom)
		{
			UE_LOG(LogMyBondageLife, Log, TEXT("LAN discovery: found room '%s' at %s"),
				*Room.RoomName, *Room.GetEndpoint());
		}
	}
}

void UMyBondageLifeLanDiscoverySubsystem::PruneStaleRooms()
{
	if (RoomsById.Num() == 0)
	{
		return;
	}

	const double Now = FPlatformTime::Seconds();

	TArray<FString> ExpiredIds;
	for (const TPair<FString, double>& Pair : LastSeenById)
	{
		if (Now - Pair.Value > RoomTimeoutSeconds)
		{
			ExpiredIds.Add(Pair.Key);
		}
	}

	for (const FString& Id : ExpiredIds)
	{
		if (const FMyBondageLifeRoomInfo* Room = RoomsById.Find(Id))
		{
			UE_LOG(LogMyBondageLife, Log, TEXT("LAN discovery: lost room '%s' at %s"), *Room->RoomName, *Room->GetEndpoint());
		}
		RoomsById.Remove(Id);
		LastSeenById.Remove(Id);
	}
}

TArray<FMyBondageLifeRoomInfo> UMyBondageLifeLanDiscoverySubsystem::GetDiscoveredRooms() const
{
	const double Now = FPlatformTime::Seconds();

	TArray<FMyBondageLifeRoomInfo> Result;
	Result.Reserve(RoomsById.Num());

	for (const TPair<FString, FMyBondageLifeRoomInfo>& Pair : RoomsById)
	{
		FMyBondageLifeRoomInfo Room = Pair.Value;
		if (const double* LastSeen = LastSeenById.Find(Pair.Key))
		{
			Room.LastSeenAge = Now - *LastSeen;
		}
		Result.Add(Room);
	}

	Result.Sort([](const FMyBondageLifeRoomInfo& A, const FMyBondageLifeRoomInfo& B)
	{
		return A.RoomName < B.RoomName;
	});

	return Result;
}

void UMyBondageLifeLanDiscoverySubsystem::ClearDiscoveredRooms()
{
	RoomsById.Reset();
	LastSeenById.Reset();
}

FString UMyBondageLifeLanDiscoverySubsystem::SerializeRoom(const FMyBondageLifeRoomInfo& Room)
{
	return FString::Printf(TEXT("%s|%s|%s|%s|%d|%d|%d"),
		RoomPacketTag,
		*SanitizeField(Room.RoomId),
		*SanitizeField(Room.RoomName),
		*SanitizeField(Room.HostAddress),
		Room.Port,
		Room.CurrentPlayers,
		Room.MaxPlayers);
}

bool UMyBondageLifeLanDiscoverySubsystem::DeserializeRoom(const FString& Payload, FMyBondageLifeRoomInfo& OutRoom)
{
	TArray<FString> Fields;
	Payload.ParseIntoArray(Fields, TEXT("|"), true);

	if (Fields.Num() < 7 || Fields[0] != RoomPacketTag)
	{
		return false;
	}

	OutRoom.RoomId = Fields[1];
	OutRoom.RoomName = Fields[2];
	OutRoom.HostAddress = Fields[3];
	OutRoom.Port = FCString::Atoi(*Fields[4]);
	OutRoom.CurrentPlayers = FCString::Atoi(*Fields[5]);
	OutRoom.MaxPlayers = FCString::Atoi(*Fields[6]);

	if (OutRoom.RoomId.IsEmpty() || OutRoom.Port <= 0 || OutRoom.HostAddress.IsEmpty())
	{
		return false;
	}

	return true;
}
