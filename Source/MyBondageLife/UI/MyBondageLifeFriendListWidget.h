// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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

/**
 *  Bottom entry "好友列表(V)", the friend list panel and the incoming request dialog.
 *
 *  The list is backed by UMyBondageLifeAccountSubsystem::GetFriends(); without a
 *  backend the entries are filled by the in-session prototype flow.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeFriendListWidget : public UUserWidget
{
	GENERATED_BODY()

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

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void BuildLayout();

	UFUNCTION()
	void HandleOpenButtonClicked();

	UFUNCTION()
	void HandleCloseButtonClicked();

	UFUNCTION()
	void HandleFriendRowClicked();

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
