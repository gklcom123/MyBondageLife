// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeLockMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/MyBondageLifeRestraintCatalog.h"
#include "Engine/Texture2D.h"
#include "MyBondageLife.h"

namespace
{
	const TCHAR* LockIconPath(int32 LockType)
	{
		switch (LockType)
		{
		case 1: return TEXT("/Game/2DAssest/UI/MasterLock.MasterLock");
		case 2: return TEXT("/Game/2DAssest/UI/MasterTimeLock.MasterTimeLock");
		case 3: return TEXT("/Game/2DAssest/UI/NormalLock.NormalLock");
		case 4: return TEXT("/Game/2DAssest/UI/PasswordLock.PasswordLock");
		case 5: return TEXT("/Game/2DAssest/UI/TimePadLock.TimePadLock");
		default: return nullptr;
		}
	}

	FText LockDisplayName(int32 LockType)
	{
		switch (LockType)
		{
		case 1: return FText::FromString(TEXT("\u4E3B\u4EBA\u9501"));
		case 2: return FText::FromString(TEXT("\u4E3B\u4EBA\u8BA1\u65F6\u9501"));
		case 3: return FText::FromString(TEXT("\u666E\u901A\u6302\u9501"));
		case 4: return FText::FromString(TEXT("\u5BC6\u7801\u9501"));
		case 5: return FText::FromString(TEXT("\u8BA1\u65F6\u9501"));
		default: return FText::GetEmpty();
		}
	}

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

	UButton* FindNamedButton(UWidgetTree* Tree, const FString& Name)
	{
		return Tree ? FindButtonIn(Tree->FindWidget(*Name)) : nullptr;
	}
}

void UMyBondageLifeLockChoiceProxy::HandleClicked()
{
	if (Owner)
	{
		Owner->OnLockChosen.Broadcast(LockType);
	}
}

TSharedRef<SWidget> UMyBondageLifeLockMenuWidget::RebuildWidget()
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

void UMyBondageLifeLockMenuWidget::BuildLayout()
{
	MenuTitle = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_LockMenu_Title")));
	CancelButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_LockMenu_Cancel")));

	LockButtons.Reset();
	ChoiceProxies.Reset();

	// Buttons placed inside W_LockMenu_Row are used in order when names are missing:
	// the designer can skip naming entirely.
	TArray<UButton*> RowButtons;
	if (UPanelWidget* RowPanel = Cast<UPanelWidget>(WidgetTree->FindWidget(TEXT("W_LockMenu_Row"))))
	{
		for (int32 Index = 0; Index < RowPanel->GetChildrenCount(); ++Index)
		{
			if (UButton* RowButton = FindButtonIn(RowPanel->GetChildAt(Index)))
			{
				RowButtons.Add(RowButton);
			}
		}
	}

	// Last resort: take buttons in hierarchy order (the designer may not name them).
	if (RowButtons.Num() < 5)
	{
		TArray<UButton*> AllButtons;
		if (UPanelWidget* RootPanel = Cast<UPanelWidget>(WidgetTree->RootWidget))
		{
			for (int32 Index = 0; Index < RootPanel->GetChildrenCount(); ++Index)
			{
				if (UButton* Found = FindButtonIn(RootPanel->GetChildAt(Index)))
				{
					AllButtons.Add(Found);
				}
			}
		}

		// Skip the cancel button: it is not one of the five locks.
		TArray<UButton*> Filtered;
		for (UButton* Candidate : AllButtons)
		{
			if (Candidate && Candidate != CancelButton)
			{
				Filtered.Add(Candidate);
			}
		}

		if (Filtered.Num() >= 5)
		{
			RowButtons = Filtered;
		}
	}

	bool bAnyDesigned = false;

	for (int32 LockType = 1; LockType <= 5; ++LockType)
	{
		// Several accepted spellings, plus an order-based fallback below.
		UButton* Button = FindNamedButton(WidgetTree, FString::Printf(TEXT("W_Lock_%d"), LockType));
		if (!Button)
		{
			Button = FindNamedButton(WidgetTree, FString::Printf(TEXT("W_Lock%d"), LockType));
		}
		if (!Button)
		{
			Button = FindNamedButton(WidgetTree, FString::Printf(TEXT("W_Lock_Button_%d"), LockType));
		}

		if (!Button && RowButtons.IsValidIndex(LockType - 1))
		{
			Button = RowButtons[LockType - 1];
		}

		if (Button)
		{
			LockButtons.Add(Button);
			bAnyDesigned = true;
		}
		else if (bAnyDesigned)
		{
			// Partial designs: leave the missing one out instead of rebuilding the old row.
			LockButtons.Add(nullptr);
			continue;
		}
		else
		{
			// Fallback: build the row in code when the asset has no lock buttons.
			UVerticalBox* Column = Cast<UVerticalBox>(WidgetTree->RootWidget);
			if (!Column)
			{
				Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LockMenuRoot"));
				if (WidgetTree->RootWidget)
				{
					if (UPanelWidget* ExistingRoot = Cast<UPanelWidget>(WidgetTree->RootWidget))
					{
						ExistingRoot->AddChild(Column);
					}
				}
				else
				{
					WidgetTree->RootWidget = Column;
				}
			}

			UHorizontalBox* Row = Cast<UHorizontalBox>(WidgetTree->FindWidget(TEXT("LockMenuRow")));
			if (!Row)
			{
				Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("LockMenuRow"));
				Column->AddChildToVerticalBox(Row);
			}

			USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			IconSize->SetWidthOverride(110.0f);
			IconSize->SetHeightOverride(110.0f);
			if (UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconSize))
			{
				IconSlot->SetPadding(FMargin(6.0f));
			}

			Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
			IconSize->AddChild(Button);

			UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, LockIconPath(LockType)))
			{
				Icon->SetBrushFromTexture(Texture, true);
			}
			Button->AddChild(Icon);

			LockButtons.Add(Button);

			UTextBlock* Caption = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Caption->SetText(LockDisplayName(LockType));
			Caption->SetJustification(ETextJustify::Center);
			Column->AddChildToVerticalBox(Caption);
		}

		if (LockButtons.IsValidIndex(LockType - 1) && !IsValid(LockButtons[LockType - 1]))
		{
			continue;
		}

		Button->SetToolTipText(LockDisplayName(LockType));

		UMyBondageLifeLockChoiceProxy* Proxy = NewObject<UMyBondageLifeLockChoiceProxy>(this);
		Proxy->Owner = this;
		Proxy->LockType = LockType;
		ChoiceProxies.Add(Proxy);
		Button->OnClicked.AddUniqueDynamic(Proxy, &UMyBondageLifeLockChoiceProxy::HandleClicked);
	}

	if (IsValid(CancelButton))
	{
		CancelButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeLockMenuWidget::HandleCancel);
	}

	SetVisibility(ESlateVisibility::Collapsed);
	OnLayoutBuilt();
}

void UMyBondageLifeLockMenuWidget::OpenForSlot(int32 InSlotIndex)
{	// Canvas panels sort by ZOrder, not by child index: force the menu on top.
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetZOrder(200);
	}

	SlotIndex = InSlotIndex;

	if (MenuTitle)
	{
		MenuTitle->SetText(FText::FromString(FString::Printf(TEXT("%s \uFF1A\u9009\u62E9\u9501"),
			*FMyBondageLifeRestraintCatalog::GetSlotName(SlotIndex))));
	}

	SetVisibility(ESlateVisibility::Visible);
}

void UMyBondageLifeLockMenuWidget::CloseMenu()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UMyBondageLifeLockMenuWidget::SetLockAvailable(int32 LockType, bool bAvailable)
{
	if (LockButtons.IsValidIndex(LockType - 1) && IsValid(LockButtons[LockType - 1]))
	{
		LockButtons[LockType - 1]->SetIsEnabled(bAvailable);
	}
}

void UMyBondageLifeLockMenuWidget::HandleChoice(int32 LockType)
{
	OnLockChosen.Broadcast(LockType);
}

void UMyBondageLifeLockMenuWidget::HandleCancel()
{
	CloseMenu();
	OnCancelled.Broadcast();
}
