// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/MyBondageLifeAccountSubsystem.h"

#include "IWebSocket.h"
#include "Json.h"
#include "MyBondageLife.h"
#include "TimerManager.h"
#include "WebSocketsModule.h"

/**
 *  Realtime channel (see docs/RealtimeAPI.md).
 *
 *  The server pushes "restraints.changed" when anybody (including a friend) touches
 *  our wardrobe, so the character refreshes without polling. Polling stays as a
 *  fallback while the socket is down.
 */
void UMyBondageLifeAccountSubsystem::ConnectRealtime()
{
	if (bRealtimeConnected || Token.IsEmpty())
	{
		return;
	}

	FString Url = GetServerUrl();
	Url.ReplaceInline(TEXT("https://"), TEXT("wss://"));
	Url.ReplaceInline(TEXT("http://"), TEXT("ws://"));
	while (Url.EndsWith(TEXT("/")))
	{
		Url.LeftChopInline(1);
	}
	Url += TEXT("/realtime?token=") + Token;

	RealtimeSocket = FWebSocketsModule::Get().CreateWebSocket(Url, TEXT(""));

	RealtimeSocket->OnConnected().AddLambda([this]()
	{
		bRealtimeConnected = true;
		RealtimeRetryIndex = 0;
		UE_LOG(LogMyBondageLife, Log, TEXT("Realtime: connected."));
	});

	RealtimeSocket->OnConnectionError().AddLambda([this](const FString& Error)
	{
		bRealtimeConnected = false;
		UE_LOG(LogMyBondageLife, Warning, TEXT("Realtime: error %s"), *Error);
		ScheduleRealtimeReconnect();
	});

	RealtimeSocket->OnClosed().AddLambda([this](int32 Code, const FString& Reason, bool)
	{
		bRealtimeConnected = false;
		UE_LOG(LogMyBondageLife, Log, TEXT("Realtime: closed %d (%s)."), Code, *Reason);
		ScheduleRealtimeReconnect();
	});

	RealtimeSocket->OnMessage().AddLambda([this](const FString& Message)
	{
		HandleRealtimeMessage(Message);
	});

	RealtimeSocket->Connect();
}

void UMyBondageLifeAccountSubsystem::DisconnectRealtime()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RealtimeReconnectTimer);
	}

	if (RealtimeSocket.IsValid())
	{
		RealtimeSocket->Close();
		RealtimeSocket.Reset();
	}

	bRealtimeConnected = false;
}

void UMyBondageLifeAccountSubsystem::HandleRealtimeMessage(const FString& InMessage)
{
	TSharedPtr<FJsonObject> Json;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InMessage);
	if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
	{
		return;
	}

	FString Type;
	Json->TryGetStringField(TEXT("type"), Type);

	if (Type == TEXT("ping"))
	{
		if (RealtimeSocket.IsValid())
		{
			RealtimeSocket->Send(TEXT("{\"type\":\"pong\"}"));
		}
		return;
	}

	if (Type == TEXT("ready") || Type == TEXT("restraints.changed"))
	{
		// The event carries no payload on purpose: always re-read the truth.
		RefreshRestraints();
		return;
	}

	if (Type == TEXT("error"))
	{
		FString Code;
		Json->TryGetStringField(TEXT("code"), Code);
		UE_LOG(LogMyBondageLife, Warning, TEXT("Realtime: server error %s"), *Code);
	}
}

void UMyBondageLifeAccountSubsystem::ScheduleRealtimeReconnect()
{
	UWorld* World = GetWorld();
	if (!World || Token.IsEmpty())
	{
		return;
	}

	static const float Delays[] = { 1.0f, 2.0f, 5.0f, 10.0f, 30.0f };
	const int32 Index = FMath::Clamp(RealtimeRetryIndex, 0, UE_ARRAY_COUNT(Delays) - 1);
	++RealtimeRetryIndex;

	World->GetTimerManager().SetTimer(RealtimeReconnectTimer, this,
		&UMyBondageLifeAccountSubsystem::ConnectRealtime, Delays[Index], false);
}
