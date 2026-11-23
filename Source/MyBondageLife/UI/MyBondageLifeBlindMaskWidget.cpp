// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeBlindMaskWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

TSharedRef<SWidget> UMyBondageLifeBlindMaskWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		MaskImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("W_BlindMask"));
		WidgetTree->RootWidget = MaskImage;

		MaskImage->SetVisibility(ESlateVisibility::HitTestInvisible);

		if (UMaterialInterface* MaskMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Effects/M_BlindMaskUI.M_BlindMaskUI")))
		{
			MaskMID = UMaterialInstanceDynamic::Create(MaskMaterial, this);
			MaskImage->SetBrushFromMaterial(MaskMID);
			MaskImage->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
		}
	}

	return Super::RebuildWidget();
}

void UMyBondageLifeBlindMaskWidget::SetStrength(float InStrength)
{
	if (MaskMID)
	{
		MaskMID->SetScalarParameterValue(TEXT("Strength"), FMath::Clamp(InStrength, 0.0f, 1.0f));
	}
}
