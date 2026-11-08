// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeHudEntryWidget.h"
#include "MyBondageLifeHudButtonWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeHudButtonClicked, EMyBondageLifeHudEntryAction, Action);

/**
 *  Bottom-row HUD button (WBP_HudButton, parent = this class).
 *
 *  Designed hooks (all optional, a default look is built when missing):
 *      W_Button : UButton  - the clickable area
 *      W_Icon   : UImage   - icon (filled from EntryAction)
 *      W_Label  : UTextBlock - caption (filled from EntryAction)
 *      W_RedDot : UImage   - optional notification dot
 *
 *  Each placed instance picks its behaviour through the *instance editable*
 *  EntryAction property, so one designed asset serves the whole row.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeHudButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Which HUD entry this button represents (editable per placed instance). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hud Button")
	EMyBondageLifeHudEntryAction EntryAction = EMyBondageLifeHudEntryAction::Message;

	UFUNCTION(BlueprintCallable, Category = "Hud Button")
	void SetEntryAction(EMyBondageLifeHudEntryAction InAction);

	UFUNCTION(BlueprintCallable, Category = "Hud Button")
	void SetRedDotVisible(bool bVisible);

	virtual void NativeConstruct() override;

	UFUNCTION()
	void RefreshRedDot();

	/** Re-applies icon/caption from EntryAction. */
	UFUNCTION(BlueprintCallable, Category = "Hud Button")
	void RefreshAppearance();

	UPROPERTY(BlueprintAssignable, Category = "Hud Button")
	FMyBondageLifeHudButtonClicked OnClicked;

protected:

	UFUNCTION()
	void HandleClicked();

	UPROPERTY(Transient) TObjectPtr<UButton> EntryButton;
	UPROPERTY(Transient) TObjectPtr<UImage> EntryIcon;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> EntryLabel;
	UPROPERTY(Transient) TObjectPtr<UImage> RedDot;

	bool bLayoutBuilt = false;
};
