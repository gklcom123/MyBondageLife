// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeOutfitWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Core/MyBondageLifeOutfitCatalog.h"
#include "Engine/Texture2D.h"

namespace
{
	UButton* MakeFlatButton(UWidgetTree* Tree, const FString& Caption, UTextBlock*& OutLabel)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		OutLabel = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		OutLabel->SetText(FText::FromString(Caption));
		OutLabel->SetJustification(ETextJustify::Center);
		Button->AddChild(OutLabel);
		return Button;
	}
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UMyBondageLifeOutfitSlotsWidget::RebuildWidget()
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

void UMyBondageLifeOutfitSlotsWidget::BuildLayout()
{
	SlotButtons.Reset();
	SlotIcons.Reset();
	SlotLabels.Reset();

	UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Canvas)
	{
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("OutfitSlotsRoot"));
		WidgetTree->RootWidget = Canvas;
	}
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	const float CellSize = 96.0f;
	const float StepX = 112.0f;
	const float StepY = 140.0f;

	for (int32 SlotIndex = 0; SlotIndex < FMyBondageLifeOutfitCatalog::SlotCount; ++SlotIndex)
	{
		const FString Suffix = FString::Printf(TEXT("_%d"), SlotIndex);
		UButton* Button = Cast<UButton>(WidgetTree->FindWidget(*(FString(TEXT("W_OutfitSlotButton")) + Suffix)));

		if (!Button)
		{
			USizeBox* Cell = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			Cell->SetWidthOverride(CellSize);
			Cell->SetHeightOverride(CellSize);
			if (UCanvasPanelSlot* CellSlot = Canvas->AddChildToCanvas(Cell))
			{
				const int32 Column = SlotIndex % 6;
				const int32 Row = SlotIndex / 6;
				CellSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
				CellSlot->SetPosition(FVector2D(16.0f + Column * StepX, 16.0f + Row * StepY));
				CellSlot->SetSize(FVector2D(CellSize, CellSize));
			}

			Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
			Cell->AddChild(Button);
		}

		UImage* Icon = Cast<UImage>(WidgetTree->FindWidget(*(FString(TEXT("W_OutfitIcon")) + Suffix)));
		if (!Icon)
		{
			Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), *(FString(TEXT("W_OutfitIcon")) + Suffix));
			Button->AddChild(Icon);
		}

		UTextBlock* Label = Cast<UTextBlock>(WidgetTree->FindWidget(*(FString(TEXT("W_OutfitLabel")) + Suffix)));
		if (!Label)
		{
			Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(FString(TEXT("W_OutfitLabel")) + Suffix));
			Label->SetText(FText::FromString(FMyBondageLifeOutfitCatalog::GetSlotName(SlotIndex)));
			Label->SetJustification(ETextJustify::Center);
			if (UCanvasPanelSlot* LabelSlot = Canvas->AddChildToCanvas(Label))
			{
				const int32 Column = SlotIndex % 6;
				const int32 Row = SlotIndex / 6;
				LabelSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
				LabelSlot->SetPosition(FVector2D(16.0f + Column * StepX, 16.0f + Row * StepY + CellSize + 4.0f));
				LabelSlot->SetSize(FVector2D(StepX, 22.0f));
			}
		}

		SlotButtons.Add(Button);
		SlotIcons.Add(Icon);
		SlotLabels.Add(Label);

		UMyBondageLifeOutfitSlotProxy* Proxy = NewObject<UMyBondageLifeOutfitSlotProxy>(this);
		Proxy->Owner = this;
		Proxy->Slot = SlotIndex;
		ClickProxies.Add(Proxy);
		Button->OnClicked.AddUniqueDynamic(Proxy, &UMyBondageLifeOutfitSlotProxy::HandleClicked);
	}
}

void UMyBondageLifeOutfitSlotsWidget::RefreshSlots(const TMap<int32, FString>& Equipped)
{
	FMyBondageLifeOutfitCatalog::EnsureLoaded();

	for (int32 SlotIndex = 0; SlotIndex < SlotButtons.Num(); ++SlotIndex)
	{
		const FString* ItemId = Equipped.Find(SlotIndex);
		const FMyBondageLifeOutfitItem* Item = (ItemId && !ItemId->IsEmpty())
			? FMyBondageLifeOutfitCatalog::Find(FName(**ItemId))
			: nullptr;

		if (SlotIcons.IsValidIndex(SlotIndex) && SlotIcons[SlotIndex])
		{
			if (Item)
			{
				UTexture2D* Texture = Item->Icon.LoadSynchronous();
				if (Texture)
				{
					SlotIcons[SlotIndex]->SetBrushFromTexture(Texture, true);
				}
			}
			else
			{
				SlotIcons[SlotIndex]->SetBrushFromTexture(nullptr, true);
			}
		}

		if (SlotLabels.IsValidIndex(SlotIndex) && SlotLabels[SlotIndex])
		{
			const FString Name = Item ? Item->Name : FMyBondageLifeOutfitCatalog::GetSlotName(SlotIndex);
			SlotLabels[SlotIndex]->SetText(FText::FromString(Name));
		}
	}
}

void UMyBondageLifeOutfitSlotsWidget::HandleSlotClicked(int32 InSlot)
{
	OnSlotClicked.Broadcast(InSlot);
}

// ---------------------------------------------------------------------------
// Picker
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UMyBondageLifeOutfitPickerWidget::RebuildWidget()
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

void UMyBondageLifeOutfitPickerWidget::BuildLayout()
{
	PickerPanel = Cast<UBorder>(WidgetTree->FindWidget(TEXT("W_OutfitPicker_Panel")));
	PickerTitle = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_OutfitPicker_Title")));
	PickerList = Cast<UPanelWidget>(WidgetTree->FindWidget(TEXT("W_OutfitPicker_List")));
	RemoveButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_OutfitPicker_Remove")));
	UButton* CancelButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_OutfitPicker_Cancel")));

	if (PickerPanel)
	{
		PickerPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (RemoveButton)
	{
		RemoveButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeOutfitPickerWidget::HandleRemoveClicked);
	}
	if (CancelButton)
	{
		CancelButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeOutfitPickerWidget::HandleCancelClicked);
	}
}

void UMyBondageLifeOutfitPickerWidget::OpenForSlot(int32 InSlot, const TMap<int32, FString>& Equipped)
{
	PickerSlot = InSlot;
	const FString* Current = Equipped.Find(InSlot);
	EquippedItemId = Current ? *Current : FString();

	if (PickerTitle)
	{
		PickerTitle->SetText(FText::FromString(
			FString::Printf(TEXT("%s : 选择衣物"), *FMyBondageLifeOutfitCatalog::GetSlotName(InSlot))));
	}

	RefreshContents();

	if (PickerPanel)
	{
		PickerPanel->SetVisibility(ESlateVisibility::Visible);
	}
	SetVisibility(ESlateVisibility::Visible);
}

void UMyBondageLifeOutfitPickerWidget::ClosePicker()
{
	if (PickerPanel)
	{
		PickerPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetVisibility(ESlateVisibility::Collapsed);
}

void UMyBondageLifeOutfitPickerWidget::RefreshContents()
{
	if (!PickerList)
	{
		return;
	}

	PickerList->ClearChildren();

	for (const FMyBondageLifeOutfitItem* Item : FMyBondageLifeOutfitCatalog::GetItemsForSlot(PickerSlot))
	{
		if (!Item)
		{
			continue;
		}

		UTextBlock* Label = nullptr;
		UButton* Button = MakeFlatButton(WidgetTree, Item->Name.IsEmpty() ? Item->ItemId.ToString() : Item->Name, Label);

		FString ItemId = Item->ItemId.ToString();
		UMyBondageLifeOutfitItemProxy* Proxy = NewObject<UMyBondageLifeOutfitItemProxy>(this);
		Proxy->Owner = this;
		Proxy->ItemId = ItemId;
		ItemProxies.Add(Proxy);
		Button->OnClicked.AddUniqueDynamic(Proxy, &UMyBondageLifeOutfitItemProxy::HandleClicked);

		if (UPanelWidget* List = PickerList)
		{
			List->AddChild(Button);
		}
	}

	if (RemoveButton)
	{
		RemoveButton->SetVisibility(EquippedItemId.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

void UMyBondageLifeOutfitPickerWidget::ChooseItem(const FString& ItemId)
{
	ChosenItemId = ItemId;
	const int32 ChosenSlot = PickerSlot;
	ClosePicker();
	OnItemChosen.Broadcast(ChosenSlot);
}

void UMyBondageLifeOutfitPickerWidget::HandleRemoveClicked()
{
	const int32 RemovedSlot = PickerSlot;
	ClosePicker();
	OnRemoveChosen.Broadcast(RemovedSlot);
}

void UMyBondageLifeOutfitPickerWidget::HandleCancelClicked()
{
	ClosePicker();
}

void UMyBondageLifeOutfitSlotProxy::HandleClicked()
{
	if (Owner)
	{
		Owner->HandleSlotClicked(Slot);
	}
}

void UMyBondageLifeOutfitItemProxy::HandleClicked()
{
	if (Owner)
	{
		Owner->ChooseItem(ItemId);
	}
}
