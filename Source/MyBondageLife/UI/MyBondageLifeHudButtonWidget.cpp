// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeHudButtonWidget.h"
#include "MyBondageLifeUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "MyBondageLife.h"
#include "Core/MyBondageLifeAccountSubsystem.h"
#include "MyBondageLifePlayerController.h"

namespace
{
	struct FButtonStyleInfo
	{
		const TCHAR* Icon;
		const TCHAR* Caption;
	};

	FButtonStyleInfo StyleForButton(EMyBondageLifeHudEntryAction Action)
	{
		switch (Action)
		{
		case EMyBondageLifeHudEntryAction::Message:
			return { TEXT("/Game/2DAssest/UI/UI_Message.UI_Message"), TEXT("\u6D88\u606F\u901A\u77E5\uFF08Q\uFF09") };
		case EMyBondageLifeHudEntryAction::FriendList:
			return { TEXT("/Game/2DAssest/UI/UI_FriendIcon.UI_FriendIcon"), TEXT("\u597D\u53CB\u5217\u8868\uFF08V\uFF09") };
		case EMyBondageLifeHudEntryAction::Wardrobe:
			return { TEXT("/Game/2DAssest/UI/UI_Wardrobe.UI_Wardrobe"), TEXT("\u8863\u67DC\uFF08B\uFF09") };
		case EMyBondageLifeHudEntryAction::Emote:
			return { TEXT("/Game/2DAssest/UI/UI_Emote.UI_Emote"), TEXT("\u8868\u60C5\u52A8\u4F5C\uFF08Z\uFF09") };
		default:
			return { TEXT("/Game/2DAssest/UI/UI_HeadIcon.UI_HeadIcon"), TEXT("\u4E2A\u4EBA\u4FE1\u606F\uFF08C\uFF09") };
		}
	}
}

TSharedRef<SWidget> UMyBondageLifeHudButtonWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (!IsDesignTime() && WidgetTree)
	{
		// Only build the fallback look when the design has no button of its own.
		EntryButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_Button")));
		if (!EntryButton)
		{
			UVerticalBox* Column = Cast<UVerticalBox>(WidgetTree->RootWidget);
			if (!Column)
			{
				Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HudButtonRoot"));
				WidgetTree->RootWidget = Column;
			}

			USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("W_IconSize"));
			IconSize->SetWidthOverride(56.0f);
			IconSize->SetHeightOverride(56.0f);
			if (UVerticalBoxSlot* SizeSlot = Column->AddChildToVerticalBox(IconSize))
			{
				SizeSlot->SetHorizontalAlignment(HAlign_Center);
			}

			UOverlay* IconOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("W_IconOverlay"));
			IconSize->AddChild(IconOverlay);

			EntryButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("W_Button"));
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
			EntryIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
			IconOverlay->AddChildToOverlay(EntryIcon);

			RedDot = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("W_RedDot"));
			RedDot->SetVisibility(ESlateVisibility::Collapsed);
			if (UOverlaySlot* DotSlot = IconOverlay->AddChildToOverlay(RedDot))
			{
				DotSlot->SetHorizontalAlignment(HAlign_Right);
				DotSlot->SetVerticalAlignment(VAlign_Top);
			}

			EntryLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("W_Label"));
			EntryLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			EntryLabel->SetJustification(ETextJustify::Center);
			EntryLabel->SetShadowOffset(FVector2D(1.0f, 1.0f));
			EntryLabel->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
			{
				FSlateFontInfo LabelFont = EntryLabel->GetFont();
				LabelFont.Size = 12;
				LabelFont.FontObject = MyBondageLifeUI::LoadUIFont();
				EntryLabel->SetFont(LabelFont);
			}
			if (UVerticalBoxSlot* LabelSlot = Column->AddChildToVerticalBox(EntryLabel))
			{
				LabelSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}
	}

	const TSharedRef<SWidget> Result = Super::RebuildWidget();

	if (!IsDesignTime() && WidgetTree && !bLayoutBuilt)
	{
		bLayoutBuilt = true;

		if (!EntryButton) { EntryButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_Button"))); }
		if (!EntryIcon) { EntryIcon = Cast<UImage>(WidgetTree->FindWidget(TEXT("W_Icon"))); }
		if (!EntryLabel) { EntryLabel = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Label"))); }
		if (!RedDot) { RedDot = Cast<UImage>(WidgetTree->FindWidget(TEXT("W_RedDot"))); }

		if (EntryIcon) { EntryIcon->SetVisibility(ESlateVisibility::HitTestInvisible); }
		if (RedDot) { RedDot->SetVisibility(ESlateVisibility::Collapsed); }

		if (EntryButton)
		{
			// 按下即触发：第一次点击不再被"抢焦点"吃掉
			EntryButton->SetClickMethod(EButtonClickMethod::MouseDown);
			EntryButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeHudButtonWidget::HandleClicked);
		}

		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		RefreshAppearance();
	}

	return Result;
}

void UMyBondageLifeHudButtonWidget::SetEntryAction(EMyBondageLifeHudEntryAction InAction)
{
	EntryAction = InAction;
	RefreshAppearance();
}

void UMyBondageLifeHudButtonWidget::RefreshAppearance()
{
	const FButtonStyleInfo Style = StyleForButton(EntryAction);

	if (EntryLabel)
	{
		EntryLabel->SetText(FText::FromString(Style.Caption));
	}

	if (EntryIcon)
	{
		if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, Style.Icon))
		{
			EntryIcon->SetBrushFromTexture(Texture, true);
			EntryIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			EntryIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UMyBondageLifeHudButtonWidget::SetRedDotVisible(bool bVisible)
{
	if (RedDot)
	{
		RedDot->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UMyBondageLifeHudButtonWidget::HandleClicked()
{
	OnClicked.Broadcast(EntryAction);
}

void UMyBondageLifeHudButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (EntryAction == EMyBondageLifeHudEntryAction::Message)
	{
		if (UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
		{
			Account->OnNotificationsChanged.RemoveDynamic(this, &UMyBondageLifeHudButtonWidget::RefreshRedDot);
			Account->OnNotificationsChanged.AddUniqueDynamic(this, &UMyBondageLifeHudButtonWidget::RefreshRedDot);
		}
	}

	RefreshRedDot();
}

void UMyBondageLifeHudButtonWidget::RefreshRedDot()
{
	if (!RedDot)
	{
		RedDot = WidgetTree ? Cast<UImage>(WidgetTree->FindWidget(TEXT("W_RedDot"))) : nullptr;
	}
	if (!RedDot)
	{
		return;
	}

	// 只有「消息通知」入口显示红点，其余入口隐藏。
	bool bShow = false;
	if (EntryAction == EMyBondageLifeHudEntryAction::Message)
	{
		if (const UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
		{
			bShow = Account->GetPendingNotificationCount() > 0;
		}
	}

	if (bShow)
	{
		if (!RedDot->GetBrush().GetResourceObject())
		{
			if (UTexture2D* DotTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/2DAssest/UI/UI_Redpoint.UI_Redpoint")))
			{
				RedDot->SetBrushFromTexture(DotTexture, true);
			}
		}
		RedDot->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		RedDot->SetVisibility(ESlateVisibility::Collapsed);
	}
}