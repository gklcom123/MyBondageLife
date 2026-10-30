// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/MyBondageLifeAccountSubsystem.h"
#include "MyBondageLifeFriendRowWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeFriendRowClicked, const FString&, Account);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeFriendRowMenuClicked, const FString&, Account);

/**
 *  One row of the friend list.
 *
 *  Designed in /Game/UI/WBP_FriendRow (parent = this class). Optional widgets:
 *  W_Select (button covering the row), W_Avatar (image), W_Name, W_Status,
 *  W_Role (text) and W_MenuButton (button).
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeFriendRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Fills the row from a friend entry. */
	UFUNCTION(BlueprintCallable, Category = "Friends")
	void SetFriendEntry(const FMyBondageLifeFriendEntry& InEntry, bool bInSelected);

	UFUNCTION(BlueprintCallable, Category = "Friends")
	void SetSelected(bool bInSelected);

	UFUNCTION(BlueprintPure, Category = "Friends")
	FString GetAccountName() const { return Entry.AccountName; }

	UPROPERTY(BlueprintAssignable, Category = "Friends")
	FMyBondageLifeFriendRowClicked OnRowClicked;

	UPROPERTY(BlueprintAssignable, Category = "Friends")
	FMyBondageLifeFriendRowMenuClicked OnRowMenuClicked;

	UPROPERTY(BlueprintReadOnly, Category = "Friends", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(BlueprintReadOnly, Category = "Friends", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(BlueprintReadOnly, Category = "Friends", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> RoleText;

	UPROPERTY(BlueprintReadOnly, Category = "Friends", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UImage> AvatarImage;

protected:

	void BuildLayout();

	UFUNCTION()
	void HandleRowClicked();

	UFUNCTION()
	void HandleMenuClicked();

	UFUNCTION()
	void HandleRestraintsChanged();

	UPROPERTY(Transient)
	TObjectPtr<UButton> SelectButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> MenuButton;

	FMyBondageLifeFriendEntry Entry;
	bool bSelected = false;
	bool bLayoutBuilt = false;
};
