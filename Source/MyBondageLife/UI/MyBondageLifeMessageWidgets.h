// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeMessageWidgets.generated.h"

class UButton;
class UCanvasPanel;
class UScrollBox;
class UTextBlock;
class UWidget;
class UBorder;

/**
 *  HUD entry for the message center: chat-adjacent icon plus a red dot when there
 *  are pending notifications.
 *
 *  Designed in /Game/UI/WBP_MessageButton: W_Entry (button), W_RedDot (any widget).
 */
/** Routes a code-built row button to the notification it belongs to. */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeMessageButtonProxy : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY()
	TObjectPtr<class UMyBondageLifeAccountSubsystem> Subsystem;

	FString NotificationId;
	bool bAccept = false;

	UFUNCTION()
	void HandleClicked();
};
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeMessageButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(BlueprintReadOnly, Category = "Message", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UButton> EntryButton;

protected:

	void BuildLayout();

	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleNotificationsChanged();

	UPROPERTY(Transient)
	TObjectPtr<UWidget> RedDot;

	bool bLayoutBuilt = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMyBondageLifeMessageButtonProxy>> RowProxies;
};

/**
 *  One notification row.
 *
 *  Designed in /Game/UI/WBP_MessageRow: W_Text, W_Accept, W_Decline (buttons).
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeMessageRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintCallable, Category = "Message")
	void SetNotification(const FString& InNotificationId, const FString& InText, bool bInContractKind);

	UFUNCTION(BlueprintPure, Category = "Message")
	FString GetNotificationId() const { return NotificationId; }

protected:

	void BuildLayout();

	UFUNCTION()
	void HandleAccept();

	UFUNCTION()
	void HandleDecline();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> AcceptButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> DeclineButton;

	FString NotificationId;
	FString PendingMessageText;
	bool bHasPendingMessage = false;
	bool bLayoutBuilt = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMyBondageLifeMessageButtonProxy>> RowProxies;
};

/**
 *  Message center panel (list of pending notifications).
 *
 *  Designed in /Game/UI/WBP_MessageCenter: W_Panel, W_List (ScrollBox),
 *  W_RowTemplate (a WBP_MessageRow instance, hidden).
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeMessageCenterWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintCallable, Category = "Message")
	void OpenCenter();

	UFUNCTION(BlueprintCallable, Category = "Message")
	void CloseCenter();

	UFUNCTION(BlueprintCallable, Category = "Message")
	void SetPanelOpen(bool bOpen);

	UFUNCTION(BlueprintCallable, Category = "Message")
	void TogglePanel();

	UFUNCTION(BlueprintPure, Category = "Message")
	bool IsPanelOpen() const;


protected:

	void BuildLayout();
	void RefreshList();
	void HidePrototypeRows();

	UFUNCTION()
	void HandleNotificationsChanged();

	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> PanelBorder;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> ListBox;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UMyBondageLifeMessageRowWidget> RowTemplate;

	UPROPERTY(EditDefaultsOnly, Category = "Message")
	TSubclassOf<UMyBondageLifeMessageRowWidget> RowClass;

	bool bLayoutBuilt = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMyBondageLifeMessageButtonProxy>> RowProxies;
};
