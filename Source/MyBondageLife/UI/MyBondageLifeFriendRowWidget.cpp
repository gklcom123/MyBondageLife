// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeFriendRowWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "MyBondageLife.h"

namespace
{
	void CollectTexts(UWidget* Root, TArray<UTextBlock*>& Out)
	{
		if (!Root)
		{
			return;
		}
		if (UTextBlock* AsText = Cast<UTextBlock>(Root))
		{
			Out.Add(AsText);
		}
		if (UPanelWidget* Panel = Cast<UPanelWidget>(Root))
		{
			for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
			{
				CollectTexts(Panel->GetChildAt(Index), Out);
			}
		}
	}
}
TSharedRef<SWidget> UMyBondageLifeFriendRowWidget::RebuildWidget()
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

void UMyBondageLifeFriendRowWidget::BuildLayout()
{
	NameText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Name")));
	StatusText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Status")));
	RoleText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Role")));
	ClassText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Class")));
	AvatarImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("W_Avatar")));
	SelectButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_Select")));
	MenuButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_MenuButton")));

	// Positional fallback: if the design did not name the labels, use tree order
	// (name, role/class, status), which matches the usual row layout.
	if (!NameText || !ClassText || !RoleText || !StatusText)
	{
		TArray<UTextBlock*> Texts;
		CollectTexts(WidgetTree->RootWidget, Texts);

		if (!NameText && Texts.IsValidIndex(0)) { NameText = Texts[0]; }
		if (Texts.Num() >= 4)
		{
			if (!ClassText && Texts.IsValidIndex(1)) { ClassText = Texts[1]; }
			if (!RoleText && Texts.IsValidIndex(2)) { RoleText = Texts[2]; }
			if (!StatusText && Texts.IsValidIndex(3)) { StatusText = Texts[3]; }
		}
		else
		{
			// Three labels: name / class / status (关系 needs its own W_Role label).
			if (!ClassText && Texts.IsValidIndex(1)) { ClassText = Texts[1]; }
			if (!StatusText && Texts.IsValidIndex(2)) { StatusText = Texts[2]; }
		}
	}

	if (IsValid(SelectButton))
	{
		SelectButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeFriendRowWidget::HandleRowClicked);
	}
	if (IsValid(MenuButton))
	{
		MenuButton->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeFriendRowWidget::HandleMenuClicked);
	}
}

void UMyBondageLifeFriendRowWidget::SetFriendEntry(const FMyBondageLifeFriendEntry& InEntry, bool bInSelected)
{
	Entry = InEntry;
	SetSelected(bInSelected);
}

void UMyBondageLifeFriendRowWidget::SetSelected(bool bInSelected)
{
	bSelected = bInSelected;

	if (NameText)
	{
		NameText->SetText(FText::FromString(Entry.CharacterName.IsEmpty() ? Entry.AccountName : Entry.CharacterName));
		NameText->SetColorAndOpacity(FSlateColor(bSelected
			? FLinearColor(1.0f, 0.85f, 0.35f)
			: FLinearColor(0.95f, 0.95f, 0.95f)));
	}

	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Entry.bOnline
			? TEXT("\u5728\u7EBF")
			: TEXT("\u79BB\u7EBF")));
		StatusText->SetColorAndOpacity(FSlateColor(Entry.bOnline
			? FLinearColor(0.45f, 0.95f, 0.55f)
			: FLinearColor(0.6f, 0.6f, 0.62f)));
	}

	if (ClassText)
	{
		ClassText->SetText(FText::FromString(
			UMyBondageLifeAccountSubsystem::MakeCharacterClassLabel(Entry.CharacterClass)));
		ClassText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.9f, 0.92f)));
	}

	if (RoleText)
	{
		// 显示"对方在我的关系里是什么身份"，所以要和 ContractRole 反过来。
		// The service reports the state explicitly; the timestamp is only a fallback.
		const bool bTrial = Entry.ContractStatus.Equals(TEXT("trial"), ESearchCase::IgnoreCase)
			|| (Entry.ContractStatus.IsEmpty() && Entry.ContractTrialEndsAt.GetTicks() > 0
				&& FDateTime::UtcNow() < Entry.ContractTrialEndsAt);

		FString RoleLabel;
		FLinearColor RoleColor(0.35f, 0.35f, 0.38f);
		switch (Entry.ContractRole)
		{
		case EMyBondageLifeContractRole::Master:
			// 我是主人 → 对方是我的奴隶
			RoleLabel = bTrial ? TEXT("\u4E34\u65F6\u5974\u96B6") : TEXT("\u5974\u96B6");
			RoleColor = FLinearColor(0.45f, 0.2f, 0.6f);
			break;
		case EMyBondageLifeContractRole::Slave:
			// 我是奴隶 → 对方是我的主人
			RoleLabel = bTrial ? TEXT("\u4E34\u65F6\u4E3B\u4EBA") : TEXT("\u4E3B\u4EBA");
			RoleColor = FLinearColor(0.55f, 0.25f, 0.75f);
			break;
		default:
			RoleLabel = TEXT("\u65E0");
			break;
		}
		RoleText->SetText(FText::FromString(RoleLabel));
		RoleText->SetColorAndOpacity(FSlateColor(RoleColor));
	}
}

void UMyBondageLifeFriendRowWidget::HandleRowClicked()
{
	OnRowClicked.Broadcast(Entry.AccountName);
}

void UMyBondageLifeFriendRowWidget::HandleMenuClicked()
{
	OnRowMenuClicked.Broadcast(Entry.AccountName);
}

void UMyBondageLifeFriendRowWidget::HandleRestraintsChanged()
{
}
