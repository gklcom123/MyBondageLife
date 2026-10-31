// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeHudEntryWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;

UENUM(BlueprintType)
enum class EMyBondageLifeHudEntryAction : uint8
{
	Message,
	PlayerInfo,
	FriendList,
	Wardrobe
};

/**
 *  One HUD entry: icon + caption on a single row-less template, shared by the message
 *  center, player info, friend list and wardrobe buttons.
 *
 *  The widget always reports exactly the size of its content (icon + caption), so the
 *  main HUD can pin it to the bottom-left corner of its host box.
 *
 *  Designed in /Game/UI/WBP_HudEntry (parent = this class): W_Entry (button),
 *  W_Icon (image), W_Label (text). A default look is built when they are missing.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeHudEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Configures icon, caption and click action for this entry. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetEntryKind(EMyBondageLifeHudEntryAction InAction);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetRedDotVisible(bool bVisible);

	UFUNCTION(BlueprintPure, Category = "HUD")
	EMyBondageLifeHudEntryAction GetEntryAction() const { return Action; }

protected:

	void BuildLayout();
	void BuildTree();
	void RefreshAppearance();

	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleNotificationsChanged();

	UPROPERTY(Transient)
	TObjectPtr<UButton> EntryButton;

	UPROPERTY(Transient)
	TObjectPtr<UImage> EntryIcon;

	UPROPERTY(Transient)
	TObjectPtr<UImage> RedDot;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EntryLabel;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	EMyBondageLifeHudEntryAction Action = EMyBondageLifeHudEntryAction::PlayerInfo;

	bool bLayoutBuilt = false;
};
