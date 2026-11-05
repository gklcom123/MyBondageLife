// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeRuleDetailPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/MyBondageLifeRuleCatalog.h"
#include "MyBondageLife.h"

namespace
{
	UTextBlock* MakeLabel(UWidgetTree* Tree, const FString& Text)
	{
		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(Text));
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.92f, 0.92f)));
		return Label;
	}
}

TSharedRef<SWidget> UMyBondageLifeRuleDetailPanelWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}
	return Super::RebuildWidget();
}

void UMyBondageLifeRuleDetailPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree || bLayoutBuilt)
	{
		return;
	}
	bLayoutBuilt = true;

	FieldsHost = Cast<UPanelWidget>(WidgetTree->FindWidget(TEXT("W_FieldsHost")));
	ConfirmButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_Confirm")));
	CancelButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_Cancel")));
	CloseButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_Close")));

	if (ConfirmButton) { ConfirmButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeRuleDetailPanelWidget::HandleConfirmClicked); }
	if (CancelButton) { CancelButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeRuleDetailPanelWidget::HandleCancelClicked); }
	if (CloseButton) { CloseButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeRuleDetailPanelWidget::HandleCloseClicked); }
}

void UMyBondageLifeRuleDetailPanelWidget::BuildForRule(const FString& InRuleId)
{
	RuleId = InRuleId;
	IntBoxes.Reset();
	StringBoxes.Reset();
	BoolBoxes.Reset();

	if (!FieldsHost)
	{
		FieldsHost = Cast<UPanelWidget>(WidgetTree ? WidgetTree->FindWidget(TEXT("W_FieldsHost")) : nullptr);
	}
	if (!FieldsHost || !WidgetTree)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("RuleDetail: W_FieldsHost missing."));
		return;
	}

	FieldsHost->ClearChildren();

	const FMyBondageLifeRuleItem* Item = FMyBondageLifeRuleCatalog::Find(RuleId);
	if (!Item)
	{
		return;
	}

	FMyBondageLifeRuleDetail Current;
	if (const UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr)
	{
		Account->GetRuleDetailStruct(RuleId, Current);
	}

	// 描述顺序：ints -> strings -> bools
	int32 DescIndex = 0;
	auto DescAt = [Item, &DescIndex]() -> FString
	{
		const FString Value = Item->DetailDescs.IsValidIndex(DescIndex)
			? Item->DetailDescs[DescIndex] : FString();
		++DescIndex;
		return Value;
	};

	for (int32 Index = 0; Index < Item->DetailIntCount; ++Index)
	{
		UVerticalBox* Row = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Row->AddChildToVerticalBox(MakeLabel(WidgetTree, DescAt()));

		UEditableTextBox* Box = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
		Box->SetText(FText::FromString(Current.Ints.IsValidIndex(Index)
			? FString::FromInt(Current.Ints[Index]) : FString()));
		Row->AddChildToVerticalBox(Box);

		FieldsHost->AddChild(Row);
		IntBoxes.Add(Box);
	}

	for (int32 Index = 0; Index < Item->DetailStringCount; ++Index)
	{
		UVerticalBox* Row = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Row->AddChildToVerticalBox(MakeLabel(WidgetTree, DescAt()));

		UEditableTextBox* Box = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
		Box->SetText(FText::FromString(Current.Strings.IsValidIndex(Index) ? Current.Strings[Index] : FString()));
		Row->AddChildToVerticalBox(Box);

		FieldsHost->AddChild(Row);
		StringBoxes.Add(Box);
	}

	for (int32 Index = 0; Index < Item->DetailBoolCount; ++Index)
	{
		UVerticalBox* Row = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Row->AddChildToVerticalBox(MakeLabel(WidgetTree, DescAt()));

		UCheckBox* Box = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
		Box->SetIsChecked(Current.Bools.IsValidIndex(Index) ? Current.Bools[Index] : false);
		Row->AddChildToVerticalBox(Box);

		FieldsHost->AddChild(Row);
		BoolBoxes.Add(Box);
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("RuleDetail: built %d int / %d string / %d bool for '%s'"),
		IntBoxes.Num(), StringBoxes.Num(), BoolBoxes.Num(), *RuleId);
}

void UMyBondageLifeRuleDetailPanelWidget::HandleConfirmClicked()
{
	FMyBondageLifeRuleDetail Detail;

	for (const TObjectPtr<UEditableTextBox>& Box : IntBoxes)
	{
		const FString Text = Box ? Box->GetText().ToString().TrimStartAndEnd() : FString();
		Detail.Ints.Add(Text.IsEmpty() ? 0 : FCString::Atoi(*Text));
	}
	for (const TObjectPtr<UEditableTextBox>& Box : StringBoxes)
	{
		Detail.Strings.Add(Box ? Box->GetText().ToString() : FString());
	}
	for (const TObjectPtr<UCheckBox>& Box : BoolBoxes)
	{
		Detail.Bools.Add(Box ? Box->IsChecked() : false);
	}

	OnSaved.Broadcast(RuleId, Detail);
}

void UMyBondageLifeRuleDetailPanelWidget::HandleCancelClicked()
{
	OnClosed.Broadcast();
}

void UMyBondageLifeRuleDetailPanelWidget::HandleCloseClicked()
{
	OnClosed.Broadcast();
}
