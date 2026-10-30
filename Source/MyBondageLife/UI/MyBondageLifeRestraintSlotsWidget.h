// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeRestraintSlotsWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UMyBondageLifeRestraintSlotsWidget;

/** Per-slot click forwarding (dynamic delegates need a UObject). */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeRestraintSlotClickProxy : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION()
	void HandleClicked();

	UPROPERTY(Transient)
	TObjectPtr<UMyBondageLifeRestraintSlotsWidget> Owner;

	int32 Slot = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeRestraintSlotClicked, int32, SlotIndex);

/**
 *  The 12 restraint slots plus the hint line under them.
 *
 *  Split out of the wardrobe panel so it can be designed on its own in
 *  /Game/UI/WBP_RestraintSlots (parent = this class).
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeRestraintSlotsWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Empty = the local player; otherwise another account's wardrobe. */
	UFUNCTION(BlueprintCallable, Category = "Restraint")
	void SetViewAccount(const FString& InAccount);

	/** Re-reads the equipment and repaints every slot. */
	UFUNCTION(BlueprintCallable, Category = "Restraint")
	void RefreshSlots();

	UFUNCTION(BlueprintImplementableEvent, Category = "Restraint")
	void OnLayoutBuilt();

	UPROPERTY(BlueprintAssignable, Category = "Restraint")
	FMyBondageLifeRestraintSlotClicked OnSlotClicked;

	UPROPERTY(BlueprintReadOnly, Category = "Restraint", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UButton>> SlotButtons;

	UPROPERTY(BlueprintReadOnly, Category = "Restraint", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UImage>> SlotIcons;

	UPROPERTY(BlueprintReadOnly, Category = "Restraint", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UTextBlock>> SlotLabels;

	UPROPERTY(BlueprintReadOnly, Category = "Restraint", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UImage>> SlotLockBadges;

protected:

	void BuildLayout();
	void HandleSlotClicked(int32 SlotIndex);

	UFUNCTION()
	void HandleRestraintsChanged();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMyBondageLifeRestraintSlotClickProxy>> ClickProxies;

	FString ViewAccount;
	bool bLayoutBuilt = false;
};
