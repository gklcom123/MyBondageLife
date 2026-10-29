// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeWardrobeWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UImage;
class UEditableTextBox;
class UTextBlock;
class UTexture2D;

class UMyBondageLifeWardrobeWidget;

/** Carries the slot / item of a button click into a per-instance handler. */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeWardrobeClickProxy : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION()
	void HandleClicked();

	UPROPERTY(Transient)
	TObjectPtr<UMyBondageLifeWardrobeWidget> Owner;

	/** True for a picker item button, false for a slot button. */
	bool bIsItemButton = false;

	/** True for the picker "取消" button. */
	bool bClosePicker = false;

	/** True for a lock choice in the lock menu. */
	bool bLockChoice = false;

	/** True for the "上锁 / 开锁" action button. */
	bool bLockAction = false;

	/** True for the confirm / cancel buttons of the lock parameter popup. */
	bool bLockConfirm = false;
	bool bLockCancel = false;

	/** Lock type carried by a lock menu button (1..5). */
	int32 LockType = 0;

	int32 Slot = 0;
	FName ItemId;
};

/** The wardrobe panel: three tabs, body height/hair and the restraint slots. *//** Raised when the wardrobe panel opens or closes, so the controller can gate input. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeWardrobeOpenChanged, bool, bIsOpen);

/**
 *  Wardrobe: the bottom "衣柜(B)" entry and the panel it opens.
 *
 *  Three tabs (身体 / 外观 / 拘束). Only 身体 has content for now: a body height
 *  selector that cycles 娇小 / 普通 / 高大 and persists the value through the
 *  account service (0.9 / 1.0 / 1.1).
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeWardrobeWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UMyBondageLifeWardrobeWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Wardrobe")
	void SetPanelOpen(bool bOpen);

	UFUNCTION(BlueprintCallable, Category = "Wardrobe")
	void TogglePanel();

	UFUNCTION(BlueprintPure, Category = "Wardrobe")
	bool IsPanelOpen() const;

	/** Current body height multiplier shown by the selector. */
	UFUNCTION(BlueprintPure, Category = "Wardrobe")
	float GetDisplayedHeightScale() const;

	UPROPERTY(BlueprintAssignable, Category = "Wardrobe")
	FMyBondageLifeWardrobeOpenChanged OnPanelOpenChanged;

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	void BuildLayout();

	/** Shows one of the three tabs (0 body, 1 appearance, 2 restraint). */
	void SelectTab(int32 TabIndex);

	/** Moves the height selector by Delta steps and persists the new value. */
	void StepHeight(int32 Delta);

	/** Rewrites the height caption from the current index. */
	void RefreshHeightLabel();

	UFUNCTION()
	void HandleOpenButtonClicked();

	UFUNCTION()
	void HandleCloseButtonClicked();

	UFUNCTION()
	void HandleBodyTabClicked();

	UFUNCTION()
	void HandleAppearanceTabClicked();

	UFUNCTION()
	void HandleRestraintTabClicked();

	UFUNCTION()
	void HandleHeightDownClicked();

	UFUNCTION()
	void HandleHeightUpClicked();

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> WardrobeIconTexture;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	//~ Bottom entry
	UPROPERTY(Transient)
	TObjectPtr<UButton> OpenButton;

	//~ Panel
	UPROPERTY(Transient)
	TObjectPtr<UBorder> PanelBorder;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BodyTabLabel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> AppearanceTabLabel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RestraintTabLabel;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> BodyPage;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> AppearancePage;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RestraintPage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HeightValueLabel;

	UPROPERTY(Transient)
	TObjectPtr<UButton> HeightDownButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> HeightUpButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HeightDownLabel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HeightUpLabel;

	/** Greys out the arrow that would leave the allowed height range. */
	void RefreshArrowStates();

	/** Moves the hair selector by Delta steps and persists the new style. */
	void StepHair(int32 Delta);

	/** Rewrites the hair caption from the current style. */
	void RefreshHairLabel();

	/** Greys out the hair arrow that would leave the table range. */
	void RefreshHairArrowStates();

	UFUNCTION()
	void HandleHairDownClicked();

	UFUNCTION()
	void HandleHairUpClicked();

	UPROPERTY(Transient)
	TObjectPtr<UButton> HairDownButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> HairUpButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HairDownLabel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HairUpLabel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HairValueLabel;

	/** Current DT_Hair style ID shown by the selector. */
	int32 HairId = 1;

	//~ Restraint slots ------------------------------------------------------

	/** Builds the 12 slot buttons plus the picker popup inside the 拘束 page. */
public:
	/** Switches the panel to another account's wardrobe (body tab disabled). */
	void EnterFriendMode(const FString& InAccount);

protected:

	bool IsFriendMode() const { return bFriendMode; }
	FString GetFriendAccount() const { return FriendAccount; }

	FString FriendAccount;
	bool bFriendMode = false;

	void BuildRestraintSlots();

	/** Rewrites slot icons / labels from the account equipment. */
	void RefreshRestraintSlots();

	/** Opens the item picker for one slot. */
	void OpenRestraintPicker(int32 Slot);

	/** Hides the picker again. */
	void CloseRestraintPicker();

	/** Equips an item in the slot the picker was opened for. */
	void PickRestraintItem(FName ItemId);

	friend class UMyBondageLifeWardrobeClickProxy;

	/** Slot index the picker is currently editing. */
	int32 PickerSlot = 0;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> SlotButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> SlotIcons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> SlotLabels;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> PickerBackdrop;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> PickerGrid;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PickerTitle;

	//~ Restraint locks ------------------------------------------------------

	/** Adds the corner lock badge + the 上锁/开锁 button to the picker. */
	void BuildLockWidgets();

	/** Re-applies lock badges on the 12 slots. */
	void RefreshLockBadges();

	/** Updates the 上锁/开锁 button for the slot the picker was opened for. */
	void RefreshLockActionButton();

	/** 上锁 when unlocked, 开锁 when locked (password locks open the popup). */
	void HandleLockAction();

	/** Whether the local player may apply this lock type to the slot owner. */
	bool IsLockTypeAllowed(int32 LockType) const;

	/** Equipment or lock state changed on the server: repaint slots and the button. */
	UFUNCTION()
	void HandleRestraintsChanged();

	/** Rewrites the picker title with the remaining lock time. */
	void UpdateLockCountdown();
	void StopLockCountdown();

	/** Plays UI_Lock_Cue (silently skipped when the asset is missing). */
	void PlayLockCue();

	FDateTime PickerLockExpiry;
	bool bPickerLockHasExpiry = false;
	int32 PickerLockType = 0;
	FTimerHandle LockCountdownTimer;

	void OpenLockMenu();
	void CloseLockMenu();

	/** Opens the time / password popup for locks that need a parameter. */
	void OpenLockInput(int32 InLockType);
	void CloseLockInput();
	void ConfirmLockInput();

	/** Lock type picked in the lock menu (0 when none). */
	int32 PendingLockType = 0;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> SlotLockBadges;

	UPROPERTY(Transient)
	TObjectPtr<UButton> LockActionButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LockActionLabel;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> LockMenuBackdrop;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> LockMenuGrid;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LockMenuTitle;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> LockInputBackdrop;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LockInputTitle;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LockInputHint;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> LockInputBox;

	/** Index into the height presets (0 = 娇小, 1 = 普通, 2 = 高大). */
	int32 HeightIndex = 1;

	/** Keeps the per-button proxies alive. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMyBondageLifeWardrobeClickProxy>> ClickProxies;
};
