// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeHudEntryWidget.h"

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

TSharedRef<SWidget> UMyBondageLifeHudEntryWidget::RebuildWidget()
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

void UMyBondageLifeHudEntryWidget::SetEntryKind(EMyBondageLifeHudEntryAction InAction)
{
	Action = InAction;

	const bool bInfo = Action == EMyBondageLifeHudEntryAction::PlayerInfo;
	if (EntryLabel)
	{
		EntryLabel->SetText(FText::FromString(bInfo ? TEXT("\u4E2A\u4EBA\u4FE1\u606F\uFF08C\uFF09") : TEXT("\u597D\u53CB\u5217\u8868\uFF08V\uFF09")));
	}
	if (EntryIcon)
	{
		const TCHAR* IconPath = bInfo
			? TEXT("/Game/2DAssest/UI/UI_HeadIcon.UI_HeadIcon")
			: TEXT("/Game/2DAssest/UI/UI_FriendIcon.UI_FriendIcon");
		if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, IconPath))
		{
			EntryIcon->SetBrushFromTexture(Texture, true);
		}
	}
}

void UMyBondageLifeHudEntryWidget::BuildLayout()
{
	EntryButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_Entry")));
	EntryLabel = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Label")));
	EntryIcon = Cast<UImage>(WidgetTree->FindWidget(TEXT("W_Icon")));

	if (!EntryButton)
	{
		UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
		if (!Canvas)
		{
			Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HudEntryRoot"));
			WidgetTree->RootWidget = Canvas;
		}
		Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		EntryButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("W_Entry"));
		FButtonStyle Transparent;
		Transparent.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
		Transparent.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
		Transparent.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
		Transparent.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
		EntryButton->SetStyle(Transparent);
		if (UCanvasPanelSlot* EntrySlot = Canvas->AddChildToCanvas(EntryButton))
		{
			EntrySlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			EntrySlot->SetOffsets(FMargin(0.0f));
		}

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		EntryButton->AddChild(Column);

		USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		IconSize->SetWidthOverride(56.0f);
		IconSize->SetHeightOverride(56.0f);
		if (UVerticalBoxSlot* IconSlot = Column->AddChildToVerticalBox(IconSize))
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
		}

		EntryIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("W_Icon"));
		IconSize->AddChild(EntryIcon);

		EntryLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("W_Label"));
		EntryLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		EntryLabel->SetJustification(ETextJustify::Center);
		EntryLabel->SetShadowOffset(FVector2D(1.0f, 1.0f));
		EntryLabel->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
		{
			FSlateFontInfo LabelFont = EntryLabel->GetFont();
			LabelFont.Size = 16;
			EntryLabel->SetFont(LabelFont);
		}
		if (UVerticalBoxSlot* LabelSlot = Column->AddChildToVerticalBox(EntryLabel))
		{
			LabelSlot->SetHorizontalAlignment(HAlign_Center);
		}
	}

	EntryButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeHudEntryWidget::HandleClicked);
	SetEntryKind(Action);
}

void UMyBondageLifeHudEntryWidget::HandleClicked()
{
	AMyBondageLifePlayerController* Controller = Cast<AMyBondageLifePlayerController>(GetOwningPlayer());
	if (!Controller)
	{
		return;
	}

	if (Action == EMyBondageLifeHudEntryAction::PlayerInfo)
	{
		Controller->TogglePlayerInfo();
	}
	else
	{
		Controller->ToggleFriendList();
	}
}
