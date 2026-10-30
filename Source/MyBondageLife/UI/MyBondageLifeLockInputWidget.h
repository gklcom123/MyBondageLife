// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeLockInputWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMyBondageLifeLockInputConfirmed, int32, LockType, const FString&, Text);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMyBondageLifeLockInputCancelled);

/**
 *  Duration / password popup for the lock menu.
 *
 *  Designed in /Game/UI/WBP_LockInput (parent = this class).
 *  Optional widgets: W_Input_Title, W_Input_Hint, W_Input_Box, W_Input_Ok, W_Input_Cancel.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeLockInputWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** LockType 2/5 = duration, 4 = password, 104 = password to unlock. */
	UFUNCTION(BlueprintCallable, Category = "Lock")
	void OpenFor(int32 InLockType, int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Lock")
	void CloseInput();

	UFUNCTION(BlueprintImplementableEvent, Category = "Lock")
	void OnLayoutBuilt();

	UPROPERTY(BlueprintAssignable, Category = "Lock")
	FMyBondageLifeLockInputConfirmed OnConfirmed;

	UPROPERTY(BlueprintAssignable, Category = "Lock")
	FMyBondageLifeLockInputCancelled OnCancelled;

	UPROPERTY(BlueprintReadOnly, Category = "Lock", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> InputTitle;

	UPROPERTY(BlueprintReadOnly, Category = "Lock", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> InputHint;

	UPROPERTY(BlueprintReadOnly, Category = "Lock", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEditableTextBox> InputBox;

	UFUNCTION(BlueprintPure, Category = "Lock")
	int32 GetLockType() const { return LockType; }

protected:

	void BuildLayout();

	UFUNCTION()
	void HandleOk();

	UFUNCTION()
	void HandleCancel();

	UPROPERTY(Transient)
	TObjectPtr<UButton> OkButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CancelButton;

	int32 LockType = 0;
	int32 SlotIndex = INDEX_NONE;
	bool bLayoutBuilt = false;
};
