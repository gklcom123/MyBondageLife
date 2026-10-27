// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeChatWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "MyBondageLife.h"
#include "UI/MyBondageLifeUIStyle.h"

namespace
{
	constexpr float ChatPanelWidth = 540.0f;
	constexpr float ChatPanelHeight = 300.0f;
	constexpr float ChatLogHeight = 240.0f;
	constexpr float ChatInputHeight = 44.0f;
	constexpr float ChatMargin = 24.0f;
}

UMyBondageLifeChatWidget::UMyBondageLifeChatWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// The layer itself must not swallow clicks; the scroll box below overrides this
	// so the player can drag its scrollbar while holding Alt.
	// SelfHitTestInvisible (not HitTestInvisible): the panel itself must not swallow
	// clicks, but its children - scroll box, scrollbar, input - must stay interactive.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

TSharedRef<SWidget> UMyBondageLifeChatWidget::RebuildWidget()
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

void UMyBondageLifeChatWidget::BuildLayout()
{
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;
	// A full screen overlay must not swallow clicks meant for the layers below it;
	// only its children (buttons, panels) take part in hit testing.
	RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ChatPanelSize"));
	PanelSize->SetWidthOverride(ChatPanelWidth);
	PanelSize->SetHeightOverride(ChatPanelHeight);
	PanelSize->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelSize))
	{
		// Bottom-left corner of the viewport. Auto size is required: without it the
		// canvas slot sizes itself from offsets (0x0) and ignores the size box.
		PanelSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
		PanelSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetPosition(FVector2D(ChatMargin, -ChatMargin));
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ChatColumn"));
	Column->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	PanelSize->AddChild(Column);

	// --- chat log ---------------------------------------------------------
	USizeBox* LogSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ChatLogSize"));
	LogSize->SetHeightOverride(ChatLogHeight);
	LogSize->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UVerticalBoxSlot* LogSlot = Column->AddChildToVerticalBox(LogSize))
	{
		LogSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		LogSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	UBorder* LogBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ChatLogBackground"));
	LogBackground->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.35f));
	LogBackground->SetPadding(FMargin(8.0f, 6.0f));
	LogSize->AddChild(LogBackground);

	ChatLog = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ChatLog"));
	// Hit-testable so the scrollbar can be dragged while Alt shows the cursor.
	ChatLog->SetVisibility(ESlateVisibility::Visible);
	// Thicker scrollbar so it is easy to grab while Alt frees the mouse.
	ChatLog->SetScrollbarThickness(FVector2D(14.0f, 14.0f));
	ChatLog->SetScrollBarVisibility(ESlateVisibility::Visible);
	LogBackground->AddChild(ChatLog);

	// --- input line -------------------------------------------------------
	USizeBox* InputSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ChatInputSize"));
	InputSize->SetWidthOverride(ChatPanelWidth);
	InputSize->SetHeightOverride(ChatInputHeight);
	InputSize->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UVerticalBoxSlot* InputSlot = Column->AddChildToVerticalBox(InputSize))
	{
		InputSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	ChatInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("ChatInput"));
	ChatInput->SetHintText(FText::FromString(TEXT("\u6309 Enter \u8F93\u5165\u804A\u5929")));
	MyBondageLifeUI::ApplyReadableTextInputStyle(ChatInput);
	ChatInput->SetIsReadOnly(true);
	ChatInput->SetVisibility(ESlateVisibility::Visible);
	ChatInput->OnTextCommitted.AddDynamic(this, &UMyBondageLifeChatWidget::HandleTextCommitted);
	InputSize->AddChild(ChatInput);
}

void UMyBondageLifeChatWidget::AppendLogLine(const FString& LineText, const FLinearColor& LineColor)
{
	if (!ChatLog)
	{
		return;
	}

	// Drop the oldest lines once the log gets too long.
	while (ChatLog->GetChildrenCount() >= MaxChatLines)
	{
		ChatLog->RemoveChildAt(0);
	}

	UTextBlock* Line = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Line->SetText(FText::FromString(LineText));
	Line->SetColorAndOpacity(FSlateColor(LineColor));
	Line->SetAutoWrapText(true);
	Line->SetShadowOffset(FVector2D(1.0f, 1.0f));
	Line->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
	{
		FSlateFontInfo LineFont = Line->GetFont();
		LineFont.Size = 16;
		Line->SetFont(LineFont);
	}

	if (UScrollBoxSlot* LineSlot = Cast<UScrollBoxSlot>(ChatLog->AddChild(Line)))
	{
		LineSlot->SetPadding(FMargin(0.0f, 1.0f));
	}

	// Always show the newest line.
	ChatLog->ScrollToEnd();
}

void UMyBondageLifeChatWidget::AddChatMessage(const FString& SenderName, const FString& Message)
{
	AppendLogLine(FString::Printf(TEXT("%s: %s"), *SenderName, *Message),
		FLinearColor(0.92f, 0.95f, 1.0f));
}

void UMyBondageLifeChatWidget::AddSystemMessage(const FString& Message)
{
	// Join/leave notices stand out in amber and carry no sender prefix.
	AppendLogLine(Message, FLinearColor(0.96f, 0.80f, 0.42f));
}

void UMyBondageLifeChatWidget::BeginTextInput()
{
	if (!ChatInput)
	{
		return;
	}

	bTextInputActive = true;

	ChatInput->SetText(FText::GetEmpty());
	ChatInput->SetIsReadOnly(false);
	ChatInput->SetKeyboardFocus();
}

void UMyBondageLifeChatWidget::EndTextInput(bool bClearText)
{
	if (!ChatInput)
	{
		return;
	}

	const bool bWasActive = bTextInputActive;
	bTextInputActive = false;

	if (bClearText)
	{
		ChatInput->SetText(FText::GetEmpty());
	}

	ChatInput->SetIsReadOnly(true);

	if (bWasActive)
	{
		OnInputEnded.Broadcast();
	}
}

void UMyBondageLifeChatWidget::HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{

	if (!bTextInputActive || CommitMethod != ETextCommit::OnEnter)
	{
		return;
	}

	const FString Message = Text.ToString().TrimStartAndEnd();

	bTextInputActive = false;
	ChatInput->SetIsReadOnly(true);
	ChatInput->SetText(FText::GetEmpty());

	if (!Message.IsEmpty())
	{
		OnMessageSubmitted.Broadcast(Message);
	}

	OnInputEnded.Broadcast();
}

FString UMyBondageLifeChatWidget::GetCurrentText() const
{
	return ChatInput ? ChatInput->GetText().ToString() : FString();
}

FReply UMyBondageLifeChatWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Escape cancels text entry here, before the key reaches the game input stack.
	// Handling it at the widget level means one press is enough even when the
	// editable text box (or the IME) would otherwise eat the first one.
	if (bTextInputActive && InKeyEvent.GetKey() == EKeys::Escape)
	{
		EndTextInput(true);
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}