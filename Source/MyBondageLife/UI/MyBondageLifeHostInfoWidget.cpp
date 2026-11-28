// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLife.h"
#include "MyBondageLifeHostInfoWidget.h"
#include "MyBondageLifeUIStyle.h"

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

	const bool bPureCPP = GetClass() == UMyBondageLifeHostInfoWidget::StaticClass();
	// 不再生成代码 UI：文本由主 HUD 的 W_RoomInfoHost_Text 提供。
	(void)bPureCPP;

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
		InfoFont.FontObject = MyBondageLifeUI::LoadUIFont();
		InfoLabel->SetFont(InfoFont);
	}
	Background->AddChild(InfoLabel);
}

void UMyBondageLifeHostInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 使用 WBP 里的 W_RoomInfoHost_Text（没有才回退代码创建的文本）。
	if (WidgetTree)
	{
		if (UTextBlock* Named = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("W_RoomInfoHost_Text"))))
		{
			InfoLabel = Named;
		}
		else
		{
			UE_LOG(LogMyBondageLife, Warning, TEXT("HostInfo: W_RoomInfoHost_Text not found; using code label."));
		}
	}

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
		FString LanguageText = TEXT("\u4E2D\u6587");
		if (Session->GetRoomLanguage() == TEXT("en")) { LanguageText = TEXT("English"); }
		else if (Session->GetRoomLanguage() == TEXT("ko")) { LanguageText = TEXT("\uD55C\uAD6D\uC5B4"); }
		else if (Session->GetRoomLanguage() == TEXT("ja")) { LanguageText = TEXT("\u65E5\u672C\u8A9E"); }

		FString ModeText = TEXT("\u516C\u5F00");
		if (Session->GetRoomVisitMode() == 1) { ModeText = TEXT("\u4EC5\u7BA1\u7406\u5458"); }
		else if (Session->GetRoomVisitMode() == 2) { ModeText = TEXT("\u7BA1\u7406\u5458+\u767D\u540D\u5355"); }

		Text = FString::Printf(TEXT("%s   |   %d/%d   |   %s   |   %s"),
			*Session->GetRoomName(),
			Session->GetPlayerCount(),
			Session->GetMaxPlayers(),
			*LanguageText,
			*ModeText);
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
