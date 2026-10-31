// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeRestraintStatusWidget.generated.h"

class UImage;
class UTexture2D;

/**
 *  Top-left restraint status icons: gag / arms / legs / cannot-move.
 *
 *  Each icon is hidden unless the local character is in that state, and pulses between
 *  0.9 and 1.1 while visible.
 *
 *  Designed in /Game/UI/WBP_RestraintStatus with these names:
 *    W_StatusGag, W_StatusArm, W_StatusLeg, W_StatusCantMove  (UImage)
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeRestraintStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:

	void BuildLayout();
	void UpdateStates(float InDeltaTime);

	UPROPERTY(Transient) TObjectPtr<UImage> GagIcon;
	UPROPERTY(Transient) TObjectPtr<UImage> ArmIcon;
	UPROPERTY(Transient) TObjectPtr<UImage> LegIcon;
	UPROPERTY(Transient) TObjectPtr<UImage> CantMoveIcon;

	UPROPERTY(EditDefaultsOnly, Category = "Status") TObjectPtr<UTexture2D> GagTexture;
	UPROPERTY(EditDefaultsOnly, Category = "Status") TObjectPtr<UTexture2D> ArmTexture;
	UPROPERTY(EditDefaultsOnly, Category = "Status") TObjectPtr<UTexture2D> LegTexture;
	UPROPERTY(EditDefaultsOnly, Category = "Status") TObjectPtr<UTexture2D> CantMoveTexture;

	float PulseTime = 0.0f;
	bool bLayoutBuilt = false;
};
