// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeSitHintWidget.h"
#include "MyBondageLifeUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

TSharedRef<SWidget> UMyBondageLifeSitHintWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (!IsDesignTime() && WidgetTree && !bLayoutBuilt)
	{
		bLayoutBuilt = true;

		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SitHintRoot"));
		WidgetTree->RootWidget = Canvas;
		Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);

		HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("W_SitHint_Text"));
		HintText->SetText(FText::FromString(TEXT("\u6309[G]\u5750\u4E0B")));
		HintText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.95f, 0.6f)));
		{
			FSlateFontInfo Font = HintText->GetFont();
			Font.Size = 22;
			Font.FontObject = MyBondageLifeUI::LoadUIFont();
			HintText->SetFont(Font);
		}

		if (UCanvasPanelSlot* TextSlot = Canvas->AddChildToCanvas(HintText))
		{
			TextSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
			TextSlot->SetAlignment(FVector2D(0.0f, 0.0f));
			TextSlot->SetAutoSize(true);
		}
	}

	return Super::RebuildWidget();
}

void UMyBondageLifeSitHintWidget::SetHintText(const FString& InText)
{
	if (HintText)
	{
		HintText->SetText(FText::FromString(InText));
	}
}
