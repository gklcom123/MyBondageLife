// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeQuestPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "UI/MyBondageLifeUIStyle.h"

TSharedRef<SWidget> UMyBondageLifeQuestPanelWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("QuestRoot"));
		WidgetTree->RootWidget = Root;

		UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("QuestBackground"));
		Background->SetBrushColor(FLinearColor(0.03f, 0.03f, 0.05f, 0.94f));
		if (UCanvasPanelSlot* BgSlot = Root->AddChildToCanvas(Background))
		{
			BgSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			BgSlot->SetOffsets(FMargin(0.0f));
		}

		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestTitle"));
		Title->SetText(FText::FromString(TEXT("任务")));
		Title->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		{
			FSlateFontInfo Font = MyBondageLifeUI::Font(28);
			Title->SetFont(Font);
		}
		if (UCanvasPanelSlot* TitleSlot = Root->AddChildToCanvas(Title))
		{
			TitleSlot->SetAnchors(FAnchors(0.5f, 0.08f, 0.5f, 0.08f));
			TitleSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			TitleSlot->SetAutoSize(true);
		}

		CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("W_Close"));
		CloseButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeQuestPanelWidget::HandleCloseClicked);

		UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestCloseLabel"));
		CloseLabel->SetText(FText::FromString(TEXT("关闭")));
		CloseLabel->SetJustification(ETextJustify::Center);
		CloseLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.1f, 0.1f, 0.1f)));
		CloseLabel->SetFont(MyBondageLifeUI::Font(20));
		CloseButton->AddChild(CloseLabel);

		if (UCanvasPanelSlot* CloseSlot = Root->AddChildToCanvas(CloseButton))
		{
			CloseSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
			CloseSlot->SetAlignment(FVector2D(1.0f, 0.0f));
			CloseSlot->SetPosition(FVector2D(-40.0f, 40.0f));
			CloseSlot->SetSize(FVector2D(140.0f, 52.0f));
		}
	}

	return Super::RebuildWidget();
}

void UMyBondageLifeQuestPanelWidget::HandleCloseClicked()
{
	OnClosed.Broadcast();
}
