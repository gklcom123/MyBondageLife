// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeQuestPanelWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMyBondageLifeQuestPanelClosed);

/** 任务面板（第一期）：代码搭建，不依赖设计版 WBP。 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeQuestPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(BlueprintAssignable)
	FMyBondageLifeQuestPanelClosed OnClosed;

protected:
	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY(Transient) TObjectPtr<UButton> CloseButton;
};
