// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/MyBondageLifeAccountSubsystem.h"
#include "MyBondageLifeRuleDetailPanelWidget.generated.h"

class UButton;
class UCheckBox;
class UEditableTextBox;
class UPanelWidget;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMyBondageLifeRuleDetailSaved, const FString&, RuleId, const FMyBondageLifeRuleDetail&, Detail);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMyBondageLifeRuleDetailClosed);

/**
 *  WBP_RuleDetailPanel (parent = this class).
 *  Hooks: W_FieldsHost (any panel), W_Confirm, W_Cancel, W_Close.
 *  Rows are generated from DT_Rule: Detail_Ints / Detail_Strings / Detail_Bools,
 *  each labelled by Detail_Descs.
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeRuleDetailPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	/** Builds the fields for one rule, pre-filled with the current values. */
	UFUNCTION(BlueprintCallable, Category = "Rule")
	void BuildForRule(const FString& InRuleId);

	UPROPERTY(BlueprintAssignable, Category = "Rule")
	FMyBondageLifeRuleDetailSaved OnSaved;

	UPROPERTY(BlueprintAssignable, Category = "Rule")
	FMyBondageLifeRuleDetailClosed OnClosed;

protected:

	UFUNCTION()
	void HandleConfirmClicked();

	UFUNCTION()
	void HandleCancelClicked();

	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY(Transient) TObjectPtr<UPanelWidget> FieldsHost;
	UPROPERTY(Transient) TObjectPtr<UButton> ConfirmButton;
	UPROPERTY(Transient) TObjectPtr<UButton> CancelButton;
	UPROPERTY(Transient) TObjectPtr<UButton> CloseButton;

	UPROPERTY(Transient) TArray<TObjectPtr<UEditableTextBox>> IntBoxes;
	UPROPERTY(Transient) TArray<TObjectPtr<UEditableTextBox>> StringBoxes;
	UPROPERTY(Transient) TArray<TObjectPtr<UCheckBox>> BoolBoxes;

	FString RuleId;
	bool bLayoutBuilt = false;
};
