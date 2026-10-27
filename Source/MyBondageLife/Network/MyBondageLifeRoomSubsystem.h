// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Interfaces/IHttpRequest.h"
#include "MyBondageLifeServerRoom.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MyBondageLifeRoomSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeRoomsUpdated, const TArray<FMyBondageLifeServerRoom>&, Rooms);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeRoomsFailed, const FText&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeRoomJoined, const FMyBondageLifeServerRoom&, Room);

/**
 *  Talks to the room service: lists open rooms, creates the local room,
 *  heartbeats it while hosting and resolves join requests into EOS endpoints.
 *
 *  The token comes from the account subsystem; nothing here stores credentials.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeRoomSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** GET /rooms; results arrive through OnRoomsUpdated / OnRoomsFailed. */
	UFUNCTION(BlueprintCallable, Category = "Rooms")
	void RefreshRooms();

	/** POST /rooms and start the heartbeat once it succeeds. */
	UFUNCTION(BlueprintCallable, Category = "Rooms")
	void CreateRoom(const FString& InRoomName, const FString& InHostPuid, const FString& InMapName, int32 InMaxPlayers);

	/** POST /rooms/{id}/join; OnRoomJoined carries the host PUID to connect to. */
	UFUNCTION(BlueprintCallable, Category = "Rooms")
	void JoinRoom(const FString& RoomId);

	/** Abandons an in-flight join; a late response is ignored. */
	UFUNCTION(BlueprintCallable, Category = "Rooms")
	void CancelJoin();

	/** True while the persistent "connecting" overlay is on screen. */
	UFUNCTION(BlueprintPure, Category = "Rooms")
	bool IsConnecting() const { return bOverlayVisible; }

	/** DELETE /rooms/{id} and stop the heartbeat. */
	UFUNCTION(BlueprintCallable, Category = "Rooms")
	void LeaveRoom();

	UFUNCTION(BlueprintPure, Category = "Rooms")
	const TArray<FMyBondageLifeServerRoom>& GetRooms() const { return CachedRooms; }

	UFUNCTION(BlueprintPure, Category = "Rooms")
	bool IsBusy() const { return bRequestInFlight; }

	UPROPERTY(BlueprintAssignable, Category = "Rooms")
	FMyBondageLifeRoomsUpdated OnRoomsUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Rooms")
	FMyBondageLifeRoomsFailed OnRoomsFailed;

	UPROPERTY(BlueprintAssignable, Category = "Rooms")
	FMyBondageLifeRoomJoined OnRoomJoined;

private:

	void SendRequest(const FString& Path, const FString& Verb, const TSharedPtr<class FJsonObject>& Body,
		TFunction<void(bool bSuccess, int32 StatusCode, const FString& Body)> OnDone);

	void HandleRoomsResponse(int32 StatusCode, const FString& Body);
	void HandleCreateResponse(int32 StatusCode, const FString& Body);
	void HandleJoinResponse(int32 StatusCode, const FString& Body);

	void StartHeartbeat();
	void StopHeartbeat();
	void SendHeartbeat();

	FString GetToken() const;
	int32 GetPlayerCount() const;

	/** Parses one room object; returns false when required fields are missing. */
	static bool ParseRoom(const TSharedPtr<class FJsonObject>& Object, FMyBondageLifeServerRoom& OutRoom);

	/** Extracts the player-facing message from an error payload. */
	static FText MakeErrorText(int32 StatusCode, const FString& Body);

	TArray<FMyBondageLifeServerRoom> CachedRooms;
	FString HostedRoomId;

	FTimerHandle HeartbeatTimer;
	bool bRequestInFlight = false;
	int32 JoinGeneration = 0;

	/** Persistent Slate overlay: survives ClientTravel, unlike the menu widget. */
	void ShowConnectingOverlay(const FText& Text);
	void SetConnectingText(const FText& Text);
	void HideConnectingOverlay();
	bool TickConnectingOverlay(float DeltaSeconds);

	TSharedPtr<class SWidget> ConnectingOverlay;
	TSharedPtr<class STextBlock> ConnectingTextBlock;
	FTSTicker::FDelegateHandle ConnectingTickerHandle;

	bool bOverlayVisible = false;
	bool bTravelStarted = false;
	double ConnectStartedSeconds = 0.0;
	double TravelStartedSeconds = 0.0;
	FString MenuMapName;
};
