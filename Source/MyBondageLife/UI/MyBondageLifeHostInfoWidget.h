// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeHostInfoWidget.generated.h"

class UTextBlock;

/**
 *  Small always-on top-right overlay that shows which host the local player is
 *  connected to (its own address when hosting).
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeHostInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UMyBondageLifeHostInfoWidget(const FObjectInitializer& ObjectInitializer);

	/** 由 PlayerController 传入主 HUD 里的 W_RoomInfoHost_Text。 */
	void SetExternalLabel(UTextBlock* InLabel) { InfoLabel = InLabel; }

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void BuildLayout();
	void RefreshInfo();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InfoLabel;

	/** Last text pushed to the label; the widget skips identical updates. */
	FString LastInfoText;

	FTimerHandle RefreshTimerHandle;
};
