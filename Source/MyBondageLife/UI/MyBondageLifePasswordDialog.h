// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifePasswordDialog.generated.h"

class UEditableTextBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifePasswordConfirmed, const FString&, Password);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMyBondageLifePasswordCancelled);

/** 简洁的房间密码输入框（代码构建）。 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifePasswordDialog : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(BlueprintAssignable) FMyBondageLifePasswordConfirmed OnConfirmed;
	UPROPERTY(BlueprintAssignable) FMyBondageLifePasswordCancelled OnCancelled;

protected:
	UFUNCTION() void HandleOkClicked();
	UFUNCTION() void HandleCancelClicked();

	UPROPERTY(Transient) TObjectPtr<UEditableTextBox> PasswordInput;
};
