// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeWardrobeButtonWidget.generated.h"

class UButton;
class UTextBlock;
class UTexture2D;

/**
 *  HUD entry for the wardrobe ("衣柜（B）").
 *
 *  Split out of the wardrobe panel so the panel can be designed on its own in
 *  /Game/UI/WBP_Wardrobe without dragging the HUD button along.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeWardrobeButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Optional designed layout: put the button in W_Entry. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Wardrobe")
	void OnLayoutBuilt();

	UPROPERTY(BlueprintReadOnly, Category = "Wardrobe", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UButton> EntryButton;

protected:

	UFUNCTION()
	void HandleEntryClicked();

	void BuildLayout();

	UPROPERTY(EditDefaultsOnly, Category = "Wardrobe")
	TObjectPtr<UTexture2D> IconTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Wardrobe")
	FString EntryText = TEXT("\u8863\u67DC\uFF08B\uFF09");

	bool bLayoutBuilt = false;
};
