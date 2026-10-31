// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeHudEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/MyBondageLifeAccountSubsystem.h"
#include "Engine/Texture2D.h"
#include "MyBondageLifePlayerController.h"

namespace
{
	struct FEntryStyle
	{
		const TCHAR* Icon;
		const TCHAR* Caption;
	};

	FEntryStyle StyleFor(EMyBondageLifeHudEntryAction Action)
	{
		switch (Action)
		{
		case EMyBondageLifeHudEntryAction::Message:
			return { TEXT("/Game/2DAssest/UI/UI_Message.UI_Message"), TEXT("\u6D88\u606F\u901A\u77E5\uFF08Q\uFF09") };
		case EMyBondageLifeHudEntryAction::FriendList:
			return { TEXT("/Game/2DAssest/UI/UI_FriendIcon.UI_FriendIcon"), TEXT("\u597D\u53CB\u5217\u8868\uFF08V\uFF09") };
		case EMyBondageLifeHudEntryAction::Wardrobe:
			return { TEXT("/Game/2DAssest/UI/UI_Wardrobe.UI_Wardrobe"), TEXT("\u8863\u67DC\uFF08B\uFF09") };
		default:
			return { TEXT("/Game/2DAssest/UI/UI_HeadIcon.UI_HeadIcon"), TEXT("\u4E2A\u4EBA\u4FE1\u606F\uFF08C\uFF09") };
		}
	}
}

TSharedRef<SWidget> UMyBondageLifeHudEntryWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	TSharedRef<SWidget> BuiltWidget = Super::RebuildWidget();

	if (!IsDesignTime() && WidgetTree && !bLayoutBuilt)
	{
		bLayoutBuilt = true;
		BuildLayout();
	}

	return BuiltWidget;
}

void UMyBondageLifeHudEntryWidget::BuildLayout()
{
	EntryButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_Entry")));
	EntryIcon = Cast<UImage>(WidgetTree->FindWidget(TEXT("W_Icon")));
	EntryLabel = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Label")));
	RedDot = Cast<UImage>(WidgetTree->FindWidget(TEXT("W_RedDot")));

	if (!EntryButton)
	{
		// One template for every entry: a shrink-wrapped column of icon + caption, so the
		// widget's own size is exactly its content (never a full-screen blank box).
		UVerticalBox* Column = Cast<UVerticalBox>(WidgetTree->RootWidget);
		if (!Column)
		{
			Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HudEntryRoot"));
			WidgetTree->RootWidget = Column;
		}

		USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("W_Entry"));
		IconSize->SetWidthOverride(56.0f);
		IconSize->SetHeightOverride(56.0f);
		if (UVerticalBoxSlot* IconSlot = Column->AddChildToVerticalBox(IconSize))
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
		}

		UOverlay* IconOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		IconSize->AddChild(IconOverlay);

		EntryButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		{
			FButtonStyle Transparent;
			Transparent.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
			Transparent.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
			Transparent.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
			Transparent.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
			EntryButton->SetStyle(Transparent);
		}
		IconOverlay->AddChildToOverlay(EntryButton);

		EntryIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("W_Icon"));
		IconOverlay->AddChildToOverlay(EntryIcon);

		RedDot = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("W_RedDot"));
		if (UOverlaySlot* DotSlot = IconOverlay->AddChildToOverlay(RedDot))
		{
			DotSlot->SetHorizontalAlignment(HAlign_Right);
			DotSlot->SetVerticalAlignment(VAlign_Top);
		}
		RedDot->SetVisibility(ESlateVisibility::Collapsed);

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

	if (UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr)
	{
		Account->OnNotificationsChanged.AddUniqueDynamic(this, &UMyBondageLifeHudEntryWidget::HandleNotificationsChanged);
	}

	RefreshAppearance();
	HandleNotificationsChanged();
}

void UMyBondageLifeHudEntryWidget::SetEntryKind(EMyBondageLifeHudEntryAction InAction)
{
	Action = InAction;
	RefreshAppearance();
	HandleNotificationsChanged();
}

void UMyBondageLifeHudEntryWidget::RefreshAppearance()
{
	const FEntryStyle Style = StyleFor(Action);

	if (EntryLabel)
	{
		EntryLabel->SetText(FText::FromString(Style.Caption));
	}

	if (EntryIcon)
	{
		if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, Style.Icon))
		{
			EntryIcon->SetBrushFromTexture(Texture, true);
		}
	}

	if (RedDot)
	{
		if (UTexture2D* DotTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/2DAssest/UI/UI_Redpoint.UI_Redpoint")))
		{
			RedDot->SetBrushFromTexture(DotTexture, true);
		}
	}
}

void UMyBondageLifeHudEntryWidget::SetRedDotVisible(bool bVisible)
{
	if (RedDot)
	{
		RedDot->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UMyBondageLifeHudEntryWidget::HandleNotificationsChanged()
{
	if (Action != EMyBondageLifeHudEntryAction::Message)
	{
		return;
	}

	int32 Count = 0;
	if (const UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr)
	{
		Count = Account->GetPendingNotificationCount();
	}

	SetRedDotVisible(Count > 0);
}

void UMyBondageLifeHudEntryWidget::HandleClicked()
{
	AMyBondageLifePlayerController* Controller = Cast<AMyBondageLifePlayerController>(GetOwningPlayer());
	if (!Controller)
	{
		return;
	}

	switch (Action)
	{
	case EMyBondageLifeHudEntryAction::Message:
		Controller->ToggleMessageCenter();
		break;
	case EMyBondageLifeHudEntryAction::FriendList:
		Controller->ToggleFriendList();
		break;
	case EMyBondageLifeHudEntryAction::Wardrobe:
		Controller->ToggleWardrobe();
		break;
	default:
		Controller->TogglePlayerInfo();
		break;
	}
}
