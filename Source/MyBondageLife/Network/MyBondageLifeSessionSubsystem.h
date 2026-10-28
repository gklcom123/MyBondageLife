// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MyBondageLifeLanTypes.h"
#include "MyBondageLifeServerRoom.h"
#include "MyBondageLifeSessionSubsystem.generated.h"

/**
 *  Owns the LAN "room" the player is currently in.
 *
 *  Hosting starts a listen server on the current map and begins announcing the
 *  room on the LAN. Joining performs a ClientTravel straight to the announced
 *  endpoint. No backend is involved yet - this is the piece the future account
 *  and relay service will sit on top of.
 *
 *  Lives on the GameInstance so the state survives ServerTravel/ClientTravel.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	static constexpr int32 DefaultGamePort = 7777;
	static constexpr int32 DefaultMaxPlayers = 8;

	/** Opens the current map as a listen server and starts announcing the room. */
	UFUNCTION(BlueprintCallable, Category = "LAN|Session")
	bool HostRoom(const FString& InRoomName);

	/** Travels to a discovered room. */
	UFUNCTION(BlueprintCallable, Category = "LAN|Session")
	bool JoinRoom(const FMyBondageLifeRoomInfo& Room);

	/** Opens the current map as an EOS listen server and registers the room on the backend. */
	UFUNCTION(BlueprintCallable, Category = "EOS|Session")
	bool HostRoomWithEOS(const FString& InRoomName, const FString& HostPuid);

	/** Connects to a backend room through EOS P2P. */
	UFUNCTION(BlueprintCallable, Category = "EOS|Session")
	bool JoinServerRoom(const FMyBondageLifeServerRoom& Room);

	/** Stops hosting/announcing and returns to the same map as a plain client. */
	UFUNCTION(BlueprintCallable, Category = "LAN|Session")
	void LeaveRoom();

	UFUNCTION(BlueprintPure, Category = "LAN|Session")
	bool IsInSession() const { return bInSession; }

	UFUNCTION(BlueprintPure, Category = "LAN|Session")
	bool IsHosting() const { return bHosting; }

	/** "ip:port" of the room we are in (our own address when hosting). */
	UFUNCTION(BlueprintPure, Category = "LAN|Session")
	FString GetHostEndpoint() const;

	UFUNCTION(BlueprintPure, Category = "LAN|Session")
	FString GetRoomName() const { return RoomName; }

	UFUNCTION(BlueprintPure, Category = "LAN|Session")
	int32 GetGamePort() const { return GamePort; }

	/** Players currently in the room, read from the replicated game state. */
	UFUNCTION(BlueprintPure, Category = "LAN|Session")
	int32 GetPlayerCount() const;

	/** Player cap of the room we are in. */
	UFUNCTION(BlueprintPure, Category = "LAN|Session")
	int32 GetMaxPlayers() const { return MaxPlayers; }

	/** Best guess at this machine's LAN IPv4 address; falls back to 127.0.0.1. */
	static FString ResolveLocalIPv4();

private:

	/** Runs after the listen-server travel to publish the room to the backend. */
	void CreatePendingRoom();

	/**
	 *  Client-side watchdog: when the host disappears without telling us (process
	 *  killed, crash), the connection dies and this returns us to the main menu.
	 */
	void StartClientWatchdog();
	void StopClientWatchdog();

	FTSTicker::FDelegateHandle ClientWatchdogHandle;
	bool bClientEverConnected = false;
	float ClientDisconnectSeconds = 0.0f;

	bool bInSession = false;
	bool bHosting = false;

	FString RoomId;
	FString RoomName;
	FString HostIp;
	int32 GamePort = DefaultGamePort;
	int32 MaxPlayers = DefaultMaxPlayers;

	FString PendingHostPuid;
	FString PendingMapName;
	FTimerHandle PendingRoomTimer;
};
