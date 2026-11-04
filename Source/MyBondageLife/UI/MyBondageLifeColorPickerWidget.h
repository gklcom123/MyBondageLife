// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeColorPickerWidget.generated.h"

class UBorder;
class UButton;
class UEditableTextBox;
class UImage;
class USlider;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMyBondageLifeColorCommitted, int32, Slot, int32, Channel, const FString&, HexColor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMyBondageLifeColorCancelled);

/**
 *  HSV colour picker used by outfit/restraint tinting.
 *  Adopts (all optional): W_ColorPicker_Hue / _Saturation / _Value (sliders),
 *  W_ColorPicker_Preview (image), W_ColorPicker_Hex (text box),
 *  W_ColorPicker_Confirm / _Cancel / _Close (buttons), W_ColorPicker_Root (border).
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeColorPickerWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Opens the picker for one slot/channel, starting from HexColor (may be empty). */
	UFUNCTION(BlueprintCallable, Category = "Color")
	void OpenForColor(int32 InSlot, int32 InChannel, const FString& HexColor);

	UFUNCTION(BlueprintCallable, Category = "Color")
	void ClosePicker();

	UPROPERTY(BlueprintAssignable, Category = "Color")
	FMyBondageLifeColorCommitted OnColorCommitted;

	UPROPERTY(BlueprintAssignable, Category = "Color")
	FMyBondageLifeColorCancelled OnColorCancelled;

protected:

	void BuildLayout();
	void RefreshPreview();

	UFUNCTION() void HandleHueChanged(float Value);
	UFUNCTION() void HandleSaturationChanged(float Value);
	UFUNCTION() void HandleValueChanged(float Value);
	UFUNCTION() void HandleHexCommitted(const FText& Text, ETextCommit::Type CommitType);
	UFUNCTION() void HandleHexChanged(const FText& Text);

	bool bUpdatingHex = false;
	UFUNCTION() void HandleConfirmClicked();
	UFUNCTION() void HandleCancelClicked();
	UFUNCTION() void HandleCloseClicked();

	int32 PickerSlot = 0;
	int32 PickerChannel = 1;
	float Hue = 0.0f;
	float Saturation = 1.0f;
	float Brightness = 1.0f;

	UPROPERTY(Transient) TObjectPtr<UBorder> RootPanel;
	UPROPERTY(Transient) TObjectPtr<USlider> HueSlider;
	UPROPERTY(Transient) TObjectPtr<USlider> SaturationSlider;
	UPROPERTY(Transient) TObjectPtr<USlider> ValueSlider;
	UPROPERTY(Transient) TObjectPtr<UImage> PreviewImage;
	UPROPERTY(Transient) TObjectPtr<UEditableTextBox> HexBox;
	UPROPERTY(Transient) TObjectPtr<UButton> ConfirmButton;
	UPROPERTY(Transient) TObjectPtr<UButton> CancelButton;
	UPROPERTY(Transient) TObjectPtr<UButton> CloseButton;

	bool bLayoutBuilt = false;
};
