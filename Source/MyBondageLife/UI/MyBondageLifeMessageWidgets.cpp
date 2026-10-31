// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeMessageWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/MyBondageLifeAccountSubsystem.h"
#include "Engine/Texture2D.h"
#include "MyBondageLife.h"
#include "MyBondageLifePlayerController.h"
#include "GameFramework/PlayerController.h"

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
		Size->SetWidthOverride(96.0f);
		Size->SetHeightOverride(96.0f);
		if (UCanvasPanelSlot* SizeSlot = Canvas->AddChildToCanvas(Size))
		{
			// Bottom-left, above the chat box (same pattern as the other HUD entries).
			SizeSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
			SizeSlot->SetAlignment(FVector2D(0.0f, 1.0f));
			SizeSlot->SetPosition(FVector2D(40.0f, -340.0f));
			SizeSlot->SetAutoSize(true);
			SizeSlot->SetZOrder(20);
		}

		EntryButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		{
			FButtonStyle Transparent;
			Transparent.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
			Transparent.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
			Transparent.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
			Transparent.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
			EntryButton->SetStyle(Transparent);
		}
		Size->AddChild(EntryButton);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		EntryButton->AddChild(Column);

		USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		IconSize->SetWidthOverride(64.0f);
		IconSize->SetHeightOverride(64.0f);
		if (UVerticalBoxSlot* IconSlot = Column->AddChildToVerticalBox(IconSize))
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
		}

		UOverlay* IconOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		IconSize->AddChild(IconOverlay);

		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		if (UTexture2D* IconTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/2DAssest/UI/UI_Message.UI_Message")))
		{
			Icon->SetBrushFromTexture(IconTexture, true);
		}
		IconOverlay->AddChildToOverlay(Icon);

		// Red dot sits on the icon's top-right corner, whatever the layout does.
		if (UTexture2D* DotTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/2DAssest/UI/UI_Redpoint.UI_Redpoint")))
		{
			UImage* Dot = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("W_RedDot"));
			Dot->SetBrushFromTexture(DotTexture, true);
			if (UOverlaySlot* DotSlot = IconOverlay->AddChildToOverlay(Dot))
			{
				DotSlot->SetHorizontalAlignment(HAlign_Right);
				DotSlot->SetVerticalAlignment(VAlign_Top);
				DotSlot->SetPadding(FMargin(0.0f, -4.0f, -4.0f, 0.0f));
			}
			RedDot = Dot;
		}

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(TEXT("\u6D88\u606F\u901A\u77E5\uFF08Q\uFF09")));
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
	if (!MessageText)
	{
		// Positional fallback: the design may leave the label unnamed (any depth).
		TArray<UWidget*> Stack;
		if (WidgetTree->RootWidget)
		{
			Stack.Add(WidgetTree->RootWidget);
		}
		while (Stack.Num() > 0 && !MessageText)
		{
			UWidget* Current = Stack.Pop();
			if (UTextBlock* AsText = Cast<UTextBlock>(Current))
			{
				MessageText = AsText;
				break;
			}
			if (UPanelWidget* Panel = Cast<UPanelWidget>(Current))
			{
				for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
				{
					if (UWidget* Child = Panel->GetChildAt(Index))
					{
						Stack.Add(Child);
					}
				}
			}
		}
	}
	AcceptButton = FindButtonIn(WidgetTree->FindWidget(TEXT("W_Accept")));
	DeclineButton = FindButtonIn(WidgetTree->FindWidget(TEXT("W_Decline")));

	if (IsValid(AcceptButton))
	{
		AcceptButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeMessageRowWidget::HandleAccept);
	}
	if (bHasPendingMessage && MessageText)
	{
		MessageText->SetText(FText::FromString(PendingMessageText));
		bHasPendingMessage = false;
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
	// The prototype row is a designer-only sample: detach it so it can never appear in the
	// panel (setting it Collapsed was undone by widget property initialisation).
	if (UWidget* Prototype = WidgetTree->FindWidget(TEXT("W_RowTemplate")))
	{
		if (UPanelWidget* PrototypeParent = Prototype->GetParent())
		{
			PrototypeParent->RemoveChild(Prototype);
		}
		else
		{
			Prototype->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	PanelBorder = Cast<UBorder>(WidgetTree->FindWidget(TEXT("W_Panel")));
	if (!PanelBorder)
	{
		// Fallback: first Border in the tree (the designer may have named it differently).
		TArray<UWidget*> Stack;
		if (WidgetTree->RootWidget)
		{
			Stack.Add(WidgetTree->RootWidget);
		}
		while (Stack.Num() > 0 && !PanelBorder)
		{
			UWidget* Current = Stack.Pop();
			if (UBorder* AsBorder = Cast<UBorder>(Current))
			{
				PanelBorder = AsBorder;
				break;
			}
			if (UPanelWidget* Panel = Cast<UPanelWidget>(Current))
			{
				for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
				{
					if (UWidget* Child = Panel->GetChildAt(Index))
					{
						Stack.Add(Child);
					}
				}
			}
		}
	}
	ListBox = Cast<UScrollBox>(WidgetTree->FindWidget(TEXT("W_List")));
	if (!ListBox)
	{
		TArray<UWidget*> BoxStack;
		if (WidgetTree->RootWidget)
		{
			BoxStack.Add(WidgetTree->RootWidget);
		}
		while (BoxStack.Num() > 0 && !ListBox)
		{
			UWidget* Current = BoxStack.Pop();
			if (UScrollBox* AsBox = Cast<UScrollBox>(Current))
			{
				ListBox = AsBox;
				break;
			}
			if (UPanelWidget* Panel = Cast<UPanelWidget>(Current))
			{
				for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
				{
					if (UWidget* Child = Panel->GetChildAt(Index))
					{
						BoxStack.Add(Child);
					}
				}
			}
		}
	}
	// Every message-row instance in the asset is a prototype: hide all of them and use the
	// first one's class to build the real list (a stray sample row used to stay visible).
	{
		TArray<UWidget*> Stack;
		if (WidgetTree->RootWidget)
		{
			Stack.Add(WidgetTree->RootWidget);
		}
		while (Stack.Num() > 0)
		{
			UWidget* Current = Stack.Pop();
			if (!Current)
			{
				continue;
			}
			if (UMyBondageLifeMessageRowWidget* AsRow = Cast<UMyBondageLifeMessageRowWidget>(Current))
			{
				if (!RowTemplate)
				{
					RowTemplate = AsRow;
					RowClass = AsRow->GetClass();
				}
				AsRow->SetVisibility(ESlateVisibility::Collapsed);
				continue;
			}
			if (UPanelWidget* Panel = Cast<UPanelWidget>(Current))
			{
				for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
				{
					if (UWidget* Child = Panel->GetChildAt(Index))
					{
						Stack.Add(Child);
					}
				}
			}
		}
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
	RowProxies.Reset();
	HidePrototypeRows();


	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	if (!Account || !RowClass)
	{
		return;
	}

	for (const FMyBondageLifeNotification& Item : Account->GetNotifications())
	{
		FString RowText;
		switch (Item.Type)
		{
		case EMyBondageLifeNotificationType::FriendRequest:
			RowText = FString::Printf(TEXT("\u6536\u5230\u6765\u81EA\u3010%s\u3011\u7684\u597D\u53CB\u8BF7\u6C42\uFF0C\u662F\u5426\u540C\u610F"), *Item.CharacterName);
			break;
		case EMyBondageLifeNotificationType::ContractTrial:
			RowText = FString::Printf(TEXT("\u6536\u5230\u6765\u81EA\u3010%s\u3011\u7684\u7B7E\u8BA2\u4E34\u65F6\u5951\u7EA6\u8BF7\u6C42\uFF0C\u662F\u5426\u540C\u610F"), *Item.CharacterName);
			break;
		default:
			RowText = FString::Printf(TEXT("\u6536\u5230\u6765\u81EA\u3010%s\u3011\u7684\u7B7E\u8BA2\u5951\u7EA6\u8BF7\u6C42\uFF0C\u662F\u5426\u540C\u610F"), *Item.CharacterName);
			break;
		}

		// Rows are built in code: the designed row prototype could not be resolved
		// reliably (its labels are unnamed), and a broken list is worse than a plain one.
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Row->SetVisibility(ESlateVisibility::Visible);

		UBorder* RowBack = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		RowBack->SetBrushColor(FLinearColor(0.92f, 0.92f, 0.94f, 1.0f));
		RowBack->SetPadding(FMargin(12.0f, 6.0f));
		Row->AddChild(RowBack);
		RowBack->AddChild(WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass()));

		UHorizontalBox* Content = Cast<UHorizontalBox>(RowBack->GetContent());
		if (!Content)
		{
			Content = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			RowBack->AddChild(Content);
		}

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(RowText));
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
		if (UHorizontalBoxSlot* LabelSlot = Content->AddChildToHorizontalBox(Label))
		{
			LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}

		auto MakeButton = [this, Content, &Item](const FString& Caption, bool bAccept)
		{
			UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
			UTextBlock* Caption2 = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Caption2->SetText(FText::FromString(Caption));
			Caption2->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f)));
			Button->AddChild(Caption2);

			if (UHorizontalBoxSlot* ButtonSlot = Content->AddChildToHorizontalBox(Button))
			{
				ButtonSlot->SetPadding(FMargin(8.0f, 0.0f));
				ButtonSlot->SetVerticalAlignment(VAlign_Center);
			}

			if (UMyBondageLifeAccountSubsystem* Subsystem = GetGameInstance()
				? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
				: nullptr)
			{
				UMyBondageLifeMessageButtonProxy* Proxy = NewObject<UMyBondageLifeMessageButtonProxy>(this);
				Proxy->Subsystem = Subsystem;
				Proxy->NotificationId = Item.Id;
				Proxy->bAccept = bAccept;
				RowProxies.Add(Proxy);
				Button->OnClicked.AddDynamic(Proxy, &UMyBondageLifeMessageButtonProxy::HandleClicked);
			}
		};

		MakeButton(TEXT("\u540C\u610F"), true);
		MakeButton(TEXT("\u62D2\u7EDD"), false);

		if (UScrollBoxSlot* RowSlot = Cast<UScrollBoxSlot>(ListBox->AddChild(Row)))
		{
			RowSlot->SetPadding(FMargin(0.0f, 4.0f));
		}
		continue;
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

	// The controller owns the input mode: just report the new state.
	if (AMyBondageLifePlayerController* OwningController = Cast<AMyBondageLifePlayerController>(GetOwningPlayer()))
	{
		OwningController->SetMessageCenterOpen(bOpen);
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

void UMyBondageLifeMessageButtonProxy::HandleClicked()
{
	if (Subsystem && !NotificationId.IsEmpty())
	{
		Subsystem->ResolveNotification(NotificationId, bAccept);
	}
}

void UMyBondageLifeMessageCenterWidget::HidePrototypeRows()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		return;
	}

	// Any message-row instance still sitting in the tree is a prototype (the list is built
	// in code), so keep all of them out of sight every time the list refreshes.
	TArray<UWidget*> Stack;
	Stack.Add(WidgetTree->RootWidget);
	while (Stack.Num() > 0)
	{
		UWidget* Current = Stack.Pop();
		if (!Current)
		{
			continue;
		}
		if (Cast<UMyBondageLifeMessageRowWidget>(Current))
		{
			Current->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		if (UPanelWidget* Panel = Cast<UPanelWidget>(Current))
		{
			for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
			{
				if (UWidget* Child = Panel->GetChildAt(Index))
				{
					Stack.Add(Child);
				}
			}
		}
	}
}
