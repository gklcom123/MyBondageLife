// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeLockInputWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "MyBondageLife.h"

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

TSharedRef<SWidget> UMyBondageLifeLockInputWidget::RebuildWidget()
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

void UMyBondageLifeLockInputWidget::BuildLayout()
{
	InputTitle = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Input_Title")));
	InputHint = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Input_Hint")));
	InputBox = Cast<UEditableTextBox>(WidgetTree->FindWidget(TEXT("W_Input_Box")));
	OkButton = FindButtonIn(WidgetTree->FindWidget(TEXT("W_Input_Ok")));
	CancelButton = FindButtonIn(WidgetTree->FindWidget(TEXT("W_Input_Cancel")));

	if (!InputBox)
	{
		UVerticalBox* Column = Cast<UVerticalBox>(WidgetTree->RootWidget);
		if (!Column)
		{
			Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LockInputRoot"));
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

		InputBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("W_Input_Box"));
		Column->AddChildToVerticalBox(InputBox);
	}

	if (IsValid(OkButton))
	{
		OkButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeLockInputWidget::HandleOk);
	}
	if (IsValid(CancelButton))
	{
		CancelButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeLockInputWidget::HandleCancel);
	}

	SetVisibility(ESlateVisibility::Collapsed);
	OnLayoutBuilt();
}

void UMyBondageLifeLockInputWidget::OpenFor(int32 InLockType, int32 InSlotIndex)
{
	LockType = InLockType;
	SlotIndex = InSlotIndex;

	const bool bPassword = (LockType == 4 || LockType == 104);

	if (InputTitle)
	{
		InputTitle->SetText(FText::FromString(bPassword
			? TEXT("\u8F93\u5165\u5BC6\u7801")
			: TEXT("\u8F93\u5165\u65F6\u957F")));
	}
	if (InputHint)
	{
		InputHint->SetText(FText::FromString(bPassword
			? TEXT("4~8 \u4F4D\u7EAF\u6570\u5B57")
			: TEXT("\u683C\u5F0F \u65F6:\u5206:\u79D2\uFF0C\u4F8B\u5982 01:30:00")));
	}
	if (InputBox)
	{
		InputBox->SetHintText(FText::FromString(bPassword ? TEXT("0000") : TEXT("HH:MM:SS")));
		InputBox->SetText(FText::GetEmpty());
	}

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetZOrder(300);
	}

	SetVisibility(ESlateVisibility::Visible);
}

void UMyBondageLifeLockInputWidget::CloseInput()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UMyBondageLifeLockInputWidget::HandleOk()
{
	const FString Text = InputBox ? InputBox->GetText().ToString().TrimStartAndEnd() : FString();
	CloseInput();
	OnConfirmed.Broadcast(LockType, Text);
}

void UMyBondageLifeLockInputWidget::HandleCancel()
{
	CloseInput();
	OnCancelled.Broadcast();
}
