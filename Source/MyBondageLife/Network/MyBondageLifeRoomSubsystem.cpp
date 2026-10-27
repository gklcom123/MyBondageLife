// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeRoomSubsystem.h"

#include "Core/MyBondageLifeAccountSubsystem.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "MyBondageLife.h"
#include "Network/MyBondageLifeSessionSubsystem.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	constexpr float RoomRequestTimeoutSeconds = 8.0f;
	constexpr float HeartbeatIntervalSeconds = 20.0f;
}

void UMyBondageLifeRoomSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ConnectingTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UMyBondageLifeRoomSubsystem::TickConnectingOverlay),
		0.25f);
}

void UMyBondageLifeRoomSubsystem::Deinitialize()
{
	HideConnectingOverlay();
	FTSTicker::GetCoreTicker().RemoveTicker(ConnectingTickerHandle);

	StopHeartbeat();
	Super::Deinitialize();
}

void UMyBondageLifeRoomSubsystem::RefreshRooms()
{
	SendRequest(TEXT("/rooms"), TEXT("GET"), nullptr,
		[this](bool bSuccess, int32 StatusCode, const FString& Body)
		{
			if (bSuccess)
			{
				HandleRoomsResponse(StatusCode, Body);
			}
			else
			{
				OnRoomsFailed.Broadcast(MakeErrorText(StatusCode, Body));
			}
		});
}

void UMyBondageLifeRoomSubsystem::CreateRoom(const FString& InRoomName, const FString& InHostPuid, const FString& InMapName, int32 InMaxPlayers)
{
	if (bRequestInFlight || InHostPuid.IsEmpty())
	{
		return;
	}

	const TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("name"), InRoomName);
	Payload->SetStringField(TEXT("hostPuid"), InHostPuid);
	Payload->SetStringField(TEXT("socketName"), TEXT("GameSocket"));
	Payload->SetStringField(TEXT("mapName"), InMapName);
	Payload->SetNumberField(TEXT("maxPlayers"), InMaxPlayers);
	Payload->SetNumberField(TEXT("playerCount"), 1);

	bRequestInFlight = true;
	SendRequest(TEXT("/rooms"), TEXT("POST"), Payload,
		[this](bool bSuccess, int32 StatusCode, const FString& Body)
		{
			bRequestInFlight = false;
			if (!bSuccess)
			{
				OnRoomsFailed.Broadcast(MakeErrorText(StatusCode, Body));
				return;
			}
			HandleCreateResponse(StatusCode, Body);
		});
}

void UMyBondageLifeRoomSubsystem::JoinRoom(const FString& RoomId)
{
	if (bRequestInFlight || RoomId.IsEmpty())
	{
		return;
	}

	const int32 Generation = ++JoinGeneration;
	bRequestInFlight = true;

	bTravelStarted = false;
	ConnectStartedSeconds = FPlatformTime::Seconds();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UWorld* World = GameInstance->GetWorld())
		{
			MenuMapName = World->GetMapName();
		}
	}
	ShowConnectingOverlay(FText::FromString(TEXT("\u6B63\u5728\u8FDE\u63A5\u623F\u95F4\u2026")));

	SendRequest(FString::Printf(TEXT("/rooms/%s/join"), *RoomId), TEXT("POST"), nullptr,
		[this, Generation](bool bSuccess, int32 StatusCode, const FString& Body)
		{
			if (Generation != JoinGeneration)
			{
				// Cancelled or superseded by a newer join.
				return;
			}

			bRequestInFlight = false;
			if (!bSuccess)
			{
				HideConnectingOverlay();
				OnRoomsFailed.Broadcast(MakeErrorText(StatusCode, Body));
				return;
			}
			HandleJoinResponse(StatusCode, Body);
		});
}

void UMyBondageLifeRoomSubsystem::CancelJoin()
{
	++JoinGeneration;
	bRequestInFlight = false;
	HideConnectingOverlay();
}

void UMyBondageLifeRoomSubsystem::ShowConnectingOverlay(const FText& Text)
{
	if (!GEngine || !GEngine->GameViewport)
	{
		return;
	}

	if (!ConnectingOverlay.IsValid())
	{
		SAssignNew(ConnectingTextBlock, STextBlock)
			.Text(Text)
			.ColorAndOpacity(FSlateColor(FLinearColor::White))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 32))
			.Justification(ETextJustify::Center);

		ConnectingOverlay = SNew(SBorder)
			.Visibility(EVisibility::Visible)
			.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					ConnectingTextBlock.ToSharedRef()
				]
			];

		GEngine->GameViewport->AddViewportWidgetContent(ConnectingOverlay.ToSharedRef(), 1000);
	}
	else
	{
		SetConnectingText(Text);
	}

	bOverlayVisible = true;
}

void UMyBondageLifeRoomSubsystem::SetConnectingText(const FText& Text)
{
	if (ConnectingTextBlock.IsValid())
	{
		ConnectingTextBlock->SetText(Text);
	}
}

void UMyBondageLifeRoomSubsystem::HideConnectingOverlay()
{
	if (ConnectingOverlay.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(ConnectingOverlay.ToSharedRef());
	}

	ConnectingOverlay.Reset();
	ConnectingTextBlock.Reset();
	bOverlayVisible = false;
	bTravelStarted = false;
}

bool UMyBondageLifeRoomSubsystem::TickConnectingOverlay(float DeltaSeconds)
{
	if (!bOverlayVisible)
	{
		return true;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return true;
	}

	UWorld* World = GameInstance->GetWorld();
	const double Now = FPlatformTime::Seconds();

	if (bTravelStarted)
	{
		// Success: the client world is up and the local player exists.
		if (World && World->HasBegunPlay() && World->GetNetMode() == NM_Client &&
			GameInstance->GetFirstLocalPlayerController())
		{
			HideConnectingOverlay();
			return true;
		}

		// Failure: travel bounced back to the menu without joining.
		if (World && World->HasBegunPlay() && World->GetNetMode() == NM_Standalone &&
			Now - TravelStartedSeconds > 8.0)
		{
			HideConnectingOverlay();
			OnRoomsFailed.Broadcast(FText::FromString(TEXT("\u8FDE\u63A5\u5931\u8D25\uFF0C\u8BF7\u91CD\u8BD5")));
			return true;
		}

		if (Now - TravelStartedSeconds > 45.0)
		{
			HideConnectingOverlay();
			OnRoomsFailed.Broadcast(FText::FromString(TEXT("\u8FDE\u63A5\u8D85\u65F6\uFF0C\u8BF7\u91CD\u8BD5")));
		}

		return true;
	}

	if (Now - ConnectStartedSeconds > 15.0)
	{
		CancelJoin();
		OnRoomsFailed.Broadcast(FText::FromString(TEXT("\u8FDE\u63A5\u8D85\u65F6\uFF0C\u8BF7\u91CD\u8BD5")));
	}

	return true;
}

void UMyBondageLifeRoomSubsystem::LeaveRoom()
{
	StopHeartbeat();

	if (HostedRoomId.IsEmpty())
	{
		return;
	}

	const FString RoomId = HostedRoomId;
	HostedRoomId.Reset();
	SendRequest(FString::Printf(TEXT("/rooms/%s"), *RoomId), TEXT("DELETE"), nullptr,
		[this](bool bSuccess, int32 StatusCode, const FString& Body)
		{
			if (!bSuccess)
			{
				UE_LOG(LogMyBondageLife, Warning, TEXT("Rooms: 关闭房间失败（%d）"), StatusCode);
			}
		});
}

void UMyBondageLifeRoomSubsystem::SendRequest(const FString& Path, const FString& Verb,
	const TSharedPtr<FJsonObject>& Body, TFunction<void(bool, int32, const FString&)> OnDone)
{
	if (!OnDone)
	{
		return;
	}

	const FString Url = FString::Printf(TEXT("%s%s"), *UMyBondageLifeAccountSubsystem::GetServerUrl(), *Path);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(Verb);
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));

	const FString Token = GetToken();
	if (!Token.IsEmpty())
	{
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Token));
	}

	if (Body.IsValid())
	{
		FString BodyText;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyText);
		FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);
		Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		Request->SetContentAsString(BodyText);
	}
	else if (!Verb.Equals(TEXT("GET"), ESearchCase::IgnoreCase) &&
		!Verb.Equals(TEXT("HEAD"), ESearchCase::IgnoreCase))
	{
		// Fastify rejects POST/DELETE without a JSON content type, even for an empty body.
		Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		Request->SetContentAsString(TEXT("{}"));
	}

	Request->SetTimeout(RoomRequestTimeoutSeconds);
	Request->OnProcessRequestComplete().BindLambda(
		[OnDone = MoveTemp(OnDone)](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bConnectedSuccessfully)
		{
			if (!bConnectedSuccessfully || !HttpResponse.IsValid())
			{
				OnDone(false, 0, FString());
				return;
			}

			const int32 StatusCode = HttpResponse->GetResponseCode();
			OnDone(StatusCode >= 200 && StatusCode < 300, StatusCode, HttpResponse->GetContentAsString());
		});

	if (!Request->ProcessRequest())
	{
		OnDone(false, 0, FString());
	}
}

void UMyBondageLifeRoomSubsystem::HandleRoomsResponse(int32 StatusCode, const FString& Body)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OnRoomsFailed.Broadcast(FText::FromString(TEXT("\u623F\u95F4\u5217\u8868\u6570\u636E\u65E0\u6CD5\u89E3\u6790")));
		return;
	}

	CachedRooms.Reset();

	const TArray<TSharedPtr<FJsonValue>>* RoomsArray = nullptr;
	if (Root->TryGetArrayField(TEXT("rooms"), RoomsArray) && RoomsArray)
	{
		for (const TSharedPtr<FJsonValue>& Value : *RoomsArray)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (Value.IsValid() && Value->TryGetObject(Object) && Object && Object->IsValid())
			{
				FMyBondageLifeServerRoom Room;
				if (ParseRoom(*Object, Room))
				{
					CachedRooms.Add(MoveTemp(Room));
				}
			}
		}
	}

	OnRoomsUpdated.Broadcast(CachedRooms);
}

void UMyBondageLifeRoomSubsystem::HandleCreateResponse(int32 StatusCode, const FString& Body)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OnRoomsFailed.Broadcast(FText::FromString(TEXT("\u623F\u95F4\u521B\u5EFA\u54CD\u5E94\u65E0\u6CD5\u89E3\u6790")));
		return;
	}

	const TSharedPtr<FJsonObject>* RoomObject = nullptr;
	if (!Root->TryGetObjectField(TEXT("room"), RoomObject) || !RoomObject || !RoomObject->IsValid())
	{
		OnRoomsFailed.Broadcast(FText::FromString(TEXT("\u623F\u95F4\u521B\u5EFA\u5931\u8D25")));
		return;
	}

	FMyBondageLifeServerRoom Room;
	if (!ParseRoom(*RoomObject, Room))
	{
		OnRoomsFailed.Broadcast(FText::FromString(TEXT("\u623F\u95F4\u521B\u5EFA\u54CD\u5E94\u7F3A\u5C11\u5B57\u6BB5")));
		return;
	}

	HostedRoomId = Room.RoomId;
	StartHeartbeat();

	UE_LOG(LogMyBondageLife, Log, TEXT("Rooms: 已创建房间 '%s'（%s）"), *Room.RoomName, *Room.RoomId);
}

void UMyBondageLifeRoomSubsystem::HandleJoinResponse(int32 StatusCode, const FString& Body)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		HideConnectingOverlay();
		OnRoomsFailed.Broadcast(FText::FromString(TEXT("\u52A0\u5165\u623F\u95F4\u54CD\u5E94\u65E0\u6CD5\u89E3\u6790")));
		return;
	}

	const TSharedPtr<FJsonObject>* RoomObject = nullptr;
	if (!Root->TryGetObjectField(TEXT("room"), RoomObject) || !RoomObject || !RoomObject->IsValid())
	{
		HideConnectingOverlay();
		OnRoomsFailed.Broadcast(FText::FromString(TEXT("\u52A0\u5165\u623F\u95F4\u5931\u8D25")));
		return;
	}

	FMyBondageLifeServerRoom Room;
	if (!ParseRoom(*RoomObject, Room))
	{
		HideConnectingOverlay();
		OnRoomsFailed.Broadcast(FText::FromString(TEXT("\u52A0\u5165\u623F\u95F4\u54CD\u5E94\u7F3A\u5C11\u5B57\u6BB5")));
		return;
	}

	// Keep the overlay up through ClientTravel and the map load; it is hidden
	// once the client world is actually running.
	bTravelStarted = true;
	TravelStartedSeconds = FPlatformTime::Seconds();
	SetConnectingText(FText::FromString(TEXT("\u6B63\u5728\u8FDB\u5165\u623F\u95F4\u2026")));

	OnRoomJoined.Broadcast(Room);
}

void UMyBondageLifeRoomSubsystem::StartHeartbeat()
{
	StopHeartbeat();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		GameInstance->GetTimerManager().SetTimer(
			HeartbeatTimer,
			FTimerDelegate::CreateUObject(this, &UMyBondageLifeRoomSubsystem::SendHeartbeat),
			HeartbeatIntervalSeconds,
			true);
	}
}

void UMyBondageLifeRoomSubsystem::StopHeartbeat()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		GameInstance->GetTimerManager().ClearTimer(HeartbeatTimer);
	}
}

void UMyBondageLifeRoomSubsystem::SendHeartbeat()
{
	if (HostedRoomId.IsEmpty())
	{
		return;
	}

	const TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("playerCount"), GetPlayerCount());

	SendRequest(FString::Printf(TEXT("/rooms/%s/heartbeat"), *HostedRoomId), TEXT("POST"), Payload,
		[this](bool bSuccess, int32 StatusCode, const FString& Body)
		{
			if (StatusCode == 404)
			{
				// The room expired (e.g. the host was suspended); stop trying.
				UE_LOG(LogMyBondageLife, Warning, TEXT("Rooms: 房间已过期，停止心跳"));
				HostedRoomId.Reset();
				StopHeartbeat();
			}
		});
}

FString UMyBondageLifeRoomSubsystem::GetToken() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			return Account->GetToken();
		}
	}

	return FString();
}

int32 UMyBondageLifeRoomSubsystem::GetPlayerCount() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UMyBondageLifeSessionSubsystem* Session = GameInstance->GetSubsystem<UMyBondageLifeSessionSubsystem>())
		{
			return FMath::Max(1, Session->GetPlayerCount());
		}
	}

	return 1;
}

bool UMyBondageLifeRoomSubsystem::ParseRoom(const TSharedPtr<FJsonObject>& Object, FMyBondageLifeServerRoom& OutRoom)
{
	if (!Object.IsValid())
	{
		return false;
	}

	if (!Object->TryGetStringField(TEXT("id"), OutRoom.RoomId) || OutRoom.RoomId.IsEmpty())
	{
		return false;
	}

	Object->TryGetStringField(TEXT("name"), OutRoom.RoomName);
	Object->TryGetStringField(TEXT("hostPuid"), OutRoom.HostPuid);
	Object->TryGetStringField(TEXT("socketName"), OutRoom.SocketName);
	Object->TryGetStringField(TEXT("mapName"), OutRoom.MapName);
	Object->TryGetStringField(TEXT("version"), OutRoom.Version);

	double Number = 0.0;
	if (Object->TryGetNumberField(TEXT("playerCount"), Number))
	{
		OutRoom.CurrentPlayers = static_cast<int32>(Number);
	}
	if (Object->TryGetNumberField(TEXT("maxPlayers"), Number))
	{
		OutRoom.MaxPlayers = static_cast<int32>(Number);
	}

	const TSharedPtr<FJsonObject>* HostObject = nullptr;
	if (Object->TryGetObjectField(TEXT("host"), HostObject) && HostObject && HostObject->IsValid())
	{
		(*HostObject)->TryGetStringField(TEXT("characterName"), OutRoom.HostCharacterName);
	}

	return !OutRoom.HostPuid.IsEmpty();
}

FText UMyBondageLifeRoomSubsystem::MakeErrorText(int32 StatusCode, const FString& Body)
{
	FString ServerMessage;

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
	if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
	{
		const TSharedPtr<FJsonObject>* ErrorObject = nullptr;
		if (Root->TryGetObjectField(TEXT("error"), ErrorObject) && ErrorObject && ErrorObject->IsValid())
		{
			(*ErrorObject)->TryGetStringField(TEXT("message"), ServerMessage);
		}
	}

	if (StatusCode == 0)
	{
		return FText::FromString(TEXT("\u670D\u52A1\u5668\u65E0\u54CD\u5E94\uFF0C\u8BF7\u786E\u8BA4\u670D\u52A1\u5668\u5DF2\u542F\u52A8"));
	}
	if (StatusCode == 409)
	{
		return FText::FromString(TEXT("\u623F\u95F4\u5DF2\u6EE1\u5458\u6216\u4E0D\u53EF\u52A0\u5165"));
	}
	if (StatusCode == 404)
	{
		return FText::FromString(TEXT("\u623F\u95F4\u5DF2\u5173\u95ED"));
	}
	if (StatusCode == 401)
	{
		return FText::FromString(TEXT("\u767B\u5F55\u5DF2\u8FC7\u671F\uFF0C\u8BF7\u91CD\u65B0\u767B\u5F55"));
	}

	return FText::FromString(ServerMessage.IsEmpty()
		? FString::Printf(TEXT("\u623F\u95F4\u64CD\u4F5C\u5931\u8D25\uFF08HTTP %d\uFF09"), StatusCode)
		: ServerMessage);
}
