// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeWardrobeWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UTextBlock;
class UTexture2D;

/** Raised when the wardrobe panel opens or closes, so the controller can gate input. */
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

	/** Index into the height presets (0 = 娇小, 1 = 普通, 2 = 高大). */
	int32 HeightIndex = 1;
};
