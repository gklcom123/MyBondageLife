// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeGameMenuWidget.generated.h"

class UButton;
class UCanvasPanel;
class UVerticalBox;

/**
 *  In-game menu, opened with Escape.
 *
 *  Buttons: 游戏设置 (placeholder), 返回菜单 (leaves the room and goes back to the
 *  main scene), 退出游戏 (closes the process). While it is up the player
 *  controller keeps the pawn's input disabled, so the game cannot be played.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeGameMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UMyBondageLifeGameMenuWidget(const FObjectInitializer& ObjectInitializer);

	/** Leaves the current room and returns to the main scene. */
	UFUNCTION(BlueprintCallable, Category = "GameMenu")
	void ReturnToMenu();

	/** Closes the game process. */
	UFUNCTION(BlueprintCallable, Category = "GameMenu")
	void QuitGameProcess();

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	void BuildLayout();

	UButton* CreateMenuButton(UVerticalBox* Parent, const FString& LabelText, float Height, bool bInteractive);

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleReturnClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;
};
