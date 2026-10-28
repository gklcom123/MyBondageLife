// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeNameplateLayer.generated.h"

class AActor;
class AMyBondageLifeCharacter;
class UCanvasPanel;
class UTextBlock;
class UMyBondageLifeAccountSubsystem;

/**
 *  Screen-space nameplate layer.
 *
 *  Every character's head is projected to screen coordinates each frame and a text
 *  block is parked exactly on that pixel. Because the position comes from the
 *  projection alone, the plate never drifts when the camera orbits, and the text is
 *  plain 2D - it never rotates.
 *
 *  Driven by AMyBondageLifePlayerController::PlayerTick.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeNameplateLayer : public UUserWidget
{
	GENERATED_BODY()

public:

	UMyBondageLifeNameplateLayer(const FObjectInitializer& ObjectInitializer);

	/** Projects every character's head and moves its plate there. */
	void UpdateNameplates();

protected:

	/** Account state used for the contract name prefix. */
	UMyBondageLifeAccountSubsystem* GetAccountSubsystem() const;

	virtual TSharedRef<SWidget> RebuildWidget() override;

	void BuildLayout();

	/** Returns the label for a character, creating it on first use. */
	UTextBlock* FindOrCreateLabel(AMyBondageLifeCharacter* Character);

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	/** One label per character. */
	UPROPERTY(Transient)
	TMap<TObjectPtr<AMyBondageLifeCharacter>, TObjectPtr<UTextBlock>> Labels;

	/** Cached text per character, so the label is only rewritten when it changes. */
	TMap<TObjectPtr<AMyBondageLifeCharacter>, FString> LabelTexts;
};
