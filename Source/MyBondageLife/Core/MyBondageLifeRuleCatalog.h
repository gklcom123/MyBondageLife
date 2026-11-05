// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UUserWidget;

/** One row of DT_Rule. */
struct FMyBondageLifeRuleItem
{
	FString RuleId;          // == Rule_Effect_String
	FString Name;
	FString Desc;
	TSoftClassPtr<UUserWidget> DetailPanel;
};

/**
 *  Slave-rule catalogue: reads /Game/DataTable/DT_Rule once.
 *  RuleId is the Rule_Effect_String column and is what the server stores.
 */
class FMyBondageLifeRuleCatalog
{
public:
	static void Refresh();
	static void EnsureLoaded();

	static const TArray<FMyBondageLifeRuleItem>& GetItems();
	static const FMyBondageLifeRuleItem* Find(const FString& RuleId);
	static const FMyBondageLifeRuleItem* FindByEffect(const FString& Effect);

private:
	static TArray<FMyBondageLifeRuleItem> Items;
};
