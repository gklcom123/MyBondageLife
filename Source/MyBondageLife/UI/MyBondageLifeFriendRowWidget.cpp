// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeFriendRowWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "MyBondageLife.h"

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
	AvatarImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("W_Avatar")));
	SelectButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_Select")));
	MenuButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_MenuButton")));

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

	if (RoleText)
	{
		RoleText->SetText(FText::FromString(Entry.CharacterClass > 0
			? FString::Printf(TEXT("%d"), Entry.CharacterClass)
			: FString()));
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
