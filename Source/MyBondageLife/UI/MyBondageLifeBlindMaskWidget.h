// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeBlindMaskWidget.generated.h"

class UImage;

/** 蒙眼时的屏幕黑色蒙版（UI 层，不可点击）。 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeBlindMaskWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** 0..1 的蒙版强度。 */
	void SetStrength(float InStrength);

protected:
	UPROPERTY(Transient) TObjectPtr<UImage> MaskImage;
	UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> MaskMID;
};
