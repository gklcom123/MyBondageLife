// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Interfaces/IHttpRequest.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MyBondageLifeAccountSubsystem.generated.h"

/** Raised when the account service accepted the credentials. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMyBondageLifeAuthSucceeded);

/** Raised when login/registration failed; carries a player-facing message. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeAuthFailed, const FText&, ErrorMessage);

/** Master/slave relation of one contract, seen from the signed-in player. */
UENUM(BlueprintType)
enum class EMyBondageLifeContractRole : uint8
{
	None,
	TemporaryMaster,
	TemporarySlave,
	Master,
	Slave
};

/** One contract (trial or formal) that the signed-in player takes part in. */
USTRUCT(BlueprintType)
struct FMyBondageLifeContractInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Contract")
	FString ContractId;

	/** The other party: my master when Role is *Slave, my slave when it is *Master. */
	UPROPERTY(BlueprintReadOnly, Category = "Contract")
	FString PeerAccount;

	UPROPERTY(BlueprintReadOnly, Category = "Contract")
	FString PeerCharacterName;

	UPROPERTY(BlueprintReadOnly, Category = "Contract")
	int32 PeerClass = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Contract")
	EMyBondageLifeContractRole Role = EMyBondageLifeContractRole::None;

	/** trialStartedAt for a trial, formalStartedAt once formal. */
	UPROPERTY(BlueprintReadOnly, Category = "Contract")
	FDateTime Since;

	/** trialStartedAt + 72h. */
	UPROPERTY(BlueprintReadOnly, Category = "Contract")
	FDateTime TrialEndsAt;
};

/** One entry of the friend list, as returned by GET /friends. */
USTRUCT(BlueprintType)
struct FMyBondageLifeFriendEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Friends")
	FString AccountName;

	UPROPERTY(BlueprintReadOnly, Category = "Friends")
	FString CharacterName;

	UPROPERTY(BlueprintReadOnly, Category = "Friends")
	int32 CharacterClass = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Friends")
	bool bOnline = false;

	/** Contract relation with this friend, as seen by the signed-in player. */
	UPROPERTY(BlueprintReadOnly, Category = "Friends")
	EMyBondageLifeContractRole ContractRole = EMyBondageLifeContractRole::None;

	UPROPERTY(BlueprintReadOnly, Category = "Friends")
	FDateTime ContractSince;

	UPROPERTY(BlueprintReadOnly, Category = "Friends")
	FDateTime ContractTrialEndsAt;

	UPROPERTY(BlueprintReadOnly, Category = "Friends")
	FString ContractId;
};

/** A pending incoming friend request, as reported by the service. */
USTRUCT(BlueprintType)
struct FMyBondageLifeFriendRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Friends")
	FString RequestId;

	UPROPERTY(BlueprintReadOnly, Category = "Friends")
	FString Account;

	UPROPERTY(BlueprintReadOnly, Category = "Friends")
	FString CharacterName;

	UPROPERTY(BlueprintReadOnly, Category = "Friends")
	int32 CharacterClass = 1;
};

/** Public profile of any account (GET /users/{account}). */
USTRUCT(BlueprintType)
struct FMyBondageLifeUserProfile
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Contract")
	FString Account;

	UPROPERTY(BlueprintReadOnly, Category = "Contract")
	FString CharacterName;

	UPROPERTY(BlueprintReadOnly, Category = "Contract")
	int32 CharacterClass = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Contract")
	FDateTime CreatedAt;

	UPROPERTY(BlueprintReadOnly, Category = "Contract")
	bool bOnline = false;

	/** True when this character is somebody's slave (trial or formal). */
	UPROPERTY(BlueprintReadOnly, Category = "Contract")
	bool bHasMaster = false;

	/** True when that relation is already formal. */
	UPROPERTY(BlueprintReadOnly, Category = "Contract")
	bool bMasterIsFormal = false;

	UPROPERTY(BlueprintReadOnly, Category = "Contract")
	FString MasterAccount;

	UPROPERTY(BlueprintReadOnly, Category = "Contract")
	FString MasterCharacterName;

	/** trialStartedAt, or formalStartedAt for a formal contract. */
	UPROPERTY(BlueprintReadOnly, Category = "Contract")
	FDateTime ContractSince;

	/** Hair style ID worn by this character. */
	UPROPERTY(BlueprintReadOnly, Category = "Contract")
	int32 HairIndex = 1;
};

/** Raised whenever the local friend list changes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMyBondageLifeFriendsChanged);

/** Raised when an incoming friend request arrives (polling). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMyBondageLifeFriendRequestReceived, const FString&, RequestId, const FString&, RequesterCharacterName);

/** Raised after we answered a friend request. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeFriendAnswerEvent, bool, bAccepted);

/** Raised after we sent a friend request. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMyBondageLifeFriendRequestSent, bool, bSucceeded, const FString&, TargetAccount);

/** Raised when the local contract state changed (after refresh or after an action). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMyBondageLifeContractsChanged);

/** Raised when an incoming contract request arrives (polling). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMyBondageLifeContractRequestReceived, const FString&, RequestId, const FString&, RequesterCharacterName, bool, bFormal);

/** Raised after a contract action (request / respond / dissolve). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMyBondageLifeContractActionResult, bool, bSucceeded, const FString&, ErrorCode, int32, Action);

/** Raised when the stored body height changed (0.9 / 1.0 / 1.1). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeHeightChanged, float, NewHeightScale);

/** Raised when the stored hair style changed (1 based DT_Hair ID). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeHairChanged, int32, NewHairIndex);

/** Raised when a public profile was fetched or refreshed. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeUserProfileUpdated, const FString&, Account);

/**
 *  Talks to the account service (register / login), keeps the signed-in state and
 *  owns the friend list, which is served by the friends endpoints.
 *
 *  Every backend error is translated into a Chinese message before it reaches the
 *  UI, so the login screen only has to display what it is given.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeAccountSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	/** Base URL of the account service (from Config/ServerConfig.ini). */
	static FString GetServerUrl();

	/** Signs in against the server; the outcome arrives through the delegates. */
	UFUNCTION(BlueprintCallable, Category = "Account")
	void RequestLogin(const FString& Account, const FString& Password);

	/** Registers a new account; on success the player is signed in directly. */
	UFUNCTION(BlueprintCallable, Category = "Account")
	void RequestRegister(const FString& Account, const FString& Password, const FString& InCharacterName, int32 InCharacterClass);

	UFUNCTION(BlueprintCallable, Category = "Account")
	void LogOut();

	UFUNCTION(BlueprintPure, Category = "Account")
	bool IsLoggedIn() const { return bLoggedIn; }

	/** True while an auth request is on the wire (buttons should be disabled). */
	UFUNCTION(BlueprintPure, Category = "Account")
	bool IsAuthRequestInFlight() const { return bAuthInFlight; }

	UFUNCTION(BlueprintPure, Category = "Account")
	FString GetAccountName() const { return AccountName; }

	UFUNCTION(BlueprintPure, Category = "Account")
	FString GetAccountId() const { return AccountId; }

	UFUNCTION(BlueprintPure, Category = "Account")
	FString GetCharacterName() const { return CharacterName; }

	UFUNCTION(BlueprintPure, Category = "Account")
	int32 GetCharacterClass() const { return CharacterClass; }

	/** JWT returned by the service, also used for the friend endpoints. */
	UFUNCTION(BlueprintPure, Category = "Account")
	FString GetToken() const { return Token; }

	/** Account creation time reported by the service (UTC). */
	UFUNCTION(BlueprintPure, Category = "Account")
	FDateTime GetCreatedAt() const { return CreatedAt; }

	/** Body height multiplier stored for this account: 0.9 / 1.0 / 1.1. */
	UFUNCTION(BlueprintPure, Category = "Account")
	float GetHeightScale() const { return HeightScale; }

	/** PATCH /users/me - persists a new body height and applies it locally. */
	UFUNCTION(BlueprintCallable, Category = "Account")
	void RequestUpdateHeight(float NewHeightScale);

	/** Hair style ID stored for this account (1 based, matches DT_Hair ID). */
	UFUNCTION(BlueprintPure, Category = "Account")
	int32 GetHairIndex() const { return HairIndex; }

	/** PATCH /users/me - persists a new hair style and applies it locally. */
	UFUNCTION(BlueprintCallable, Category = "Account")
	void RequestUpdateHair(int32 NewHairIndex);

	UPROPERTY(BlueprintAssignable, Category = "Account")
	FMyBondageLifeHairChanged OnHairChanged;

	UPROPERTY(BlueprintAssignable, Category = "Account")
	FMyBondageLifeHeightChanged OnHeightChanged;

	/** Character class as a player-facing label: 支配者 / 转换者 / 臣服者 / 奴隶. */
	UFUNCTION(BlueprintPure, Category = "Account")
	FString GetCharacterClassLabel() const;

	/** Same mapping for any class value, including other players' characters. */
	static FString MakeCharacterClassLabel(int32 ClassValue);

	/** Room name suggested when this account creates a room. */
	UFUNCTION(BlueprintPure, Category = "Account")
	FString GetDefaultRoomName() const;

	//~ Friends -----------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Friends")
	TArray<FMyBondageLifeFriendEntry> GetFriends() const { return Friends; }

	UFUNCTION(BlueprintPure, Category = "Friends")
	bool IsFriend(const FString& InAccountName) const;

	/** Updates the cached entry and broadcasts the change. */
	UFUNCTION(BlueprintCallable, Category = "Friends")
	void AddFriend(const FMyBondageLifeFriendEntry& Entry);

	/** Sets the online flag of a cached entry (used by presence fallbacks). */
	UFUNCTION(BlueprintCallable, Category = "Friends")
	void SetFriendOnline(const FString& InAccountName, bool bInOnline);

	/** GET /friends */
	UFUNCTION(BlueprintCallable, Category = "Friends")
	void RefreshFriendsFromServer();

	/** POST /friends/requests */
	UFUNCTION(BlueprintCallable, Category = "Friends")
	void RequestAddFriend(const FString& TargetAccount);

	/** POST /friends/requests/{id}/respond */
	UFUNCTION(BlueprintCallable, Category = "Friends")
	void RespondToFriendRequest(const FString& RequestId, bool bAccept, const FString& RequesterCharacterName);

	/** DELETE /friends/{account} */
	UFUNCTION(BlueprintCallable, Category = "Friends")
	void RemoveFriendOnServer(const FString& TargetAccount);

	/** GET /friends/requests?direction=incoming&status=pending */
	UFUNCTION(BlueprintCallable, Category = "Friends")
	void PollFriendRequests();

	/** POST /presence/heartbeat */
	UFUNCTION(BlueprintCallable, Category = "Friends")
	void SendPresenceHeartbeat();

	/** POST /presence/offline - marks this account offline right away. */
	UFUNCTION(BlueprintCallable, Category = "Friends")
	void SendPresenceOffline();

	//~ Contracts ---------------------------------------------------------

	/** GET /contracts/me */
	UFUNCTION(BlueprintCallable, Category = "Contract")
	void RefreshContractState();

	/** POST /contracts/requests (kind=formal needs a matured trial). */
	UFUNCTION(BlueprintCallable, Category = "Contract")
	void RequestContract(const FString& TargetAccount, bool bFormal);

	/** POST /contracts/requests/{id}/respond */
	UFUNCTION(BlueprintCallable, Category = "Contract")
	void RespondToContractRequest(const FString& RequestId, bool bAccept);

	/** DELETE /contracts/{id} */
	UFUNCTION(BlueprintCallable, Category = "Contract")
	void DissolveContract(const FString& ContractId);

	/** GET /contracts/requests?direction=incoming&status=pending */
	UFUNCTION(BlueprintCallable, Category = "Contract")
	void PollContractRequests();

	/** Contract with a specific account, as seen from this player. */
	UFUNCTION(BlueprintPure, Category = "Contract")
	EMyBondageLifeContractRole GetContractRoleWith(const FString& InAccountName) const;

	/** Looks up the cached friend entry of an account. */
	UFUNCTION(BlueprintPure, Category = "Friends")
	bool GetFriendEntry(const FString& InAccountName, FMyBondageLifeFriendEntry& OutEntry) const;

	/** Remembers that an account already has a master (known after a failed request). */
	void MarkAccountOwned(const FString& InAccountName);

	UFUNCTION(BlueprintPure, Category = "Contract")
	bool IsAccountKnownOwned(const FString& InAccountName) const;

	/** GET /users/{account}: public profile, including who owns this character. */
	UFUNCTION(BlueprintCallable, Category = "Contract")
	void RequestUserProfile(const FString& InAccountName);

	/** Cached public profile; false when it was never fetched. */
	UFUNCTION(BlueprintPure, Category = "Contract")
	bool GetUserProfile(const FString& InAccountName, FMyBondageLifeUserProfile& OutProfile) const;

	UPROPERTY(BlueprintAssignable, Category = "Contract")
	FMyBondageLifeUserProfileUpdated OnUserProfileUpdated;

	/** My master, if any. */
	UFUNCTION(BlueprintPure, Category = "Contract")
	FMyBondageLifeContractInfo GetContractAsSlave() const { return ContractAsSlave; }

	UFUNCTION(BlueprintPure, Category = "Contract")
	bool HasMaster() const { return bHasMaster; }

	/** Contracts where this player is the master. */
	UFUNCTION(BlueprintPure, Category = "Contract")
	TArray<FMyBondageLifeContractInfo> GetContractsAsMaster() const { return ContractsAsMaster; }

	UPROPERTY(BlueprintAssignable, Category = "Contract")
	FMyBondageLifeContractsChanged OnContractsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Contract")
	FMyBondageLifeContractRequestReceived OnContractRequestReceived;

	UPROPERTY(BlueprintAssignable, Category = "Contract")
	FMyBondageLifeContractActionResult OnContractActionFinished;

	UPROPERTY(BlueprintAssignable, Category = "Friends")
	FMyBondageLifeFriendsChanged OnFriendsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Friends")
	FMyBondageLifeFriendRequestReceived OnFriendRequestReceived;

	UPROPERTY(BlueprintAssignable, Category = "Friends")
	FMyBondageLifeFriendAnswerEvent OnFriendRequestAnswered;

	UPROPERTY(BlueprintAssignable, Category = "Friends")
	FMyBondageLifeFriendRequestSent OnFriendRequestSent;

	UPROPERTY(BlueprintAssignable, Category = "Account")
	FMyBondageLifeAuthSucceeded OnAuthSucceeded;

	UPROPERTY(BlueprintAssignable, Category = "Account")
	FMyBondageLifeAuthFailed OnAuthFailed;

private:

	/** Fires a POST to /auth/<Path> with the given JSON body. */
	void SendAuthRequest(const FString& Path, const TSharedRef<class FJsonObject>& Payload, bool bIsRegistration);

	void HandleAuthResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, bool bIsRegistration);

	/** Reads token/user out of a successful response. */
	bool ApplyAuthPayload(const TSharedPtr<class FJsonObject>& Payload);

	void FailAuth(const FText& Message);

	/** Turns a backend error payload into a player-facing message. */
	static FText MakeErrorText(int32 StatusCode, const FString& ErrorCode, const FString& ServerMessage, bool bIsRegistration);

	/** Polls requests / friends / presence while signed in. */
	void StartFriendPolling();
	void StopFriendPolling();

	/** Authenticated JSON call; OnDone receives the parsed body on success. */
	void SendFriendApiRequest(
		const FString& Verb,
		const FString& Path,
		const TSharedPtr<class FJsonObject>& Body,
		TFunction<void(bool bOk, int32 StatusCode, const TSharedPtr<class FJsonObject>& Json)>&& OnDone);

	bool bLoggedIn = false;
	bool bAuthInFlight = false;

	FString AccountName;
	FString AccountId;
	FString CharacterName;
	int32 CharacterClass = 1;
	FString Token;
	FDateTime CreatedAt;

	/** Body height multiplier (0.9 / 1.0 / 1.1). */
	float HeightScale = 1.0f;

	/** Hair style ID from DT_Hair (1 based). */
	int32 HairIndex = 1;

	/** Friend list, refreshed from GET /friends. */
	TArray<FMyBondageLifeFriendEntry> Friends;

	/** Public profiles fetched through GET /users/{account}. */
	TMap<FString, FMyBondageLifeUserProfile> UserProfiles;

	/** When each profile was fetched (FPlatformTime seconds), for cache expiry. */
	TMap<FString, double> UserProfileFetchSeconds;

	/** Pending requests already surfaced to the UI. */
	TArray<FString> SeenRequestIds;

	/** My master, if any (GET /contracts/me -> asSlave). */
	FMyBondageLifeContractInfo ContractAsSlave;
	bool bHasMaster = false;

	/** Contracts where I am the master. */
	TArray<FMyBondageLifeContractInfo> ContractsAsMaster;

	/** Contract requests already surfaced to the UI. */
	TArray<FString> SeenContractRequestIds;

	/** Accounts reported as already owned by the service (TARGET_ALREADY_OWNED). */
	TArray<FString> KnownOwnedAccounts;

	FTSTicker::FDelegateHandle FriendPollHandle;
	double LastRequestPollSeconds = 0.0;
	double LastFriendsRefreshSeconds = 0.0;
	double LastHeartbeatSeconds = 0.0;
	double LastContractRefreshSeconds = 0.0;
};
