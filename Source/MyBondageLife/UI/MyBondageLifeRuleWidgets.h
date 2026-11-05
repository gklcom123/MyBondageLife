// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyBondageLifeRuleWidgets.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMyBondageLifeRuleSwitchRequested, const FString&, RuleId, bool, bWantEnabled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyBondageLifeRuleDetailRequested, const FString&, RuleId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMyBondageLifeRulePanelClosed);

/** One rule line (WBP_RuleRow). Hooks: W_Name, W_Desc, W_Switch, W_DetailButton. */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeRuleRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Rule")
	void SetRule(const FString& InRuleId, const FString& InName, const FString& InDesc,
		bool bInEnabled, bool bInCanEdit, bool bHasDetailPanel);

	FString GetRuleId() const { return RuleId; }

	UPROPERTY(BlueprintAssignable, Category = "Rule")
	FMyBondageLifeRuleSwitchRequested OnSwitchRequested;

	UPROPERTY(BlueprintAssignable, Category = "Rule")
	FMyBondageLifeRuleDetailRequested OnDetailRequested;

protected:

	UFUNCTION()
	void HandleSwitchClicked();

	UFUNCTION()
	void HandleDetailClicked();

	FString RuleId;
	bool bEnabled = false;
	bool bCanEdit = false;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DescText;
	UPROPERTY(Transient) TObjectPtr<UButton> SwitchButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SwitchLabel;
	UPROPERTY(Transient) TObjectPtr<UButton> DetailButton;
	bool bLayoutBuilt = false;
};

/** Rule panel (WBP_RulePanel). Hooks: W_RuleList (any panel), W_Close. */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeRulePanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Rule")
	void PopulateRules(const FString& ForAccount);

	UPROPERTY(BlueprintAssignable, Category = "Rule")
	FMyBondageLifeRuleSwitchRequested OnSwitchRequested;

	UPROPERTY(BlueprintAssignable, Category = "Rule")
	FMyBondageLifeRuleDetailRequested OnDetailRequested;

	UPROPERTY(BlueprintAssignable, Category = "Rule")
	FMyBondageLifeRulePanelClosed OnClosed;

protected:

	UFUNCTION()
	void HandleRowSwitch(const FString& RuleId, bool bWantEnabled);

	UFUNCTION()
	void HandleRowDetail(const FString& RuleId);

	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY(Transient) TObjectPtr<UPanelWidget> RuleList;
	UPROPERTY(Transient) TObjectPtr<UButton> CloseButton;
	UPROPERTY(Transient) TArray<TObjectPtr<UMyBondageLifeRuleRowWidget>> Rows;
	FString ViewAccount;
	bool bLayoutBuilt = false;
};
