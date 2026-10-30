// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeWardrobeButtonWidget.h"

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
#include "MyBondageLifePlayerController.h"

namespace
{
	constexpr float EntryIconSize = 64.0f;
	constexpr float EntryPositionX = 930.0f;
	constexpr float EntryPositionY = -30.0f;
}

TSharedRef<SWidget> UMyBondageLifeWardrobeButtonWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (WidgetTree && !bLayoutBuilt)
	{
		bLayoutBuilt = true;
		BuildLayout();
	}

	return Super::RebuildWidget();
}

void UMyBondageLifeWardrobeButtonWidget::BuildLayout()
{
	// A designed layout may already provide the button; otherwise build the default one.
	EntryButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_Entry")));

	if (!EntryButton)
	{
		UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
		if (!RootCanvas)
		{
			RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WardrobeButtonRoot"));
			WidgetTree->RootWidget = RootCanvas;
		}
		RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		EntryButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("W_Entry"));

		FButtonStyle ButtonStyle;
		ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
		ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
		ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
		ButtonStyle.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
		EntryButton->SetStyle(ButtonStyle);

		if (UCanvasPanelSlot* EntrySlot = RootCanvas->AddChildToCanvas(EntryButton))
		{
			EntrySlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
			EntrySlot->SetAlignment(FVector2D(0.0f, 1.0f));
			EntrySlot->SetPosition(FVector2D(EntryPositionX, EntryPositionY));
			EntrySlot->SetAutoSize(true);
			EntrySlot->SetZOrder(20);
		}

		UVerticalBox* EntryColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		EntryButton->AddChild(EntryColumn);

		USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		IconSize->SetWidthOverride(EntryIconSize);
		IconSize->SetHeightOverride(EntryIconSize);
		if (UVerticalBoxSlot* IconSlot = EntryColumn->AddChildToVerticalBox(IconSize))
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
		}

		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		UTexture2D* ResolvedIcon = IconTexture;
		if (!ResolvedIcon)
		{
			ResolvedIcon = LoadObject<UTexture2D>(nullptr, TEXT("/Game/2DAssest/UI/UI_Wardrobe.UI_Wardrobe"));
		}
		if (ResolvedIcon)
		{
			Icon->SetBrushFromTexture(ResolvedIcon, true);
		}
		else
		{
			// No icon configured: keep the entry transparent instead of a white block.
			Icon->SetBrush(FSlateBrush());
			Icon->SetVisibility(ESlateVisibility::Collapsed);
		}
		IconSize->AddChild(Icon);

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(EntryText));
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Label->SetJustification(ETextJustify::Center);
		Label->SetShadowOffset(FVector2D(1.0f, 1.0f));
		Label->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
		{
			FSlateFontInfo EntryFont = Label->GetFont();
			EntryFont.Size = 16;
			Label->SetFont(EntryFont);
		}
		if (UVerticalBoxSlot* LabelSlot = EntryColumn->AddChildToVerticalBox(Label))
		{
			LabelSlot->SetHorizontalAlignment(HAlign_Center);
		}
	}

	EntryButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeWardrobeButtonWidget::HandleEntryClicked);
	OnLayoutBuilt();
}

void UMyBondageLifeWardrobeButtonWidget::HandleEntryClicked()
{
	if (AMyBondageLifePlayerController* Controller = Cast<AMyBondageLifePlayerController>(GetOwningPlayer()))
	{
		Controller->ToggleWardrobe();
	}
}
