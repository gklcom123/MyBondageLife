// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeLockMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UMyBondageLifeLockMenuWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeLockChosen, int32, LockType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMyBondageLifeLockCancelled);

/** One of the five lock buttons. */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeLockChoiceProxy : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION()
	void HandleClicked();

	UPROPERTY(Transient)
	TObjectPtr<UMyBondageLifeLockMenuWidget> Owner;

	int32 LockType = 1;
};

/**
 *  Third level menu: the five lock types.
 *
 *  Designed on its own in /Game/UI/WBP_LockMenu (parent = this class).
 *  Optional widgets: W_LockMenu_Title, W_Lock_1 .. W_Lock_5, W_LockMenu_Cancel.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeLockMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Opens the menu for a slot; the title uses the slot name. */
	UFUNCTION(BlueprintCallable, Category = "Lock")
	void OpenForSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Lock")
	void CloseMenu();

	/** Greys out a lock type the player may not use here. */
	UFUNCTION(BlueprintCallable, Category = "Lock")
	void SetLockAvailable(int32 LockType, bool bAvailable);

	UFUNCTION(BlueprintPure, Category = "Lock")
	int32 GetSlotIndex() const { return SlotIndex; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Lock")
	void OnLayoutBuilt();

	UPROPERTY(BlueprintAssignable, Category = "Lock")
	FMyBondageLifeLockChosen OnLockChosen;

	UPROPERTY(BlueprintAssignable, Category = "Lock")
	FMyBondageLifeLockCancelled OnCancelled;

	UPROPERTY(BlueprintReadOnly, Category = "Lock", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UButton>> LockButtons;

	UPROPERTY(BlueprintReadOnly, Category = "Lock", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> MenuTitle;

	UPROPERTY(BlueprintReadOnly, Category = "Lock", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UButton> CancelButton;

protected:

	void BuildLayout();
	void HandleChoice(int32 LockType);
	void HandleCancel();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMyBondageLifeLockChoiceProxy>> ChoiceProxies;

	int32 SlotIndex = INDEX_NONE;
	bool bLayoutBuilt = false;
};
