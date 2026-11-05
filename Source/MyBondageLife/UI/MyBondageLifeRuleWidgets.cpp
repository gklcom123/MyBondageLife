// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeRuleWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Core/MyBondageLifeAccountSubsystem.h"
#include "Core/MyBondageLifeRuleCatalog.h"
#include "MyBondageLife.h"
namespace
{
	/** True only inside a real game/PIE world (never in the widget-blueprint preview world). */
	bool IsGameWorldForWidget(const UUserWidget* Widget)
	{
		const UWorld* World = Widget ? Widget->GetWorld() : nullptr;
		return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
	}
}

// ---------------------------------------------------------------------------
// Row
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UMyBondageLifeRuleRowWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	const TSharedRef<SWidget> Result = Super::RebuildWidget();

	if (IsGameWorldForWidget(this) && WidgetTree && !bLayoutBuilt)
	{
		bLayoutBuilt = true;

		NameText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Name")));
		DescText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Desc")));
		SwitchButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_Switch")));
		DetailButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_DetailButton")));

		if (SwitchButton)
		{
			SwitchLabel = Cast<UTextBlock>(SwitchButton->GetChildAt(0));
			SwitchButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeRuleRowWidget::HandleSwitchClicked);
		}
		if (DetailButton)
		{
			DetailButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeRuleRowWidget::HandleDetailClicked);
		}
	}

	return Result;
}

void UMyBondageLifeRuleRowWidget::SetRule(const FString& InRuleId, const FString& InName, const FString& InDesc,
	bool bInEnabled, bool bInCanEdit, bool bHasDetailPanel)
{
	RuleId = InRuleId;
	bEnabled = bInEnabled;
	bCanEdit = bInCanEdit;

	if (NameText)
	{
		NameText->SetText(FText::FromString(InName));
	}
	if (DescText)
	{
		DescText->SetText(FText::FromString(InDesc));
	}

	if (SwitchButton)
	{
		// 只有主人能操作；本人查看时保持可读但不可点
		SwitchButton->SetIsEnabled(bCanEdit);

		// 未开启 = 绿色 + 「开启」；已开启 = 灰色 + 「关闭」
		const FLinearColor Color = bEnabled
			? FLinearColor(0.75f, 0.75f, 0.75f)
			: FLinearColor(0.15f, 0.75f, 0.25f);
		SwitchButton->SetBackgroundColor(Color);

		if (!SwitchLabel)
		{
			SwitchLabel = Cast<UTextBlock>(SwitchButton->GetChildAt(0));
		}
		if (SwitchLabel)
		{
			SwitchLabel->SetText(FText::FromString(bEnabled ? TEXT("\u5173\u95ED") : TEXT("\u5F00\u542F")));
			SwitchLabel->SetColorAndOpacity(FSlateColor(bEnabled ? FLinearColor::White : FLinearColor::White));
		}
	}

	if (DetailButton)
	{
		DetailButton->SetIsEnabled(bHasDetailPanel);
	}
}

void UMyBondageLifeRuleRowWidget::HandleSwitchClicked()
{
	if (!bCanEdit)
	{
		return;
	}
	OnSwitchRequested.Broadcast(RuleId, !bEnabled);
}

void UMyBondageLifeRuleRowWidget::HandleDetailClicked()
{
	OnDetailRequested.Broadcast(RuleId);
}

// ---------------------------------------------------------------------------
// Panel
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UMyBondageLifeRulePanelWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	const TSharedRef<SWidget> Result = Super::RebuildWidget();

	if (IsGameWorldForWidget(this) && WidgetTree && !bLayoutBuilt)
	{
		bLayoutBuilt = true;

		RuleList = Cast<UPanelWidget>(WidgetTree->FindWidget(TEXT("W_RuleList")));
		CloseButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_Close")));
		if (CloseButton)
		{
			CloseButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeRulePanelWidget::HandleCloseClicked);
		}
	}

	return Result;
}

void UMyBondageLifeRulePanelWidget::PopulateRules(const FString& ForAccount)
{
	ViewAccount = ForAccount;

	if (!RuleList)
	{
		RuleList = Cast<UPanelWidget>(WidgetTree ? WidgetTree->FindWidget(TEXT("W_RuleList")) : nullptr);
	}
	if (!RuleList)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Rule: W_RuleList not found in the panel."));
		return;
	}

	RuleList->ClearChildren();
	Rows.Reset();

	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr;

	UClass* RowClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_RuleRow.WBP_RuleRow_C"));
	if (!RowClass)
	{
		RowClass = UMyBondageLifeRuleRowWidget::StaticClass();
	}

	for (const FMyBondageLifeRuleItem& Item : FMyBondageLifeRuleCatalog::GetItems())
	{
		UMyBondageLifeRuleRowWidget* Row = CreateWidget<UMyBondageLifeRuleRowWidget>(this, RowClass);
		if (!Row)
		{
			continue;
		}

		Row->OnSwitchRequested.AddUniqueDynamic(this, &UMyBondageLifeRulePanelWidget::HandleRowSwitch);
		Row->OnDetailRequested.AddUniqueDynamic(this, &UMyBondageLifeRulePanelWidget::HandleRowDetail);

		if (UPanelWidget* List = RuleList)
		{
			if (UScrollBoxSlot* ScrollSlot = Cast<UScrollBoxSlot>(List->AddChild(Row)))
			{
				ScrollSlot->SetPadding(FMargin(2.0f));
			}
			else if (UVerticalBoxSlot* BoxSlot = Cast<UVerticalBoxSlot>(List->AddChild(Row)))
			{
				BoxSlot->SetPadding(FMargin(2.0f));
			}
		}

		const bool bEnabled = Account ? Account->IsRuleEnabled(Item.RuleId) : false;
		const bool bCanEdit = Account ? Account->CanEditRules() : false;
		Row->SetRule(Item.RuleId, Item.Name, Item.Desc, bEnabled, bCanEdit, !Item.DetailPanel.IsNull());

		Rows.Add(Row);
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("Rule: panel shows %d row(s)."), Rows.Num());
}

void UMyBondageLifeRulePanelWidget::HandleRowSwitch(const FString& RuleId, bool bWantEnabled)
{
	OnSwitchRequested.Broadcast(RuleId, bWantEnabled);
}

void UMyBondageLifeRulePanelWidget::HandleRowDetail(const FString& RuleId)
{
	OnDetailRequested.Broadcast(RuleId);
}

void UMyBondageLifeRulePanelWidget::HandleCloseClicked()
{
	OnClosed.Broadcast();
}
