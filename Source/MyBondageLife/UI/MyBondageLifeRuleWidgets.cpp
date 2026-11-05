// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeRuleWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
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

	/** Accepts either a panel or a nested user widget (e.g. a WBP_RuleList container). */
	UPanelWidget* ResolveContainer(UWidget* Candidate)
	{
		if (UPanelWidget* Panel = Cast<UPanelWidget>(Candidate))
		{
			return Panel;
		}

		if (UUserWidget* AsUserWidget = Cast<UUserWidget>(Candidate))
		{
			if (AsUserWidget->WidgetTree)
			{
				if (UPanelWidget* RootPanel = Cast<UPanelWidget>(AsUserWidget->WidgetTree->RootWidget))
				{
					return RootPanel;
				}
			}
		}
		return nullptr;
	}
}// ---------------------------------------------------------------------------
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

	}

	return Result;
}

void UMyBondageLifeRuleRowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree || bLayoutBuilt)
	{
		return;
	}
	bLayoutBuilt = true;

	{
		NameText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Name")));
		DescText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Desc")));
		SwitchButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_Switch")));
		DetailButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_DetailButton")));

		if (SwitchButton)
		{
			SwitchLabel = Cast<UTextBlock>(SwitchButton->GetChildAt(0));
			SwitchButton->SetClickMethod(EButtonClickMethod::MouseDown);
			SwitchButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeRuleRowWidget::HandleSwitchClicked);
		}
		if (DetailButton)
		{
			DetailButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeRuleRowWidget::HandleDetailClicked);
		}
	}
}

void UMyBondageLifeRuleRowWidget::SetRule(const FString& InRuleId, const FString& InName, const FString& InDesc,
	bool bInEnabled, bool bInCanEdit, bool bHasDetailPanel)
{
	RuleId = InRuleId;
	bEnabled = bInEnabled;
	bCanEdit = bInCanEdit;

	if (WidgetTree)
	{
		if (!NameText) { NameText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Name"))); }
		if (!DescText) { DescText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Desc"))); }
		if (!SwitchButton) { SwitchButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_Switch"))); }
		if (!DetailButton) { DetailButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_DetailButton"))); }
		if (SwitchButton && !SwitchLabel) { SwitchLabel = Cast<UTextBlock>(SwitchButton->GetChildAt(0)); }
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("Rule: apply '%s' name=%s desc=%s switch=%s detail=%s"),
		*RuleId,
		NameText ? TEXT("ok") : TEXT("NULL"),
		DescText ? TEXT("ok") : TEXT("NULL"),
		SwitchButton ? TEXT("ok") : TEXT("NULL"),
		DetailButton ? TEXT("ok") : TEXT("NULL"));

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
		SwitchButton->SetClickMethod(EButtonClickMethod::MouseDown);
		SwitchButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeRuleRowWidget::HandleSwitchClicked);

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
		// 只有「主人 + 规则已启用 + 该规则有细节参数」才可点开。
		const bool bDetailEnabled = bHasDetailPanel && bCanEdit && bEnabled;
		DetailButton->SetIsEnabled(bDetailEnabled);
		DetailButton->SetClickMethod(EButtonClickMethod::MouseDown);
		DetailButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeRuleRowWidget::HandleDetailClicked);

		UE_LOG(LogMyBondageLife, Log, TEXT("Rule: detail button '%s' enabled=%d (has=%d canEdit=%d on=%d)"),
			*RuleId, bDetailEnabled ? 1 : 0, bHasDetailPanel ? 1 : 0, bCanEdit ? 1 : 0, bEnabled ? 1 : 0);
	}
}

void UMyBondageLifeRuleRowWidget::HandleSwitchClicked()
{
	UE_LOG(LogMyBondageLife, Log, TEXT("Rule: switch clicked '%s' canEdit=%d enabled=%d"), *RuleId, bCanEdit ? 1 : 0, bEnabled ? 1 : 0);

	if (!bCanEdit)
	{
		return;
	}
	OnSwitchRequested.Broadcast(RuleId, !bEnabled);
}

void UMyBondageLifeRuleRowWidget::HandleDetailClicked()
{
	UE_LOG(LogMyBondageLife, Log, TEXT("Rule: detail clicked '%s'"), *RuleId);
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

	if (false)
	{
		bLayoutBuilt = true;

		RuleList = ResolveContainer(WidgetTree->FindWidget(TEXT("W_RuleList")));
		if (!RuleList)
		{
			RuleList = ResolveContainer(WidgetTree->FindWidget(TEXT("WBP_RuleList")));
		}
		if (!RuleList)
		{
			TArray<UWidget*> AllWidgets;
			WidgetTree->GetAllWidgets(AllWidgets);
			for (UWidget* Candidate : AllWidgets)
			{
				if (UPanelWidget* Panel = Cast<UPanelWidget>(Candidate))
				{
					if (Cast<UScrollBox>(Panel))
					{
						RuleList = Panel;
						break;
					}
				}
		}
		}

		UE_LOG(LogMyBondageLife, Log, TEXT("Rule: list container = %s"), RuleList ? *RuleList->GetName() : TEXT("none"));
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

	if (!RuleList && WidgetTree)
	{
		RuleList = ResolveContainer(WidgetTree->FindWidget(TEXT("W_RuleList")));
		if (!RuleList)
		{
			RuleList = ResolveContainer(WidgetTree->FindWidget(TEXT("WBP_RuleList")));
		}
		if (!RuleList)
		{
			TArray<UWidget*> All;
			WidgetTree->GetAllWidgets(All);
			for (UWidget* Candidate : All)
			{
				if (UScrollBox* Scroll = Cast<UScrollBox>(Candidate))
				{
					RuleList = Scroll;
					break;
				}
			}
		}
	}

	// 收集设计器里已经摆好的行（WBP_RuleList 的子项）。
	Rows.Reset();
	if (WidgetTree)
	{
		TArray<UWidget*> AllWidgets;
		WidgetTree->GetAllWidgets(AllWidgets);
		for (UWidget* Candidate : AllWidgets)
		{
			if (UMyBondageLifeRuleRowWidget* ExistingRow = Cast<UMyBondageLifeRuleRowWidget>(Candidate))
			{
				Rows.Add(ExistingRow);
			}
		}
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("Rule: container=%s designed rows=%d"),
		RuleList ? *RuleList->GetName() : TEXT("none"), Rows.Num());

	UMyBondageLifeAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMyBondageLifeAccountSubsystem>() : nullptr;

	UClass* RowClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_RuleRow.WBP_RuleRow_C"));
	if (!RowClass)
	{
		RowClass = UMyBondageLifeRuleRowWidget::StaticClass();
	}

	const TArray<FMyBondageLifeRuleItem>& Items = FMyBondageLifeRuleCatalog::GetItems();
	int32 RowIndex = 0;

	for (const FMyBondageLifeRuleItem& Item : Items)
	{
		UMyBondageLifeRuleRowWidget* Row = Rows.IsValidIndex(RowIndex) ? Rows[RowIndex].Get() : nullptr;
		++RowIndex;

		bool bNewRow = false;
		if (!Row)
		{
			Row = CreateWidget<UMyBondageLifeRuleRowWidget>(this, RowClass);
			bNewRow = true;
		}
		if (!Row)
		{
			continue;
		}

		Row->OnSwitchRequested.AddUniqueDynamic(this, &UMyBondageLifeRulePanelWidget::HandleRowSwitch);
		Row->OnDetailRequested.AddUniqueDynamic(this, &UMyBondageLifeRulePanelWidget::HandleRowDetail);

		if (bNewRow && RuleList)
		{
			if (UScrollBoxSlot* ScrollSlot = Cast<UScrollBoxSlot>(RuleList->AddChild(Row)))
			{
				ScrollSlot->SetPadding(FMargin(2.0f));
			}
			else if (UVerticalBoxSlot* BoxSlot = Cast<UVerticalBoxSlot>(RuleList->AddChild(Row)))
			{
				BoxSlot->SetPadding(FMargin(2.0f));
			}
			Rows.Add(Row);
		}

		const bool bEnabled = Account ? Account->IsRuleEnabled(Item.RuleId) : false;
		// 只有「被查看者的主人」能改：完全以服务端 canEdit 为准。
		const bool bCanEdit = Account && Account->CanEditRules();
		UE_LOG(LogMyBondageLife, Log, TEXT("Rule: row '%s' enabled=%d canEdit=%d"), *Item.RuleId, bEnabled ? 1 : 0, bCanEdit ? 1 : 0);
		Row->SetRule(Item.RuleId, Item.Name, Item.Desc, bEnabled, bCanEdit, Item.HasDetail());
	}

	// 多出来的设计行隐藏。
	for (int32 Index = Items.Num(); Index < Rows.Num(); ++Index)
	{
		if (Rows[Index])
		{
			Rows[Index]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("Rule: panel shows %d row(s) for %d item(s)."), Rows.Num(), Items.Num());
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

void UMyBondageLifeRulePanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree || bLayoutBuilt)
	{
		return;
	}
	bLayoutBuilt = true;

	RuleList = ResolveContainer(WidgetTree->FindWidget(TEXT("W_RuleList")));
	if (!RuleList)
	{
		RuleList = ResolveContainer(WidgetTree->FindWidget(TEXT("WBP_RuleList")));
	}

	CloseButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_Close")));
	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeRulePanelWidget::HandleCloseClicked);
	}
}