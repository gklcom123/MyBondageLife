// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifePlayerInfoEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "MyBondageLife.h"
#include "MyBondageLifePlayerController.h"

TSharedRef<SWidget> UMyBondageLifePlayerInfoEntryWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (!IsDesignTime() && WidgetTree && !bLayoutBuilt)
	{
		bLayoutBuilt = true;
		BuildLayout();
	}

	return Super::RebuildWidget();
}

void UMyBondageLifePlayerInfoEntryWidget::BuildLayout()
{
	EntryButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_Entry")));

	if (!EntryButton)
	{
		UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
		if (!RootCanvas)
		{
			RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("InfoEntryRoot"));
			WidgetTree->RootWidget = RootCanvas;
		}
		RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		EntryButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("W_Entry"));

		FButtonStyle Transparent;
		Transparent.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
		Transparent.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
		Transparent.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
		Transparent.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
		EntryButton->SetStyle(Transparent);

		if (UCanvasPanelSlot* EntrySlot = RootCanvas->AddChildToCanvas(EntryButton))
		{
			EntrySlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
			EntrySlot->SetAlignment(FVector2D(0.0f, 1.0f));
			EntrySlot->SetPosition(FVector2D(720.0f, -30.0f));
			EntrySlot->SetAutoSize(true);
			EntrySlot->SetZOrder(30);
		}

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		EntryButton->AddChild(Column);

		USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		IconSize->SetWidthOverride(64.0f);
		IconSize->SetHeightOverride(64.0f);
		if (UVerticalBoxSlot* IconSlot = Column->AddChildToVerticalBox(IconSize))
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
		}

		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		UTexture2D* ResolvedIcon = IconTexture;
		if (!ResolvedIcon)
		{
			ResolvedIcon = LoadObject<UTexture2D>(nullptr, TEXT("/Game/2DAssest/UI/UI_HeadIcon.UI_HeadIcon"));
		}
		if (ResolvedIcon)
		{
			Icon->SetBrushFromTexture(ResolvedIcon, true);
		}
		else
		{
			Icon->SetVisibility(ESlateVisibility::Collapsed);
		}
		IconSize->AddChild(Icon);

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(TEXT("\u4E2A\u4EBA\u4FE1\u606F\uFF08C\uFF09")));
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Label->SetJustification(ETextJustify::Center);
		Label->SetShadowOffset(FVector2D(1.0f, 1.0f));
		Label->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
		{
			FSlateFontInfo LabelFont = Label->GetFont();
			LabelFont.Size = 16;
			Label->SetFont(LabelFont);
		}
		if (UVerticalBoxSlot* LabelSlot = Column->AddChildToVerticalBox(Label))
		{
			LabelSlot->SetHorizontalAlignment(HAlign_Center);
		}
	}

	EntryButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifePlayerInfoEntryWidget::HandleEntryClicked);
	OnLayoutBuilt();
}

void UMyBondageLifePlayerInfoEntryWidget::HandleEntryClicked()
{
	if (AMyBondageLifePlayerController* Controller = Cast<AMyBondageLifePlayerController>(GetOwningPlayer()))
	{
		Controller->TogglePlayerInfo();
	}
}
