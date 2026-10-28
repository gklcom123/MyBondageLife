// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeSessionSubsystem.h"

#include "MyBondageLifePlayerController.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "IPAddress.h"
#include "Kismet/GameplayStatics.h"
#include "MyBondageLife.h"
#include "MyBondageLifeLanDiscoverySubsystem.h"
#include "MyBondageLifeRoomSubsystem.h"
#include "Core/MyBondageLifeEOSSubsystem.h"
#include "SocketSubsystem.h"
#include "Sockets.h"

namespace
{
	const TCHAR* IpNetDriverClass = TEXT("/Script/OnlineSubsystemUtils.IpNetDriver");
	const TCHAR* EOSNetDriverClass = TEXT("/Script/SocketSubsystemEOS.NetDriverEOS");

	/**
	 *  Picks the transport used for the next travel.
	 *
	 *  The project defaults GameNetDriver to EOS. Local (LAN) rooms need plain IP:
	 *  two instances on one machine share a single EOS device id, so they cannot
	 *  address each other over EOS. Setting the definition before travelling makes
	 *  the engine create the requested driver.
	 */
	void ForceGameNetDriverClass(const TCHAR* DriverClassName)
	{
		if (!GEngine)
		{
			return;
		}

		for (FNetDriverDefinition& Definition : GEngine->NetDriverDefinitions)
		{
			if (Definition.DefName == FName(TEXT("GameNetDriver")))
			{
				Definition.DriverClassName = FName(DriverClassName);
				UE_LOG(LogMyBondageLife, Log, TEXT("NetDriver: GameNetDriver -> %s"), DriverClassName);
			}
		}
	}
}

bool UMyBondageLifeSessionSubsystem::HostRoom(const FString& InRoomName)
{
	UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("HostRoom: no world available."));
		return false;
	}

	RoomName = InRoomName.IsEmpty() ? FString(TEXT("LAN Room")) : InRoomName;
	RoomId = FGuid::NewGuid().ToString(EGuidFormats::Short);
	HostIp = ResolveLocalIPv4();
	GamePort = DefaultGamePort;
	MaxPlayers = DefaultMaxPlayers;
	bInSession = true;
	bHosting = true;

	// Start beaconing before the travel: the discovery subsystem survives ServerTravel.
	if (UMyBondageLifeLanDiscoverySubsystem* Discovery = GameInstance->GetSubsystem<UMyBondageLifeLanDiscoverySubsystem>())
	{
		FMyBondageLifeRoomInfo Room;
		Room.RoomId = RoomId;
		Room.RoomName = RoomName;
		Room.HostAddress = HostIp;
		Room.Port = GamePort;
		Room.CurrentPlayers = 1;
		Room.MaxPlayers = DefaultMaxPlayers;
		Discovery->StartAnnouncing(Room);
	}

	const FString LevelName = UGameplayStatics::GetCurrentLevelName(World, true);
	UE_LOG(LogMyBondageLife, Log, TEXT("HostRoom: hosting '%s' at %s:%d on level '%s'"),
		*RoomName, *HostIp, GamePort, *LevelName);

	// Local rooms use plain IP so several instances on one machine can connect.
	ForceGameNetDriverClass(IpNetDriverClass);
	UGameplayStatics::OpenLevel(World, FName(*LevelName), true, TEXT("listen"));
	return true;
}

bool UMyBondageLifeSessionSubsystem::JoinRoom(const FMyBondageLifeRoomInfo& Room)
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return false;
	}

	APlayerController* PlayerController = GameInstance->GetFirstLocalPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("JoinRoom: no local player controller."));
		return false;
	}

	if (Room.HostAddress.IsEmpty() || Room.Port <= 0)
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("JoinRoom: invalid room endpoint."));
		return false;
	}

	RoomId = Room.RoomId;
	RoomName = Room.RoomName;
	HostIp = Room.HostAddress;
	GamePort = Room.Port;

	// Watch the connection: a host that dies without a goodbye leaves us stranded.
	StartClientWatchdog();
	MaxPlayers = Room.MaxPlayers > 0 ? Room.MaxPlayers : DefaultMaxPlayers;
	bInSession = true;
	bHosting = false;

	// A joined client stops beaconing in case it was hosting before.
	if (UMyBondageLifeLanDiscoverySubsystem* Discovery = GameInstance->GetSubsystem<UMyBondageLifeLanDiscoverySubsystem>())
	{
		Discovery->StopAnnouncing();
	}

	const FString Endpoint = Room.GetEndpoint();
	UE_LOG(LogMyBondageLife, Log, TEXT("JoinRoom: travelling to '%s' (%s)"), *RoomName, *Endpoint);

	// Local rooms use plain IP so several instances on one machine can connect.
	ForceGameNetDriverClass(IpNetDriverClass);
	PlayerController->ClientTravel(Endpoint, ETravelType::TRAVEL_Absolute);
	return true;
}

bool UMyBondageLifeSessionSubsystem::HostRoomWithEOS(const FString& InRoomName, const FString& HostPuid)
{
	if (HostPuid.IsEmpty())
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("HostRoomWithEOS: empty EOS PUID."));
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("HostRoomWithEOS: no world available."));
		return false;
	}

	RoomName = InRoomName.IsEmpty() ? FString(TEXT("Room")) : InRoomName;
	RoomId.Reset();
	HostIp = HostPuid;
	GamePort = DefaultGamePort;
	MaxPlayers = DefaultMaxPlayers;
	bInSession = true;
	bHosting = true;

	if (UMyBondageLifeLanDiscoverySubsystem* Discovery = GameInstance->GetSubsystem<UMyBondageLifeLanDiscoverySubsystem>())
	{
		Discovery->StopAnnouncing();
	}

	PendingHostPuid = HostPuid;
	PendingMapName = UGameplayStatics::GetCurrentLevelName(World, true);

	// Publishing waits until the listen server has finished travelling.
	GameInstance->GetTimerManager().SetTimer(
		PendingRoomTimer,
		FTimerDelegate::CreateUObject(this, &UMyBondageLifeSessionSubsystem::CreatePendingRoom),
		2.0f,
		false);

	// Cloud rooms go through EOS P2P.
	ForceGameNetDriverClass(EOSNetDriverClass);

	UE_LOG(LogMyBondageLife, Log, TEXT("HostRoomWithEOS: hosting '%s' as an EOS listen server (PUID %s)."),
		*RoomName, *HostPuid);

	UGameplayStatics::OpenLevel(World, FName(*PendingMapName), true, TEXT("listen"));
	return true;
}

void UMyBondageLifeSessionSubsystem::CreatePendingRoom()
{
	UGameInstance* GameInstance = GetGameInstance();
	UMyBondageLifeRoomSubsystem* Rooms = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeRoomSubsystem>()
		: nullptr;

	if (!Rooms)
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("CreatePendingRoom: room subsystem unavailable."));
		return;
	}

	Rooms->CreateRoom(RoomName, PendingHostPuid, PendingMapName, MaxPlayers);
}

bool UMyBondageLifeSessionSubsystem::JoinServerRoom(const FMyBondageLifeServerRoom& Room)
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return false;
	}

	APlayerController* PlayerController = GameInstance->GetFirstLocalPlayerController();
	if (!PlayerController || Room.HostPuid.IsEmpty())
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("JoinServerRoom: no local player controller or empty host PUID."));
		return false;
	}

	RoomId = Room.RoomId;
	RoomName = Room.RoomName;
	HostIp = Room.HostPuid;
	MaxPlayers = Room.MaxPlayers > 0 ? Room.MaxPlayers : DefaultMaxPlayers;
	bInSession = true;
	bHosting = false;

	StartClientWatchdog();

	if (UMyBondageLifeLanDiscoverySubsystem* Discovery = GameInstance->GetSubsystem<UMyBondageLifeLanDiscoverySubsystem>())
	{
		Discovery->StopAnnouncing();
	}

	ForceGameNetDriverClass(EOSNetDriverClass);

	const FString Address = Room.GetEOSAddress();
	UE_LOG(LogMyBondageLife, Log, TEXT("JoinServerRoom: travelling to '%s' (%s)."), *RoomName, *Address);

	if (const UMyBondageLifeEOSSubsystem* EOS = GameInstance->GetSubsystem<UMyBondageLifeEOSSubsystem>())
	{
		if (EOS->GetProductUserId() == Room.HostPuid)
		{
			UE_LOG(LogMyBondageLife, Warning,
				TEXT("JoinServerRoom: 本机与房主 PUID 相同。同一台机器多开实例会共用一个 EOS 设备身份，"
					"EOS 无法区分两个实例；请用两台机器测试穿透。"));
		}
	}

	PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
	return true;
}

void UMyBondageLifeSessionSubsystem::LeaveRoom()
{
	UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;

	if (GameInstance)
	{
		GameInstance->GetTimerManager().ClearTimer(PendingRoomTimer);

		if (UMyBondageLifeLanDiscoverySubsystem* Discovery = GameInstance->GetSubsystem<UMyBondageLifeLanDiscoverySubsystem>())
		{
			Discovery->StopAnnouncing();
		}

		if (UMyBondageLifeRoomSubsystem* Rooms = GameInstance->GetSubsystem<UMyBondageLifeRoomSubsystem>())
		{
			Rooms->LeaveRoom();
		}
	}

	// On the host this ends the room for everyone: a client left in a world without a
	// server cannot play, so it is sent back to the main menu too.
	if (World && World->GetNetMode() != NM_Client)
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (AMyBondageLifePlayerController* OtherController = Cast<AMyBondageLifePlayerController>(It->Get()))
			{
				if (!OtherController->IsLocalController())
				{
					OtherController->ClientHostLeftRoom();
				}
			}
		}
	}

	StopClientWatchdog();

	bInSession = false;
	bHosting = false;
	RoomId.Reset();
	RoomName.Reset();

	if (!World)
	{
		return;
	}

	const FString LevelName = UGameplayStatics::GetCurrentLevelName(World, true);
	UGameplayStatics::OpenLevel(World, FName(*LevelName), true, FString());
}

FString UMyBondageLifeSessionSubsystem::GetHostEndpoint() const
{
	return FString::Printf(TEXT("%s:%d"), *HostIp, GamePort);
}

int32 UMyBondageLifeSessionSubsystem::GetPlayerCount() const
{
	// PlayerArray is replicated, so this is right on the host and on every client.
	const UGameInstance* GameInstance = GetGameInstance();
	const UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;

	return GameState ? GameState->PlayerArray.Num() : 0;
}

FString UMyBondageLifeSessionSubsystem::ResolveLocalIPv4()
{
	if (ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
	{
		bool bCanBindAll = false;
		const TSharedRef<FInternetAddr> LocalAddress = SocketSubsystem->GetLocalHostAddr(*GLog, bCanBindAll);
		const FString AddressString = LocalAddress->ToString(false);
		if (!AddressString.IsEmpty() && AddressString != TEXT("0.0.0.0"))
		{
			return AddressString;
		}
	}

	return TEXT("127.0.0.1");
}

void UMyBondageLifeSessionSubsystem::StartClientWatchdog()
{
	if (ClientWatchdogHandle.IsValid())
	{
		return;
	}

	bClientEverConnected = false;
	ClientDisconnectSeconds = 0.0f;

	ClientWatchdogHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this,
		[this](float DeltaTime)
		{
			if (!bInSession || bHosting)
			{
				ClientDisconnectSeconds = 0.0f;
				return true;
			}

			const UWorld* World = GetWorld();
			if (!World || World->GetNetMode() != NM_Client)
			{
				// Still loading the level: nothing to watch yet.
				return true;
			}

			const UNetDriver* Driver = World->GetNetDriver();
			const bool bConnected = Driver != nullptr && Driver->ServerConnection != nullptr;

			if (bConnected)
			{
				bClientEverConnected = true;
				ClientDisconnectSeconds = 0.0f;
				return true;
			}

			// The host went away (killed process / crash): there is no RPC to receive,
			// so the missing connection is the only signal.
			if (bClientEverConnected)
			{
				ClientDisconnectSeconds += DeltaTime;
				if (ClientDisconnectSeconds >= 2.0f)
				{
					UE_LOG(LogMyBondageLife, Log, TEXT("Session: lost the host connection, leaving the room."));
					ClientDisconnectSeconds = 0.0f;
					bClientEverConnected = false;
					LeaveRoom();
				}
			}

			return true;
		}), 0.5f);
}

void UMyBondageLifeSessionSubsystem::StopClientWatchdog()
{
	if (ClientWatchdogHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ClientWatchdogHandle);
		ClientWatchdogHandle.Reset();
	}

	bClientEverConnected = false;
	ClientDisconnectSeconds = 0.0f;
}
