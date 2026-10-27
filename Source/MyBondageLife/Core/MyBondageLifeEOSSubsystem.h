// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IEOSSDKManager.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MyBondageLifeEOSSubsystem.generated.h"

class FSocketSubsystemEOS;
class ISocketSubsystemEOSUtils;

/** Raised once EOS is initialised, logged in and its P2P socket subsystem is registered. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMyBondageLifeEOSReady);

/** Raised when EOS could not start; carries a player-facing message. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeEOSFailed, const FText&, ErrorMessage);

/**
 *  Owns the local EOS identity.
 *
 *  Initialises the EOS platform from [EOSSDK.Platform.MyBondageLife] in
 *  DefaultEngine.ini, signs in with the anonymous Device ID flow and exposes the
 *  resulting ProductUserId, which the room service and the P2P net driver need.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeEOSSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "EOS")
	bool IsLoggedIn() const { return bLoggedIn; }

	/** EOS ProductUserId as a string, e.g. "0002..."; empty until login succeeds. */
	UFUNCTION(BlueprintPure, Category = "EOS")
	FString GetProductUserId() const { return ProductUserId; }

	/** True once the EOS P2P socket subsystem is registered with the engine. */
	UFUNCTION(BlueprintPure, Category = "EOS")
	bool IsSocketReady() const { return SocketSubsystem != nullptr; }

	/** Called from the EOS completion callbacks. */
	void HandleCreateDeviceIdResult(int32 ResultCode);
	void HandleLoginResult(int32 ResultCode);

	UPROPERTY(BlueprintAssignable, Category = "EOS")
	FMyBondageLifeEOSReady OnEOSReady;

	UPROPERTY(BlueprintAssignable, Category = "EOS")
	FMyBondageLifeEOSFailed OnEOSFailed;

private:

	void StartDeviceAuth();
	void LoginWithDeviceId();
	void CompleteLogin();
	void SetupSocketSubsystem();
	void Fail(const FText& Message);

	IEOSPlatformHandlePtr PlatformHandle;
	TSharedPtr<ISocketSubsystemEOSUtils, ESPMode::ThreadSafe> SocketUtils;
	FSocketSubsystemEOS* SocketSubsystem = nullptr;

	FString ProductUserId;
	bool bLoggedIn = false;
	bool bLoginInFlight = false;
};
