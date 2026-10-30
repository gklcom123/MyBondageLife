// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeRestraintPickerWidget.generated.h"

class UButton;
class UCanvasPanel;
class UImage;
class UTextBlock;
class UMyBondageLifeRestraintPickerWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifePickerClosed, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifePickerLockRequested, int32, SlotIndex);

/** Item button wrapper (dynamic delegates need a UObject). */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifePickerItemProxy : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION()
	void HandleClicked();

	UPROPERTY(Transient)
	TObjectPtr<UMyBondageLifeRestraintPickerWidget> Owner;

	FName ItemId;
	int32 Slot = 0;
};

/**
 *  The slot popup: title, item grid, lock action and cancel.
 *
 *  Designed on its own in /Game/UI/WBP_RestraintPicker (parent = this class).
 *  Optional widgets: W_Picker_Title, W_Picker_Grid, W_Picker_LockAction,
 *  W_Picker_LockAction_Label, W_Picker_Cancel.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeRestraintPickerWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	/** Shows the popup for a slot. Empty account = the local player. */
	UFUNCTION(BlueprintCallable, Category = "Restraint")
	void OpenForSlot(int32 SlotIndex, const FString& ViewAccount);

	UFUNCTION(BlueprintCallable, Category = "Restraint")
	void ClosePicker();

	UFUNCTION(BlueprintPure, Category = "Restraint")
	int32 GetOpenSlot() const { return PickerSlot; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Restraint")
	void OnLayoutBuilt();

	UPROPERTY(BlueprintAssignable, Category = "Restraint")
	FMyBondageLifePickerClosed OnPickerClosed;

	UPROPERTY(BlueprintAssignable, Category = "Restraint")
	FMyBondageLifePickerLockRequested OnLockRequested;

	UPROPERTY(BlueprintReadOnly, Category = "Restraint", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> PickerTitle;

	UPROPERTY(BlueprintReadOnly, Category = "Restraint", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCanvasPanel> PickerGrid;

	UPROPERTY(BlueprintReadOnly, Category = "Restraint", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UButton> LockActionButton;

	UPROPERTY(BlueprintReadOnly, Category = "Restraint", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> LockActionLabel;

protected:

	void BuildLayout();
	void RefreshContents();
public:
	void HandleItemClicked(FName ItemId);

protected:
	void HandleCancelClicked();
	void HandleLockActionClicked();

	UFUNCTION()
	void HandleRestraintsChanged();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMyBondageLifePickerItemProxy>> ItemProxies;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CancelButton;

	int32 PickerSlot = INDEX_NONE;
	FString ViewAccount;
	bool bLayoutBuilt = false;
};
