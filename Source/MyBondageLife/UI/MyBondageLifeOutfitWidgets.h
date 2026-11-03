// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeOutfitWidgets.generated.h"

class UButton;
class UImage;
class UPanelWidget;
class UTextBlock;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeOutfitSlotClicked, int32, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMyBondageLifeOutfitColorRequested, int32, Slot, int32, Channel);



/** Click relay for one colour button (W_ChangeColor_<slot>_<channel>). */
UCLASS()
class UMyBondageLifeOutfitColorProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY() TObjectPtr<UMyBondageLifeOutfitSlotsWidget> Owner;
	int32 Slot = 0;
	int32 Channel = 1;

	UFUNCTION() void HandleClicked();
};
/** Click relay for one clothing slot. */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeOutfitSlotProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY() TObjectPtr<UMyBondageLifeOutfitSlotsWidget> Owner;
	int32 Slot = 0;

	UFUNCTION()
	void HandleClicked();
};

/** Click relay for one picker entry. */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeOutfitItemProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY() TObjectPtr<UMyBondageLifeOutfitPickerWidget> Owner;
	FString ItemId;

	UFUNCTION()
	void HandleClicked();
};
/**
 *  Clothing slot grid (11 slots, same idea as the restraint grid).
 *
 *  Designed in /Game/UI/WBP_OutfitSlots. Per slot N (0..10) these names are adopted:
 *    W_OutfitSlotButton_N (button), W_OutfitIcon_N (image), W_OutfitLabel_N (text)
 *  Missing ones are generated in code.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeOutfitSlotsWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;


	/** Refreshes icons/labels from the equipped map (own or viewed account). */
	UFUNCTION(BlueprintCallable, Category = "Outfit")
	void RefreshSlots(const TMap<int32, FString>& Equipped);

	/** 空 = 自己，非空 = 查看该好友的衣柜。 */
	void SetViewAccount(const FString& InAccount);

	FString ViewAccount;

	void HandleSlotClicked(int32 Slot);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMyBondageLifeOutfitSlotProxy>> ClickProxies;

	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<class UButton>> ColorButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMyBondageLifeOutfitColorProxy>> ColorProxies;

	void HandleColorClicked(int32 Slot, int32 Channel);

	UPROPERTY(BlueprintAssignable, Category = "Outfit")
	FMyBondageLifeOutfitSlotClicked OnSlotClicked;

	UPROPERTY(BlueprintAssignable, Category = "Outfit")
	FMyBondageLifeOutfitColorRequested OnColorRequested;

protected:

	void BuildLayout();



	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> SlotButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> SlotIcons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> SlotLabels;

	bool bLayoutBuilt = false;

	/** 布局尚未构建时先缓存，构建完成后再套用。 */
	TMap<int32, FString> PendingEquipped;
	bool bHasPending = false;
};

/**
 *  Clothing picker popup: list of items for one slot.
 *
 *  Designed in /Game/UI/WBP_OutfitPicker:
 *    W_OutfitPicker_Panel (border), W_OutfitPicker_Title (text), W_OutfitPicker_List (panel),
 *    W_OutfitPicker_Remove (button), W_OutfitPicker_Cancel (button)
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeOutfitPickerWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	UFUNCTION(BlueprintCallable, Category = "Outfit")
	void OpenForSlot(int32 InSlot, const TMap<int32, FString>& Equipped);

	UFUNCTION(BlueprintCallable, Category = "Outfit")
	void ClosePicker();

	void ChooseItem(const FString& ItemId);

	UFUNCTION(BlueprintPure, Category = "Outfit")
	FString GetChosenItemId() const { return ChosenItemId; }

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMyBondageLifeOutfitItemProxy>> ItemProxies;

	UPROPERTY(BlueprintAssignable, Category = "Outfit")
	FMyBondageLifeOutfitSlotClicked OnItemChosen;

	UPROPERTY(BlueprintAssignable, Category = "Outfit")
	FMyBondageLifeOutfitSlotClicked OnRemoveChosen;

protected:

	void BuildLayout();
	void RefreshContents();

	UFUNCTION()
	void HandleRemoveClicked();

	UFUNCTION()
	void HandleCancelClicked();

	int32 PickerSlot = INDEX_NONE;
	FString EquippedItemId;
	FString ChosenItemId;

	UPROPERTY(Transient) TObjectPtr<class UBorder> PickerPanel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PickerTitle;
	UPROPERTY(Transient) TObjectPtr<UPanelWidget> PickerList;
	UPROPERTY(Transient) TObjectPtr<UButton> RemoveButton;

	bool bLayoutBuilt = false;

	/** 布局尚未构建时先缓存，构建完成后再套用。 */
	TMap<int32, FString> PendingEquipped;
	bool bHasPending = false;
};
