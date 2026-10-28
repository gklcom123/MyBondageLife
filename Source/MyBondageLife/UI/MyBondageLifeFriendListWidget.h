// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/MyBondageLifeAccountSubsystem.h"
#include "MyBondageLifeFriendListWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UImage;
class UScrollBox;
class UTextBlock;
class UTexture2D;

/** Raised when the friend panel opens or closes, so the controller can gate input. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeFriendPanelOpenChanged, bool, bIsOpen);

/** Raised when the player answers an incoming friend request. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeFriendRequestAnswered, bool, bAccepted);

/** Raised when the player answered an incoming contract request. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMyBondageLifeContractRequestAnswered, const FString&, RequestId, bool, bAccepted);

/** Raised when a modal dialog (menu / confirm / request) opens or closes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeSocialModalChanged, bool, bIsOpen);

/**
 *  Bottom entry "好友列表(V)", the friend list panel and the incoming request dialog.
 *
 *  The list is backed by UMyBondageLifeAccountSubsystem::GetFriends(); without a
 *  backend the entries are filled by the in-session prototype flow.
 */
class UMyBondageLifeFriendListWidget;

/** Turns a row button click into a per-friend call (dynamic delegates carry no payload). */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeFriendRowProxy : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION()
	void HandleClicked();

	UPROPERTY(Transient)
	TObjectPtr<UMyBondageLifeFriendListWidget> Owner;

	FString Account;
	FString ContractId;
	EMyBondageLifeContractRole Role = EMyBondageLifeContractRole::None;
};

UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeFriendListWidget : public UUserWidget
{
	GENERATED_BODY()

	friend class UMyBondageLifeFriendRowProxy;

public:

	UMyBondageLifeFriendListWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Friends")
	void SetPanelOpen(bool bOpen);

	UFUNCTION(BlueprintCallable, Category = "Friends")
	void TogglePanel();

	UFUNCTION(BlueprintPure, Category = "Friends")
	bool IsPanelOpen() const;

	/** Rebuilds the rows from the local friend list. */
	UFUNCTION(BlueprintCallable, Category = "Friends")
	void RefreshFriends();

	/** Shows "[name] 请求添加为好友，是否同意?". */
	UFUNCTION(BlueprintCallable, Category = "Friends")
	void ShowFriendRequest(const FString& RequestId, const FString& RequesterCharacterName);

	/** Request id of the dialog currently on screen. */
	UFUNCTION(BlueprintPure, Category = "Friends")
	FString GetPendingRequestId() const { return PendingRequestId; }

	UFUNCTION(BlueprintPure, Category = "Friends")
	bool IsRequestDialogOpen() const;

	UPROPERTY(BlueprintAssignable, Category = "Friends")
	FMyBondageLifeFriendPanelOpenChanged OnPanelOpenChanged;

	UPROPERTY(BlueprintAssignable, Category = "Friends")
	FMyBondageLifeFriendRequestAnswered OnRequestAnswered;

	UPROPERTY(BlueprintAssignable, Category = "Friends")
	FMyBondageLifeContractRequestAnswered OnContractRequestAnswered;

	UPROPERTY(BlueprintAssignable, Category = "Friends")
	FMyBondageLifeSocialModalChanged OnModalChanged;

	/** Shows the contract confirmation dialog with the wording from the design. */
	UFUNCTION(BlueprintCallable, Category = "Friends")
	void ShowContractRequest(const FString& RequestId, const FString& RequesterCharacterName, bool bFormal);

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void BuildLayout();
	void BuildDialogs();

	UFUNCTION()
	void HandleOpenButtonClicked();

	UFUNCTION()
	void HandleCloseButtonClicked();

	UFUNCTION()
	void HandleFriendRowClicked();

	/** Opens the per-friend action menu. */
	void OpenFriendMenu(const FString& InAccountName, const FString& InContractId, EMyBondageLifeContractRole InRole);

	UFUNCTION()
	void HandleMenuChatClicked();

	UFUNCTION()
	void HandleMenuDissolveClicked();

	UFUNCTION()
	void HandleMenuDeleteClicked();

	UFUNCTION()
	void HandleMenuCancelClicked();

	UFUNCTION()
	void HandleConfirmYesClicked();

	UFUNCTION()
	void HandleConfirmNoClicked();

	/** Contract text shown in the friend list column. */
	static FString GetContractRoleText(EMyBondageLifeContractRole Role);

	/** Refreshes the modal flag and notifies the controller. */
	void UpdateModalState();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> MenuBackdrop;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MenuTitle;

	UPROPERTY(Transient)
	TObjectPtr<UButton> MenuDissolveButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MenuDissolveLabel;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ConfirmBackdrop;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ConfirmText;

	/** 0 none, 1 dissolve, 2 delete friend. */
	int32 PendingConfirmAction = 0;

	FString SelectedFriendAccount;
	FString SelectedFriendContractId;
	EMyBondageLifeContractRole SelectedFriendRole = EMyBondageLifeContractRole::None;

	/** Contract request currently shown in the request dialog. */
	FString PendingContractRequestId;
	bool bPendingContractFormal = false;
	bool bRequestDialogIsContract = false;

	UFUNCTION()
	void HandleAcceptClicked();

	UFUNCTION()
	void HandleDeclineClicked();

	UFUNCTION()
	void HandleFriendsChanged();

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> FriendIconTexture;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	//~ Bottom entry
	UPROPERTY(Transient)
	TObjectPtr<UButton> OpenButton;

	//~ Panel
	UPROPERTY(Transient)
	TObjectPtr<UBorder> PanelBorder;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> FriendListBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EmptyHint;

	/** One proxy per row, kept alive so the button bindings stay valid. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMyBondageLifeFriendRowProxy>> RowProxies;

	//~ Request dialog
	UPROPERTY(Transient)
	TObjectPtr<UBorder> RequestBackdrop;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RequestText;

	/** Account name of the player who sent the pending request. */
	FString PendingRequesterAccount;

	/** Backend id of the pending request (used for the respond call). */
	FString PendingRequestId;
};
