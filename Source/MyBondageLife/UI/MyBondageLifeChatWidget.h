// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "MyBondageLifeChatWidget.generated.h"

class UCanvasPanel;
class UScrollBox;

/** Raised when the player submits a line of chat with Enter. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeChatMessageSubmitted, const FString&, Message);

/** Raised when text entry ends, either by submitting or by cancelling. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMyBondageLifeChatInputEnded);

/**
 *  Bottom-left chat log plus its input line.
 *
 *  The log is a scroll box that always shows the newest line. Enter puts the
 *  widget into text entry mode (the player controller stops feeding the pawn and
 *  focuses the box); Enter again submits, Escape cancels and clears.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeChatWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UMyBondageLifeChatWidget(const FObjectInitializer& ObjectInitializer);

	/** Appends "[Sender] Message" and scrolls to the newest line. */
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void AddChatMessage(const FString& SenderName, const FString& Message);

	/** Appends a system line (join/leave notice) without a sender prefix. */
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void AddSystemMessage(const FString& Message);

	/** Enables and focuses the input box. */
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void BeginTextInput();

	/** Leaves text entry mode; clears the box when bClearText is true. */
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void EndTextInput(bool bClearText);

	UFUNCTION(BlueprintPure, Category = "Chat")
	bool IsTextInputActive() const { return bTextInputActive; }

	/** Current contents of the input box. */
	UFUNCTION(BlueprintPure, Category = "Chat")
	FString GetCurrentText() const;


	UPROPERTY(BlueprintAssignable, Category = "Chat")
	FMyBondageLifeChatMessageSubmitted OnMessageSubmitted;

	UPROPERTY(BlueprintAssignable, Category = "Chat")
	FMyBondageLifeChatInputEnded OnInputEnded;

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Escape while typing cancels input here, before it reaches the game input stack. */
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	void BuildLayout();

	/** Enter inside the box: submit and leave text mode. */
	UFUNCTION()
	void HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);


	/** Creates one coloured, auto-wrapping line and keeps the log scrolled down. */
	void AppendLogLine(const FString& LineText, const FLinearColor& LineColor);
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<class USizeBox> PanelSize;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> ChatLog;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> ChatInput;

	bool bTextInputActive = false;

	/** Oldest lines are dropped past this count. */
	static constexpr int32 MaxChatLines = 200;
};
