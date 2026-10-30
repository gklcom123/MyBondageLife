// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifePlayerInfoEntryWidget.generated.h"

class UButton;
class UTexture2D;

/**
 *  HUD entry "个人信息（C）", split out so it can be designed in
 *  /Game/UI/WBP_PlayerInfoEntry (parent = this class). Put the button in W_Entry.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifePlayerInfoEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "PlayerInfo")
	void OnLayoutBuilt();

	UPROPERTY(BlueprintReadOnly, Category = "PlayerInfo", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UButton> EntryButton;

protected:

	void BuildLayout();

	UFUNCTION()
	void HandleEntryClicked();

	UPROPERTY(EditDefaultsOnly, Category = "PlayerInfo")
	TObjectPtr<UTexture2D> IconTexture;

	bool bLayoutBuilt = false;
};
