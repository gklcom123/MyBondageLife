// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeMessageWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Core/MyBondageLifeAccountSubsystem.h"
#include "Engine/Texture2D.h"
#include "MyBondageLife.h"
#include "MyBondageLifePlayerController.h"

namespace
{
	UButton* FindButtonIn(UWidget* Root)
	{
		if (!Root)
		{
			return nullptr;
		}
		if (UButton* AsButton = Cast<UButton>(Root))
		{
			return AsButton;
		}
		if (UPanelWidget* Panel = Cast<UPanelWidget>(Root))
		{
			for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
			{
				if (UButton* Found = FindButtonIn(Panel->GetChildAt(Index)))
				{
					return Found;
				}
			}
		}
		return nullptr;
	}
}

// ---------------------------------------------------------------------------
// Entry button
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UMyBondageLifeMessageButtonWidget::RebuildWidget()
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

void UMyBondageLifeMessageButtonWidget::BuildLayout()
{
	EntryButton = FindButtonIn(WidgetTree->FindWidget(TEXT("W_Entry")));
	RedDot = WidgetTree->FindWidget(TEXT("W_RedDot"));

	if (!EntryButton)
	{
		UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
		if (!Canvas)
		{
			Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MessageEntryRoot"));
			WidgetTree->RootWidget = Canvas;
		}
		Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("W_Entry"));
		Size->SetWidthOverride(88.0f);
		Size->SetHeightOverride(88.0f);
		if (UCanvasPanelSlot* SizeSlot = Canvas->AddChildToCanvas(Size))
		{
			// Bottom-left, just above the chat box (mirrors the other HUD entries).
			SizeSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
			SizeSlot->SetAlignment(FVector2D(0.0f, 1.0f));
			SizeSlot->SetPosition(FVector2D(40.0f, -300.0f));
			SizeSlot->SetAutoSize(true);
			SizeSlot->SetZOrder(20);
		}

		EntryButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		if (UTexture2D* Icon = LoadObject<UTexture2D>(nullptr, TEXT("/Game/2DAssest/UI/UI_Message.UI_Message")))
		{
			FButtonStyle Style;
			Style.Normal.SetResourceObject(Icon);
			Style.Normal.ImageSize = FVector2D(88.0f, 88.0f);
			Style.Hovered = Style.Normal;
			Style.Pressed = Style.Normal;
			EntryButton->SetStyle(Style);
		}
		Size->AddChild(EntryButton);

		if (UTexture2D* DotTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/2DAssest/UI/UI_Redpoint.UI_Redpoint")))
		{
			UImage* Dot = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("W_RedDot"));
			Dot->SetBrushFromTexture(DotTexture, true);
			if (UCanvasPanelSlot* DotSlot = Canvas->AddChildToCanvas(Dot))
			{
				DotSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
				DotSlot->SetAlignment(FVector2D(0.0f, 1.0f));
				DotSlot->SetPosition(FVector2D(96.0f, -244.0f));
				DotSlot->SetSize(FVector2D(24.0f, 24.0f));
				DotSlot->SetZOrder(21);
			}
			RedDot = Dot;
		}
	}

	if (EntryButton)
	{
		EntryButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeMessageButtonWidget::HandleClicked);
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			Account->OnNotificationsChanged.AddUniqueDynamic(this, &UMyBondageLifeMessageButtonWidget::HandleNotificationsChanged);
		}
	}

	HandleNotificationsChanged();
}

void UMyBondageLifeMessageButtonWidget::HandleClicked()
{
	if (AMyBondageLifePlayerController* Controller = Cast<AMyBondageLifePlayerController>(GetOwningPlayer()))
	{
		Controller->ToggleMessageCenter();
	}
}

void UMyBondageLifeMessageButtonWidget::HandleNotificationsChanged()
{
	int32 Count = 0;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			Count = Account->GetPendingNotificationCount();
		}
	}

	if (RedDot)
	{
		RedDot->SetVisibility(Count > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

// ---------------------------------------------------------------------------
// Row
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UMyBondageLifeMessageRowWidget::RebuildWidget()
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

void UMyBondageLifeMessageRowWidget::BuildLayout()
{
	MessageText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Text")));
	AcceptButton = FindButtonIn(WidgetTree->FindWidget(TEXT("W_Accept")));
	DeclineButton = FindButtonIn(WidgetTree->FindWidget(TEXT("W_Decline")));

	if (IsValid(AcceptButton))
	{
		AcceptButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeMessageRowWidget::HandleAccept);
	}
	if (IsValid(DeclineButton))
	{
		DeclineButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeMessageRowWidget::HandleDecline);
	}
}

void UMyBondageLifeMessageRowWidget::SetNotification(const FString& InNotificationId, const FString& InText, bool bInContractKind)
{
	NotificationId = InNotificationId;
	if (MessageText)
	{
		MessageText->SetText(FText::FromString(InText));
	}
}

void UMyBondageLifeMessageRowWidget::HandleAccept()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			Account->ResolveNotification(NotificationId, true);
		}
	}
}

void UMyBondageLifeMessageRowWidget::HandleDecline()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			Account->ResolveNotification(NotificationId, false);
		}
	}
}

// ---------------------------------------------------------------------------
// Center
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UMyBondageLifeMessageCenterWidget::RebuildWidget()
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

void UMyBondageLifeMessageCenterWidget::BuildLayout()
{
	RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	PanelBorder = Cast<UBorder>(WidgetTree->FindWidget(TEXT("W_Panel")));
	ListBox = Cast<UScrollBox>(WidgetTree->FindWidget(TEXT("W_List")));
	RowTemplate = Cast<UMyBondageLifeMessageRowWidget>(WidgetTree->FindWidget(TEXT("W_RowTemplate")));

	if (RowTemplate)
	{
		RowClass = RowTemplate->GetClass();
		RowTemplate->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (PanelBorder)
	{
		if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(PanelBorder->Slot))
		{
			PanelSlot->SetZOrder(20);
		}
		PanelBorder->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UMyBondageLifeAccountSubsystem* Account = GameInstance->GetSubsystem<UMyBondageLifeAccountSubsystem>())
		{
			Account->OnNotificationsChanged.AddUniqueDynamic(this, &UMyBondageLifeMessageCenterWidget::HandleNotificationsChanged);
		}
	}
}

void UMyBondageLifeMessageCenterWidget::RefreshList()
{
	if (!ListBox)
	{
		return;
	}

	ListBox->ClearChildren();

	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	if (!Account || !RowClass)
	{
		return;
	}

	for (const FMyBondageLifeNotification& Item : Account->GetNotifications())
	{
		UMyBondageLifeMessageRowWidget* Row = CreateWidget<UMyBondageLifeMessageRowWidget>(GetOwningPlayer(), RowClass);
		if (!Row)
		{
			continue;
		}

		if (UScrollBoxSlot* RowSlot = Cast<UScrollBoxSlot>(ListBox->AddChild(Row)))
		{
			RowSlot->SetPadding(FMargin(0.0f, 4.0f));
		}

		FString Text;
		switch (Item.Type)
		{
		case EMyBondageLifeNotificationType::FriendRequest:
			Text = FString::Printf(TEXT("\u6536\u5230\u6765\u81EA\u3010%s\u3011\u7684\u597D\u53CB\u8BF7\u6C42\uFF0C\u662F\u5426\u540C\u610F"), *Item.CharacterName);
			break;
		case EMyBondageLifeNotificationType::ContractTrial:
			Text = FString::Printf(TEXT("\u6536\u5230\u6765\u81EA\u3010%s\u3011\u7684\u7B7E\u8BA2\u4E34\u65F6\u5951\u7EA6\u8BF7\u6C42\uFF0C\u662F\u5426\u540C\u610F"), *Item.CharacterName);
			break;
		default:
			Text = FString::Printf(TEXT("\u6536\u5230\u6765\u81EA\u3010%s\u3011\u7684\u7B7E\u8BA2\u5951\u7EA6\u8BF7\u6C42\uFF0C\u662F\u5426\u540C\u610F"), *Item.CharacterName);
			break;
		}

		Row->SetNotification(Item.Id, Text, Item.Type != EMyBondageLifeNotificationType::FriendRequest);
	}
}

void UMyBondageLifeMessageCenterWidget::HandleNotificationsChanged()
{
	RefreshList();
}

void UMyBondageLifeMessageCenterWidget::HandleCloseClicked()
{
	CloseCenter();
}

void UMyBondageLifeMessageCenterWidget::OpenCenter()
{
	SetPanelOpen(true);
}

void UMyBondageLifeMessageCenterWidget::CloseCenter()
{
	SetPanelOpen(false);
}

void UMyBondageLifeMessageCenterWidget::SetPanelOpen(bool bOpen)
{
	if (IsPanelOpen() == bOpen)
	{
		return;
	}

	if (bOpen)
	{
		if (UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
			: nullptr)
		{
			Account->RefreshNotifications();
		}
		RefreshList();
	}

	if (PanelBorder)
	{
		PanelBorder->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

}

void UMyBondageLifeMessageCenterWidget::TogglePanel()
{
	SetPanelOpen(!IsPanelOpen());
}

bool UMyBondageLifeMessageCenterWidget::IsPanelOpen() const
{
	return PanelBorder != nullptr && PanelBorder->GetVisibility() == ESlateVisibility::Visible;
}
