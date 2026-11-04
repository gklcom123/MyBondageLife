// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeSitHintWidget.generated.h"

class UTextBlock;

/** 靠近椅子时显示的提示："按[G]坐下"。 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeSitHintWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintCallable, Category = "Sit")
	void SetHintText(const FString& InText);

protected:

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HintText;

	bool bLayoutBuilt = false;
};
