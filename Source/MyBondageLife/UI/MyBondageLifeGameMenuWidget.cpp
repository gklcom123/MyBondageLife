// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeGameMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MyBondageLife.h"
#include "Network/MyBondageLifeSessionSubsystem.h"

namespace
{
	constexpr float MenuButtonWidth = 420.0f;
	constexpr float MenuButtonHeight = 64.0f;
}

UMyBondageLifeGameMenuWidget::UMyBondageLifeGameMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Visible);
}

TSharedRef<SWidget> UMyBondageLifeGameMenuWidget::RebuildWidget()
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

void UMyBondageLifeGameMenuWidget::BuildLayout()
{
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	// Dims the game behind the menu and swallows stray clicks.
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.82f));
	if (UCanvasPanelSlot* BackdropSlot = RootCanvas->AddChildToCanvas(Backdrop))
	{
		BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackdropSlot->SetOffsets(FMargin(0.0f));
		BackdropSlot->SetZOrder(0);
	}

	UVerticalBox* CenterBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CenterBox"));
	if (UCanvasPanelSlot* CenterSlot = RootCanvas->AddChildToCanvas(CenterBox))
	{
		CenterSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		CenterSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CenterSlot->SetAutoSize(true);
		CenterSlot->SetZOrder(1);
	}

	// Title: 菜单
	UTextBlock* TitleLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleLabel"));
	TitleLabel->SetText(FText::FromString(TEXT("\u83DC\u5355")));
	TitleLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TitleLabel->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo TitleFont = TitleLabel->GetFont();
		TitleFont.Size = 44;
		TitleLabel->SetFont(TitleFont);
	}
	if (UVerticalBoxSlot* TitleSlot = CenterBox->AddChildToVerticalBox(TitleLabel))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 32.0f));
	}

	// 游戏设置 - visible but not implemented yet.
	UButton* SettingsButton = CreateMenuButton(CenterBox, TEXT("\u6E38\u620F\u8BBE\u7F6E"), MenuButtonHeight, false);
	SettingsButton->SetToolTipText(FText::FromString(TEXT("\u5C1A\u672A\u5B9E\u73B0")));
	SettingsButton->OnClicked.AddDynamic(this, &UMyBondageLifeGameMenuWidget::HandleSettingsClicked);

	// 返回菜单
	UButton* ReturnButton = CreateMenuButton(CenterBox, TEXT("\u8FD4\u56DE\u83DC\u5355"), MenuButtonHeight, true);
	ReturnButton->OnClicked.AddDynamic(this, &UMyBondageLifeGameMenuWidget::HandleReturnClicked);

	// 退出游戏
	UButton* QuitButton = CreateMenuButton(CenterBox, TEXT("\u9000\u51FA\u6E38\u620F"), MenuButtonHeight, true);
	QuitButton->SetToolTipText(FText::FromString(TEXT("\u5173\u95ED\u6E38\u620F\u8FDB\u7A0B")));
	QuitButton->OnClicked.AddDynamic(this, &UMyBondageLifeGameMenuWidget::HandleQuitClicked);

	// Hint: 按 ESC 返回游戏
	UTextBlock* HintLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintLabel"));
	HintLabel->SetText(FText::FromString(TEXT("\u6309 ESC \u8FD4\u56DE\u6E38\u620F")));
	HintLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.62f, 0.68f)));
	HintLabel->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo HintFont = HintLabel->GetFont();
		HintFont.Size = 16;
		HintLabel->SetFont(HintFont);
	}
	if (UVerticalBoxSlot* HintSlot = CenterBox->AddChildToVerticalBox(HintLabel))
	{
		HintSlot->SetHorizontalAlignment(HAlign_Center);
		HintSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
	}
}

UButton* UMyBondageLifeGameMenuWidget::CreateMenuButton(UVerticalBox* Parent, const FString& LabelText, float Height, bool bInteractive)
{
	USizeBox* ButtonSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ButtonSize->SetWidthOverride(MenuButtonWidth);
	ButtonSize->SetHeightOverride(Height);

	if (UVerticalBoxSlot* BoxSlot = Parent->AddChildToVerticalBox(ButtonSize))
	{
		BoxSlot->SetHorizontalAlignment(HAlign_Center);
		BoxSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetIsEnabled(bInteractive);
	ButtonSize->AddChild(Button);

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetText(FText::FromString(LabelText));
	Label->SetJustification(ETextJustify::Center);
	Label->SetColorAndOpacity(FSlateColor(bInteractive
		? FLinearColor(0.02f, 0.02f, 0.02f)
		: FLinearColor(0.45f, 0.45f, 0.48f)));
	{
		FSlateFontInfo LabelFont = Label->GetFont();
		LabelFont.Size = 26;
		Label->SetFont(LabelFont);
	}
	Button->AddChild(Label);

	return Button;
}

void UMyBondageLifeGameMenuWidget::ReturnToMenu()
{
	UE_LOG(LogMyBondageLife, Log, TEXT("GameMenu: 返回菜单 pressed, leaving the room."));

	UGameInstance* GameInstance = GetGameInstance();
	if (UMyBondageLifeSessionSubsystem* Session = GameInstance
		? GameInstance->GetSubsystem<UMyBondageLifeSessionSubsystem>()
		: nullptr)
	{
		// Stops announcing (when hosting) and travels back to the same map as a
		// plain local game, which lands the player on the main scene again.
		Session->LeaveRoom();
	}
}

void UMyBondageLifeGameMenuWidget::QuitGameProcess()
{
	UE_LOG(LogMyBondageLife, Log, TEXT("GameMenu: 退出游戏 pressed, closing the game process."));

	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UMyBondageLifeGameMenuWidget::HandleSettingsClicked()
{
	UE_LOG(LogMyBondageLife, Log, TEXT("GameMenu: 游戏设置 is not implemented yet."));
}

void UMyBondageLifeGameMenuWidget::HandleReturnClicked()
{
	ReturnToMenu();
}

void UMyBondageLifeGameMenuWidget::HandleQuitClicked()
{
	QuitGameProcess();
}
