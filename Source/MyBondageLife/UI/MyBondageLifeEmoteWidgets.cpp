// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MyBondageLifeEmoteWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/MyBondageLifeEmoteCatalog.h"
#include "Engine/Texture2D.h"
#include "MyBondageLife.h"

// ---------------------------------------------------------------------------
// Row
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UMyBondageLifeEmoteRowWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (!IsDesignTime() && WidgetTree && !bLayoutBuilt)
	{
		bLayoutBuilt = true;

		Button = Cast<UButton>(WidgetTree->FindWidget(TEXT("W_BtnImg")));
		NameText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_Name")));

		if (!Button)
		{
			Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("W_BtnImg"));
			WidgetTree->RootWidget = Button;
		}

		if (Button)
		{
			IconImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("W_Icon")));
			if (!IconImage)
			{
				IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("W_Icon"));
				Button->AddChild(IconImage);
			}
			Button->OnClicked.AddUniqueDynamic(this, &UMyBondageLifeEmoteRowWidget::HandleClicked);
		}

		if (!NameText)
		{
			NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("W_Name"));
			NameText->SetJustification(ETextJustify::Center);
			if (UVerticalBox* Column = Cast<UVerticalBox>(WidgetTree->RootWidget))
			{
				Column->AddChildToVerticalBox(NameText);
			}
		}
	}

	return Super::RebuildWidget();
}

void UMyBondageLifeEmoteRowWidget::SetEmote(FName InEmoteId, const FString& InName, UTexture2D* Icon)
{
	EmoteId = InEmoteId;

	if (NameText)
	{
		NameText->SetText(FText::FromString(InName));
	}
	if (IconImage && Icon)
	{
		IconImage->SetBrushFromTexture(Icon, true);
	}
}

void UMyBondageLifeEmoteRowWidget::HandleClicked()
{
	if (!EmoteId.IsNone())
	{
		OnPicked.Broadcast(EmoteId);
	}
}

// ---------------------------------------------------------------------------
// List
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UMyBondageLifeEmoteListWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (!IsDesignTime() && WidgetTree && !bLayoutBuilt)
	{
		bLayoutBuilt = true;

		EmoteScroll = Cast<UScrollBox>(WidgetTree->FindWidget(TEXT("W_EmoteScrollBox")));
		if (!EmoteScroll)
		{
			EmoteScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("W_EmoteScrollBox"));
			WidgetTree->RootWidget = EmoteScroll;
		}
	}

	return Super::RebuildWidget();
}

void UMyBondageLifeEmoteListWidget::PopulateEmotes()
{
	if (!EmoteScroll)
	{
		return;
	}

	EmoteScroll->ClearChildren();
	Rows.Reset();

	UClass* RowClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_Emote.WBP_Emote_C"));
	if (!RowClass)
	{
		RowClass = UMyBondageLifeEmoteRowWidget::StaticClass();
	}

	for (const FMyBondageLifeEmoteItem& Item : FMyBondageLifeEmoteCatalog::GetItems())
	{
		UMyBondageLifeEmoteRowWidget* Row = CreateWidget<UMyBondageLifeEmoteRowWidget>(this, RowClass);
		if (!Row)
		{
			continue;
		}

		Row->SetEmote(Item.EmoteId, Item.Name, Item.Icon.LoadSynchronous());
		Row->OnPicked.AddUniqueDynamic(this, &UMyBondageLifeEmoteListWidget::HandleRowPicked);

		if (UScrollBoxSlot* RowSlot = Cast<UScrollBoxSlot>(EmoteScroll->AddChild(Row)))
		{
			RowSlot->SetPadding(FMargin(4.0f));
		}
		Rows.Add(Row);
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("Emote: list shows %d row(s)."), Rows.Num());
}

void UMyBondageLifeEmoteListWidget::HandleRowPicked(FName EmoteId)
{
	OnEmotePicked.Broadcast(EmoteId);
}