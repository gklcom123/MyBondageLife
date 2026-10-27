// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MyBondageLifeLanTypes.h"
#include "MyBondageLifeLanDiscoverySubsystem.generated.h"

class FSocket;

/**
 *  Zero-configuration LAN room discovery over UDP broadcast.
 *
 *  There is no backend yet: a host periodically broadcasts a small text packet
 *  describing its room (name, ip, port, player count), and every instance keeps
 *  a time-stamped list of what it heard. Rooms that stop announcing are dropped
 *  after a few seconds.
 *
 *  The subsystem lives on the GameInstance, so hosting keeps announcing across
 *  the ServerTravel that starts the listen server.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeLanDiscoverySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** UDP port used for room beacons. */
	static constexpr int32 DiscoveryPort = 25000;

	/** How often a host re-announces its room. */
	static constexpr double AnnounceIntervalSeconds = 1.0;

	/** A room that has not been heard from for this long is removed. */
	static constexpr double RoomTimeoutSeconds = 5.0;

	/** Starts broadcasting the given room. Safe to call before ServerTravel. */
	void StartAnnouncing(const FMyBondageLifeRoomInfo& Room);

	/** Stops broadcasting. Discovered rooms stay until they time out. */
	void StopAnnouncing();

	bool IsAnnouncing() const { return bAnnouncing; }

	/** Updates the player count that gets announced. */
	void SetAnnouncedPlayerCount(int32 CurrentPlayers);

	/** Rooms currently visible on the network, ordered by name. */
	UFUNCTION(BlueprintCallable, Category = "LAN|Discovery")
	TArray<FMyBondageLifeRoomInfo> GetDiscoveredRooms() const;

	UFUNCTION(BlueprintCallable, Category = "LAN|Discovery")
	void ClearDiscoveredRooms();

	/** Forces an immediate announcement, instead of waiting for the next interval. */
	void ForceAnnounce();

private:

	bool HandleTick(float DeltaSeconds);

	void EnsureSockets();
	void DestroySockets();

	/** Drains every pending datagram without blocking. */
	void PumpIncoming();

	void SendAnnouncement();

	void PruneStaleRooms();

	static FString SerializeRoom(const FMyBondageLifeRoomInfo& Room);
	static bool DeserializeRoom(const FString& Payload, FMyBondageLifeRoomInfo& OutRoom);

	FSocket* ListenSocket = nullptr;
	FSocket* SendSocket = nullptr;

	bool bAnnouncing = false;
	double TimeUntilNextAnnounce = 0.0;
	FMyBondageLifeRoomInfo AnnouncedRoom;

	/** Rooms keyed by id. */
	TMap<FString, FMyBondageLifeRoomInfo> RoomsById;

	/** Last time (FPlatformTime::Seconds) each room was heard from. */
	TMap<FString, double> LastSeenById;

	FTSTicker::FDelegateHandle TickHandle;
};
