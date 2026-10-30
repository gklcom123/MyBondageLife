// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeRestraintPickerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/MyBondageLifeAccountSubsystem.h"
#include "Core/MyBondageLifeRestraintCatalog.h"
#include "Engine/Texture2D.h"
#include "MyBondageLife.h"

namespace
{
	/** Finds the first UButton inside a named widget (the designer may wrap it). */
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

	UButton* FindNamedButton(UWidgetTree* Tree, const TCHAR* Name)
	{
		return Tree ? FindButtonIn(Tree->FindWidget(Name)) : nullptr;
	}
}
void UMyBondageLifePickerItemProxy::HandleClicked()
{
	if (Owner)
	{
		Owner->HandleItemClicked(ItemId);
	}
}

TSharedRef<SWidget> UMyBondageLifeRestraintPickerWidget::RebuildWidget()
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

void UMyBondageLifeRestraintPickerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr)
	{
		Account->OnRestraintsChanged.AddUniqueDynamic(this, &UMyBondageLifeRestraintPickerWidget::HandleRestraintsChanged);
	}
}

void UMyBondageLifeRestraintPickerWidget::BuildLayout()
{
	// Adopt the designed widgets when the blueprint provides them.
	PickerTitle = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Picker_Title")));
	PickerGrid = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("W_Picker_Grid")));
	LockActionButton = FindNamedButton(WidgetTree, TEXT("W_Picker_LockAction"));

	RemoveButton = FindNamedButton(WidgetTree, TEXT("W_Picker_Remove"));
	if (!RemoveButton)
	{
		RemoveButton = FindNamedButton(WidgetTree, TEXT("W_Picker_Unequip"));
	}
	LockActionLabel = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Picker_LockAction_Label")));
	CancelButton = FindNamedButton(WidgetTree, TEXT("W_Picker_Cancel"));
	if (!CancelButton)
	{
		CancelButton = FindNamedButton(WidgetTree, TEXT("W_Picker_CancelButton"));
	}
	if (!CancelButton)
	{
		CancelButton = FindNamedButton(WidgetTree, TEXT("W_Picker_Close"));
	}

	if (!PickerTitle || !PickerGrid)
	{
		// Fallback layout so the widget still works before it is designed.
		UVerticalBox* Column = Cast<UVerticalBox>(WidgetTree->RootWidget);
		if (!Column)
		{
			Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PickerRoot"));
			if (WidgetTree->RootWidget)
			{
				// Keep the asset's root: adding to it avoids orphaning authored widgets.
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

		if (!PickerTitle)
		{
			PickerTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("W_Picker_Title"));
			Column->AddChildToVerticalBox(PickerTitle);
		}

		if (!PickerGrid)
		{
			PickerGrid = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("W_Picker_Grid"));
			if (UVerticalBoxSlot* GridSlot = Column->AddChildToVerticalBox(PickerGrid))
			{
				GridSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}

		if (!LockActionButton)
		{
			LockActionButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("W_Picker_LockAction"));
			Column->AddChildToVerticalBox(LockActionButton);

			LockActionLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("W_Picker_LockAction_Label"));
			LockActionLabel->SetJustification(ETextJustify::Center);
			LockActionButton->AddChild(LockActionLabel);
		}

		if (!CancelButton)
		{
			CancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("W_Picker_Cancel"));
			Column->AddChildToVerticalBox(CancelButton);

			UTextBlock* CancelLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			CancelLabel->SetText(FText::FromString(TEXT("\u53D6\u6D88")));
			CancelLabel->SetJustification(ETextJustify::Center);
			CancelButton->AddChild(CancelLabel);
		}
	}

	// Bind only real instances, never during design time: a missing widget used to
	// crash the delegate access detector.
	if (!IsDesignTime())
	{
		if (IsValid(LockActionButton))
		{
			LockActionButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeRestraintPickerWidget::HandleLockActionClicked);
		}
		if (IsValid(CancelButton))
		{
			CancelButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeRestraintPickerWidget::HandleCancelClicked);
		}
		if (IsValid(RemoveButton))
		{
			RemoveButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeRestraintPickerWidget::HandleRemoveClicked);
		}
	}

	UE_LOG(LogMyBondageLife, Log,
		TEXT("Picker: adopted title=%d grid=%d lockAction=%d cancel=%d"),
		PickerTitle ? 1 : 0, PickerGrid ? 1 : 0, LockActionButton ? 1 : 0, CancelButton ? 1 : 0);

	if (IsValid(RemoveButton))
	{
		RemoveButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	SetVisibility(ESlateVisibility::Collapsed);
	OnLayoutBuilt();
}

void UMyBondageLifeRestraintPickerWidget::OpenForSlot(int32 SlotIndex, const FString& InViewAccount)
{
	PickerSlot = SlotIndex;
	ViewAccount = InViewAccount;

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetZOrder(150);
	}

	RefreshContents();
	SetVisibility(ESlateVisibility::Visible);
}

void UMyBondageLifeRestraintPickerWidget::ClosePicker()
{
	SetVisibility(ESlateVisibility::Collapsed);
	OnPickerClosed.Broadcast(PickerSlot);
}

void UMyBondageLifeRestraintPickerWidget::RefreshContents()
{
	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	if (!Account)
	{
		return;
	}

	if (!PickerGrid)
	{
		// Nothing to fill, but the 脱下 / 上锁 state still has to be right.
		const FString WornOnly = ViewAccount.IsEmpty()
			? Account->GetEquippedRestraint(PickerSlot)
			: Account->GetViewedRestraint(PickerSlot);
		if (IsValid(RemoveButton))
		{
			RemoveButton->SetVisibility(WornOnly.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		}
		return;
	}

	const bool bOther = !ViewAccount.IsEmpty();
	const FString WornId = bOther
		? Account->GetViewedRestraint(PickerSlot)
		: Account->GetEquippedRestraint(PickerSlot);
	const FMyBondageLifeRestraintLock Lock = bOther
		? Account->GetViewedLock(PickerSlot)
		: Account->GetRestraintLock(PickerSlot);

	PickerGrid->ClearChildren();
	ItemProxies.Reset();

	if (PickerTitle)
	{
		FString Title = FMyBondageLifeRestraintCatalog::GetSlotName(PickerSlot);
		if (Lock.LockType > 0 && PickerTitle)
		{
			Title += TEXT(" \uFF1A\u5DF2\u4E0A\u9501");
		}
		else
		{
			Title += TEXT(" \uFF1A\u9009\u62E9\u62D8\u675F");
		}
		PickerTitle->SetText(FText::FromString(Title));
	}

	if (Lock.LockType == 0)
	{
		int32 Index = 0;
		for (const FMyBondageLifeRestraintItem* Item : FMyBondageLifeRestraintCatalog::GetItemsForSlot(PickerSlot))
		{
			const int32 Col = Index % 4;
			const int32 Row = Index / 4;
			++Index;

			USizeBox* CellSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			CellSize->SetWidthOverride(150.0f);
			CellSize->SetHeightOverride(190.0f);
			if (UCanvasPanelSlot* CellSlot = PickerGrid->AddChildToCanvas(CellSize))
			{
				CellSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
				CellSlot->SetAlignment(FVector2D(0.0f, 0.0f));
				CellSlot->SetPosition(FVector2D(Col * 170.0f, Row * 200.0f));
				CellSlot->SetAutoSize(true);
			}

			UButton* ItemButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
			CellSize->AddChild(ItemButton);

			UMyBondageLifePickerItemProxy* Proxy = NewObject<UMyBondageLifePickerItemProxy>(this);
			Proxy->Owner = this;
			Proxy->ItemId = Item->ItemId;
			Proxy->Slot = PickerSlot;
			ItemProxies.Add(Proxy);
			ItemButton->OnClicked.AddUniqueDynamic(Proxy, &UMyBondageLifePickerItemProxy::HandleClicked);

			UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			ItemButton->AddChild(Column);

			USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			IconSize->SetWidthOverride(150.0f);
			IconSize->SetHeightOverride(150.0f);
			if (UVerticalBoxSlot* IconSlot = Column->AddChildToVerticalBox(IconSize))
			{
				IconSlot->SetHorizontalAlignment(HAlign_Center);
			}

			UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			if (UTexture2D* Texture = Item->Icon.LoadSynchronous())
			{
				Icon->SetBrushFromTexture(Texture, true);
			}
			IconSize->AddChild(Icon);

			UTextBlock* NameLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			NameLabel->SetText(FText::FromString(FMyBondageLifeRestraintCatalog::GetDisplayName(Item->ItemId)));
			NameLabel->SetJustification(ETextJustify::Center);
			if (UVerticalBoxSlot* NameSlot = Column->AddChildToVerticalBox(NameLabel))
			{
				NameSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}
	}

	// 脱下 only makes sense when the slot holds something and is not locked.
	if (IsValid(RemoveButton))
	{
		RemoveButton->SetVisibility((!WornId.IsEmpty() && Lock.LockType == 0)
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	}

	// Lock action: 上锁 when free, 开锁 when locked and allowed.
	if (LockActionButton && LockActionLabel)
	{
		if (Lock.LockType > 0)
		{
			LockActionLabel->SetText(FText::FromString(TEXT("\u5F00\u9501")));
			LockActionButton->SetIsEnabled(Lock.bCanUnlock);
		}
		else
		{
			LockActionLabel->SetText(FText::FromString(TEXT("\u4E0A\u9501")));
			LockActionButton->SetIsEnabled(!WornId.IsEmpty());
		}
	}
}

void UMyBondageLifeRestraintPickerWidget::HandleItemClicked(FName ItemId)
{
	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	if (!Account)
	{
		return;
	}

	const bool bOther = !ViewAccount.IsEmpty();
	const FString WornId = bOther
		? Account->GetViewedRestraint(PickerSlot)
		: Account->GetEquippedRestraint(PickerSlot);

	if (WornId == ItemId.ToString())
	{
		if (bOther)
		{
			Account->UnequipRestraintFor(ViewAccount, PickerSlot);
		}
		else
		{
			Account->UnequipRestraint(PickerSlot);
		}
	}
	else if (bOther)
	{
		Account->EquipRestraintFor(ViewAccount, PickerSlot, ItemId.ToString());
	}
	else
	{
		Account->EquipRestraint(PickerSlot, ItemId.ToString());
	}

	ClosePicker();
}

void UMyBondageLifeRestraintPickerWidget::HandleCancelClicked()
{
	ClosePicker();
}

void UMyBondageLifeRestraintPickerWidget::HandleLockActionClicked()
{
	// The wardrobe owns the lock menus: it reacts to this request.
	OnLockRequested.Broadcast(PickerSlot);
}

void UMyBondageLifeRestraintPickerWidget::HandleRestraintsChanged()
{
	if (PickerSlot != INDEX_NONE && GetVisibility() == ESlateVisibility::Visible)
	{
		RefreshContents();
	}
}

void UMyBondageLifeRestraintPickerWidget::HandleRemoveClicked()
{
	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>()
		: nullptr;
	if (!Account)
	{
		return;
	}

	if (ViewAccount.IsEmpty())
	{
		Account->UnequipRestraint(PickerSlot);
	}
	else
	{
		Account->UnequipRestraintFor(ViewAccount, PickerSlot);
	}

	ClosePicker();
}

