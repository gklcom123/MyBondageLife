// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeHostInfoWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Network/MyBondageLifeSessionSubsystem.h"
#include "TimerManager.h"

UMyBondageLifeHostInfoWidget::UMyBondageLifeHostInfoWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Decorative overlay: never eat clicks meant for the game.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

TSharedRef<SWidget> UMyBondageLifeHostInfoWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}

	return Super::RebuildWidget();
}

void UMyBondageLifeHostInfoWidget::BuildLayout()
{
	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
	Background->SetPadding(FMargin(14.0f, 8.0f));

	if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(Background))
	{
		// Anchored to the top-right corner with a small margin.
		PanelSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
		PanelSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetPosition(FVector2D(-24.0f, 24.0f));
	}

	InfoLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InfoLabel"));
	InfoLabel->SetText(FText::FromString(TEXT("HOST  ---")));
	InfoLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.97f, 1.0f)));
	{
		FSlateFontInfo InfoFont = InfoLabel->GetFont();
		InfoFont.Size = 18;
		InfoLabel->SetFont(InfoFont);
	}
	Background->AddChild(InfoLabel);
}

void UMyBondageLifeHostInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshInfo();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RefreshTimerHandle, FTimerDelegate::CreateUObject(this, &UMyBondageLifeHostInfoWidget::RefreshInfo), 0.5f, true);
	}
}

void UMyBondageLifeHostInfoWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimerHandle);
	}

	Super::NativeDestruct();
}

void UMyBondageLifeHostInfoWidget::RefreshInfo()
{
	if (!InfoLabel)
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UMyBondageLifeSessionSubsystem* Session = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeSessionSubsystem>()
		: nullptr;

	FString Text;
	if (!Session || !Session->IsInSession())
	{
		Text = TEXT("---");
	}
	else
	{
		// "<角色名>的房间   |   2/8": the endpoint stays hidden, the player count remains.
		Text = FString::Printf(TEXT("%s   |   %d/%d"),
			*Session->GetRoomName(),
			Session->GetPlayerCount(),
			Session->GetMaxPlayers());
	}

	// Only touch the text when it actually changed, which happens when someone
	// joins or leaves the room (or when the session ends).
	if (Text == LastInfoText)
	{
		return;
	}

	LastInfoText = Text;
	InfoLabel->SetText(FText::FromString(Text));
}
