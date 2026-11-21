// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeAddQuestRowWidget.generated.h"

class UButton;
class UTextBlock;
class UMyBondageLifeQuestRowProxy;

/** 纯代码构建的候选任务行（用于验证设计版 WBP 是否存在问题）。 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeAddQuestRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** 在 CreateWidget 之后、AddChild 之前设置。 */
	FString InfoText;
	FString RewardText;
	FString ActionText = TEXT("添加");
	bool bActionEnabled = true;

	UPROPERTY()
	TObjectPtr<UMyBondageLifeQuestRowProxy> Proxy;

protected:
	UFUNCTION()
	void HandleAcceptClicked();

	UPROPERTY(Transient) TObjectPtr<UButton> AcceptButton;
};
