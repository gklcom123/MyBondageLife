// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyBondageLifePlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UMainSceneWidget;
class UMyBondageLifeLoginWidget;
class UMyBondageLifeHostInfoWidget;
class UMyBondageLifeNameplateLayer;
class UMyBondageLifeGameMenuWidget;
class UMyBondageLifeChatWidget;
class UMyBondageLifePlayerInfoWidget;
class UMyBondageLifeFriendListWidget;
class UMyBondageLifeWardrobeWidget;
class AMyBondageLifeCharacter;

/**
 *  Player controller for the third person game.
 *
 *  Owns the front-end flow: login screen -> main menu -> in-session HUD, plus the
 *  chat, the personal info panel and the friend list. Which screen appears on
 *  BeginPlay is decided by the persistent subsystems, so the flow also survives the
 *  travel that starts or joins a room.
 */
UCLASS(abstract)
class AMyBondageLifePlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	AMyBondageLifePlayerController();

	//~ Login screen ------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Login")
	void ShowLogin();

	UFUNCTION(BlueprintCallable, Category = "Login")
	void HideLogin();

	UFUNCTION(BlueprintPure, Category = "Login")
	bool IsLoginVisible() const;

	//~ Main menu ---------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "MainScene")
	void ShowMainScene();

	UFUNCTION(BlueprintCallable, Category = "MainScene")
	void HideMainScene();

	UFUNCTION(BlueprintPure, Category = "MainScene")
	bool IsMainSceneVisible() const;

	//~ Session overlay ---------------------------------------------------

	/** Top-right room readout shown while in a room. */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void ShowHostInfo();

	UFUNCTION(BlueprintCallable, Category = "Session")
	void HideHostInfo();

	//~ Nameplates ---------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Nameplate")
	void ShowNameplateLayer();

	UFUNCTION(BlueprintCallable, Category = "Nameplate")
	void HideNameplateLayer();

	//~ In-game menu (Escape) ----------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "GameMenu")
	void ToggleGameMenu();

	UFUNCTION(BlueprintCallable, Category = "GameMenu")
	void ShowGameMenu();

	UFUNCTION(BlueprintCallable, Category = "GameMenu")
	void HideGameMenu();

	UFUNCTION(BlueprintPure, Category = "GameMenu")
	bool IsGameMenuOpen() const;

	//~ Chat ---------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Chat")
	void ShowChat();

	UFUNCTION(BlueprintCallable, Category = "Chat")
	void HideChat();

	UFUNCTION(BlueprintCallable, Category = "Chat")
	void EnterChatInputMode();

	UFUNCTION(BlueprintCallable, Category = "Chat")
	void LeaveChatInputMode(bool bClearText);

	UFUNCTION(BlueprintPure, Category = "Chat")
	bool IsChatInputActive() const { return bChatTextInput; }

	//~ Player info ---------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "PlayerInfo")
	void ShowPlayerInfo();

	UFUNCTION(BlueprintCallable, Category = "PlayerInfo")
	void HidePlayerInfo();

	UFUNCTION(BlueprintCallable, Category = "PlayerInfo")
	void TogglePlayerInfo();

	UFUNCTION(BlueprintPure, Category = "PlayerInfo")
	bool IsPlayerInfoOpen() const;

	//~ Friends -------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Friends")
	void ShowFriendList();

	UFUNCTION(BlueprintCallable, Category = "Friends")
	void HideFriendList();

	UFUNCTION(BlueprintCallable, Category = "Friends")
	void ToggleFriendList();

	UFUNCTION(BlueprintPure, Category = "Friends")
	bool IsFriendListOpen() const { return bFriendPanelOpen; }

	//~ Wardrobe -------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Wardrobe")
	void ShowWardrobe();

	/** Creates the HUD entry button that opens the wardrobe. */
	UFUNCTION(BlueprintCallable, Category = "Wardrobe")
	void ShowWardrobeButton();

	/** Creates the HUD "个人信息（C）" entry. */
	UFUNCTION(BlueprintCallable, Category = "PlayerInfo")
	void ShowPlayerInfoEntry();

	UPROPERTY(EditDefaultsOnly, Category = "PlayerInfo")
	TSubclassOf<class UMyBondageLifePlayerInfoEntryWidget> PlayerInfoEntryWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<class UMyBondageLifePlayerInfoEntryWidget> PlayerInfoEntryWidget;

	UPROPERTY(EditDefaultsOnly, Category = "PlayerInfo")
	int32 PlayerInfoEntryZOrder = 12;

	UPROPERTY(EditDefaultsOnly, Category = "Wardrobe")
	TSubclassOf<class UMyBondageLifeWardrobeButtonWidget> WardrobeButtonWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<class UMyBondageLifeWardrobeButtonWidget> WardrobeButtonWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Wardrobe")
	int32 WardrobeButtonZOrder = 12;

	/** Opens the wardrobe of another account (friend), with the body tab disabled. */
	UFUNCTION(BlueprintCallable, Category = "Wardrobe")
	void OpenFriendWardrobe(const FString& InAccount);

	/** Polls the account every couple of seconds so remote gear changes apply fast. */
	void SyncRestraintsFromServer();

	FTimerHandle RestraintSyncTimer;

	UFUNCTION(BlueprintCallable, Category = "Wardrobe")
	void HideWardrobe();

	/** Opens or closes the wardrobe (B key and the bottom entry). */
	UFUNCTION(BlueprintCallable, Category = "Wardrobe")
	void ToggleWardrobe();

	UFUNCTION(BlueprintPure, Category = "Wardrobe")
	bool IsWardrobeOpen() const { return bWardrobeOpen; }

	//~ Animation test -------------------------------------------------------

	/** Toggles the temporary restraint pose on the local character. */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void ToggleRestraintPose();

	UFUNCTION(Server, Reliable)
	void ServerSetRestraintPose(bool bActive);

	/** F9: in-game shortcut for the same toggle (the menu button is only in the menu). */
	UFUNCTION()
	void HandleRestraintTestKeyPressed();

	/** True when the menu asked for the pose before a pawn existed. */
	bool bRestraintPosePending = false;

protected:

	/** Widget class used for the login screen. */
	UPROPERTY(EditDefaultsOnly, Category = "Login")
	TSubclassOf<UMyBondageLifeLoginWidget> LoginWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UMyBondageLifeLoginWidget> LoginWidget;

	/** Widget class used for the main menu. */
	UPROPERTY(EditDefaultsOnly, Category = "MainScene")
	TSubclassOf<UMainSceneWidget> MainSceneWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UMainSceneWidget> MainSceneWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Session")
	TSubclassOf<UMyBondageLifeHostInfoWidget> HostInfoWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UMyBondageLifeHostInfoWidget> HostInfoWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Nameplate")
	TSubclassOf<UMyBondageLifeNameplateLayer> NameplateLayerClass;

	UPROPERTY(Transient)
	TObjectPtr<UMyBondageLifeNameplateLayer> NameplateLayer;

	UPROPERTY(EditDefaultsOnly, Category = "GameMenu")
	TSubclassOf<UMyBondageLifeGameMenuWidget> GameMenuWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UMyBondageLifeGameMenuWidget> GameMenuWidget;

	/** Stops/starts pawn input while a menu or panel is up. */
	void SetGameplayInputBlocked(bool bBlocked);

	//~ Chat ---------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, Category = "Chat")
	TSubclassOf<UMyBondageLifeChatWidget> ChatWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UMyBondageLifeChatWidget> ChatWidget;

	UFUNCTION()
	void HandleChatEnterPressed();

	UFUNCTION()
	void HandleAltPressed();

	UFUNCTION()
	void HandleAltReleased();

	UFUNCTION()
	void HandleChatMessageSubmitted(const FString& Message);

	UFUNCTION()
	void HandleChatInputEnded();

	UFUNCTION(Server, Reliable)
	void ServerSendChatMessage(const FString& Message);

	UFUNCTION(Client, Reliable)
	void ClientReceiveChatMessage(const FString& SenderAccount, const FString& SenderName, const FString& Message);

	UFUNCTION(Client, Reliable)
	void ClientReceiveSystemMessage(const FString& Message);

	/** The host left the room: this client returns to the main menu. */
	UFUNCTION(Client, Reliable)
	void ClientHostLeftRoom();

	/** Client -> host keepalive; the host answers with ClientRoomHeartbeatAck. */
	UFUNCTION(Server, Unreliable)
	void ServerRoomHeartbeat();

	UFUNCTION(Client, Unreliable)
	void ClientRoomHeartbeatAck();

	/** Client-side keepalive bookkeeping (see TickRoomHeartbeat). */
	void TickRoomHeartbeat();

	/** Seconds (FPlatformTime) of the last ack from the host; 0 = never. */
	double LastHostAckSeconds = 0.0;
	double LastHeartbeatSentSeconds = 0.0;

	friend class UMyBondageLifeSessionSubsystem;

	void BroadcastSystemMessage(const FString& Message);

	FString GetDisplayCharacterName() const;

	bool bJoinNoticeSent = false;
	FString JoinNoticeName;
	TArray<FString> PendingSystemMessages;

	/** Re-applies the input mode for chat / panels / Alt mouse mode. */
	void RefreshChatInputState();

	bool bChatTextInput = false;
	bool bAltMouseMode = false;
	bool bPlayerInfoOpen = false;
	bool bFriendPanelOpen = false;

	/** Mirrors the engine's move/look ignore counters so they never drift. */
	bool bGameplayInputBlocked = false;

	/** True while an incoming friend request dialog waits for an answer. */
	bool bFriendRequestPending = false;

	/** True while the wardrobe panel is open. */
	bool bWardrobeOpen = false;

	double LastChatInputEndSeconds = 0.0;

	//~ Player info ---------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, Category = "PlayerInfo")
	TSubclassOf<UMyBondageLifePlayerInfoWidget> PlayerInfoWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UMyBondageLifePlayerInfoWidget> PlayerInfoWidget;

	UFUNCTION()
	void HandlePlayerInfoOpenChanged(bool bIsOpen);

	UFUNCTION()
	void HandlePlayerInfoKeyPressed();

	UFUNCTION()
	void HandleInspectPressed();

	void UpdateNearbyCharacter();

	UPROPERTY(Transient)
	TWeakObjectPtr<AMyBondageLifeCharacter> NearbyCharacter;

	UPROPERTY(EditDefaultsOnly, Category = "PlayerInfo")
	float InteractionRange = 200.0f;

	/** Raised by the info panel when the player asks to befriend the shown character. */
	UFUNCTION()
	void HandleAddFriendRequested(const AMyBondageLifeCharacter* Target);

	//~ Friends -------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, Category = "Friends")
	TSubclassOf<UMyBondageLifeFriendListWidget> FriendListWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UMyBondageLifeFriendListWidget> FriendListWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Wardrobe")
	TSubclassOf<UMyBondageLifeWardrobeWidget> WardrobeWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UMyBondageLifeWardrobeWidget> WardrobeWidget;

	UFUNCTION()
	void HandleWardrobeOpenChanged(bool bIsOpen);

	UFUNCTION()
	void HandleWardrobeKeyPressed();

	/** The stored body height changed: push it to the pawn. */
	UFUNCTION()
	void HandleHeightChanged(float NewHeightScale);

	/** The stored hair style changed: push it to the pawn. */
	UFUNCTION()
	void HandleHairChanged(int32 NewHairIndex);

	/** Sends the locally equipped restraints to the server pawn. */
	void PushRestraintsToPawn();

	/** The account equipment changed: re-apply it on the pawn. */
	UFUNCTION()
	void HandleRestraintsChanged();

	UFUNCTION(Server, Reliable)
	void ServerSetRestraints(const TArray<int32>& Slots, const TArray<FName>& Items);



	//~ Contracts -----------------------------------------------------------

	/** The info panel asked to start (or formalise) a contract. */
	UFUNCTION()
	void HandleContractRequested(const AMyBondageLifeCharacter* Target, bool bFormal);

	/** Result of a contract request / respond / dissolve call. */
	UFUNCTION()
	void HandleContractActionFinished(bool bSucceeded, const FString& ErrorCode, int32 Action);

	/** An incoming contract request arrived (polling). */
	UFUNCTION()
	void HandleContractRequestReceived(const FString& RequestId, const FString& RequesterCharacterName, bool bFormal);

	/** The player answered the contract request dialog. */
	UFUNCTION()
	void HandleContractRequestAnswered(const FString& RequestId, bool bAccepted);

	/** A social dialog (menu / confirm / request) opened or closed. */
	UFUNCTION()
	void HandleSocialModalChanged(bool bIsOpen);

	/** Contract state changed: refresh the panels that show it. */
	UFUNCTION()
	void HandleContractsChanged();

	/** True while a friend menu / confirm / request dialog owns the mouse. */
	bool bSocialModalOpen = false;

	UFUNCTION()
	void HandleFriendPanelOpenChanged(bool bIsOpen);

	UFUNCTION()
	void HandleFriendRequestAnswered(bool bAccepted);

	UFUNCTION()
	void HandleFriendListKeyPressed();

	/** An incoming request arrived from the backend. */
	UFUNCTION()
	void HandleFriendRequestReceived(const FString& RequestId, const FString& RequesterCharacterName);

	/** The backend finished answering a request we received. */
	UFUNCTION()
	void HandleFriendAnswerFinished(bool bAccepted);

	/** The backend accepted (or rejected) a request we sent. */
	UFUNCTION()
	void HandleFriendRequestSent(bool bSucceeded, const FString& TargetAccount);

	/** Character name of the player we just asked to befriend, for chat feedback. */
	FString PendingAddFriendName;

	/** Legacy in-session prototype RPCs (kept for compatibility, no longer used). */
	UFUNCTION(Server, Reliable)
	void ServerSendFriendRequest(const FString& TargetAccountName);

	UFUNCTION(Server, Reliable)
	void ServerAnswerFriendRequest(const FString& RequesterAccountName, bool bAccepted);

	UFUNCTION(Client, Reliable)
	void ClientFriendRequestReceived(const FString& RequesterAccountName, const FString& RequesterCharacterName, int32 RequesterClass);

	UFUNCTION(Client, Reliable)
	void ClientFriendRequestAnswered(const FString& AnswererAccountName, const FString& AnswererCharacterName, int32 AnswererClass, bool bAccepted);

	void RefreshFriendPresence();

	FString PendingRequesterAccount;
	FString PendingRequesterName;
	int32 PendingRequesterClass = 1;

	//~ Identity ------------------------------------------------------------

	UFUNCTION()
	void HandleLoginCompleted();

	void ApplyAccountNameToPlayerState();

	void ApplyCharacterNameToPawn();

	/** Registers the Enhanced Input mapping contexts (movement / look / jump). */
	void EnsureInputSetup();

	UFUNCTION(Server, Reliable)
	void ServerSetCharacterInfo(const FString& NewCharacterName, int32 NewCharacterClass, int64 NewCreatedAtUnix, float NewHeightScale, int32 NewHairIndex);

	virtual void OnPossess(APawn* InPawn) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//~ AActor / APlayerController interface
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	void SetMenuInputMode(bool bMenuMode);

	//~ Input mapping setup ------------------------------------------------

	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	UPROPERTY(EditAnywhere, Category = "Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	bool ShouldUseTouchControls() const;

public:

	// ---------------------------------------------------------------- message center
	UFUNCTION(BlueprintCallable, Category = "Message")
	void ShowMessageHud();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowMainHud();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	bool HostInMainHud(FName HostName, UUserWidget* Widget);

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<class UMyBondageLifeMainHudWidget> MainHudWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<class UMyBondageLifeMainHudWidget> MainHudWidget;

	UFUNCTION(BlueprintCallable, Category = "Message")
	void ToggleMessageCenter();

	UFUNCTION(BlueprintCallable, Category = "Message")
	void SetMessageCenterOpen(bool bOpen);

	UPROPERTY(Transient)
	bool bMessageCenterOpen = false;

	UFUNCTION()
	void HandleMessageKeyPressed();

	UFUNCTION(BlueprintCallable, Category = "Message")
	bool IsMessageCenterOpen() const;

	UFUNCTION()
	void HandleNotificationAdded(const FMyBondageLifeNotification& Notification);

	UFUNCTION()
	void PollNotifications();

	UPROPERTY(EditDefaultsOnly, Category = "Message")
	TSubclassOf<class UMyBondageLifeMessageButtonWidget> MessageButtonWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Message")
	TSubclassOf<class UMyBondageLifeMessageCenterWidget> MessageCenterWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<class UMyBondageLifeMessageButtonWidget> MessageButtonWidget;

	UPROPERTY(Transient)
	TObjectPtr<class UMyBondageLifeMessageCenterWidget> MessageCenterWidget;

	FTimerHandle NotificationPollTimer;};
