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
	PlayerInfo,
	FriendList
};

/**
 *  Small HUD entry ("个人信息（C）" / "好友列表（V）") used by the main HUD.
 *
 *  Designed in /Game/UI/WBP_HudEntry (parent = this class): W_Entry (button),
 *  optional W_Icon (image) and W_Label (text). A plain default look is built when the
 *  asset does not provide them.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeHudEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetEntryKind(EMyBondageLifeHudEntryAction InAction);

protected:

	void BuildLayout();

	UFUNCTION()
	void HandleClicked();

	UPROPERTY(Transient)
	TObjectPtr<UButton> EntryButton;

	UPROPERTY(Transient)
	TObjectPtr<UImage> EntryIcon;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EntryLabel;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	EMyBondageLifeHudEntryAction Action = EMyBondageLifeHudEntryAction::PlayerInfo;

	bool bLayoutBuilt = false;
};
