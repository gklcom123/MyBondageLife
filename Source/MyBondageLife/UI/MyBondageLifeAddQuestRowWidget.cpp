// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeAddQuestRowWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "MyBondageLifePlayerController.h"
#include "UI/MyBondageLifeUIStyle.h"

TSharedRef<SWidget> UMyBondageLifeAddQuestRowWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RowRoot"));
		Root->SetBrushColor(FLinearColor(0.97f, 0.97f, 0.97f, 1.0f));
		Root->SetPadding(FMargin(12.0f, 6.0f));
		WidgetTree->RootWidget = Root;

		UHorizontalBox* Box = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RowBox"));
		Root->SetContent(Box);

		UTextBlock* Info = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("W_QuestInfo"));
		Info->SetText(FText::FromString(InfoText));
		Info->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
		Info->SetFont(MyBondageLifeUI::Font(18));
		if (UHorizontalBoxSlot* BoxSlot = Box->AddChildToHorizontalBox(Info))
		{
			BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			BoxSlot->SetVerticalAlignment(VAlign_Center);
		}

		UTextBlock* Reward = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("W_Reward"));
		Reward->SetText(FText::FromString(RewardText));
		Reward->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
		Reward->SetFont(MyBondageLifeUI::Font(18));
		if (UHorizontalBoxSlot* BoxSlot = Box->AddChildToHorizontalBox(Reward))
		{
			BoxSlot->SetPadding(FMargin(12.0f, 0.0f));
			BoxSlot->SetVerticalAlignment(VAlign_Center);
		}

		AcceptButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("W_Accept"));
		AcceptButton->SetIsEnabled(bActionEnabled);
		AcceptButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeAddQuestRowWidget::HandleAcceptClicked);

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("W_Accept_Label"));
		Label->SetText(FText::FromString(ActionText));
		Label->SetJustification(ETextJustify::Center);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.1f, 0.1f, 0.1f)));
		Label->SetFont(MyBondageLifeUI::Font(18));
		AcceptButton->AddChild(Label);

		if (UHorizontalBoxSlot* BoxSlot = Box->AddChildToHorizontalBox(AcceptButton))
		{
			BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			BoxSlot->SetVerticalAlignment(VAlign_Center);
			BoxSlot->SetPadding(FMargin(8.0f, 0.0f));
		}
	}

	return Super::RebuildWidget();
}

void UMyBondageLifeAddQuestRowWidget::HandleAcceptClicked()
{
	if (Proxy && Proxy->Owner)
	{
		Proxy->Owner->AcceptQuestRow(Proxy);
	}
}
