// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** One row of DT_Rule. */
struct FMyBondageLifeRuleItem
{
	FString RuleId;          // == Rule_Effect_String (server-side identifier)
	FString Name;
	FString Desc;

	/** Optional detail fields configured in the data table. */
	int32 DetailIntCount = 0;
	int32 DetailStringCount = 0;
	int32 DetailBoolCount = 0;
	TArray<FString> DetailDescs;

	/** True when the rule has any editable detail parameter. */
	bool HasDetail() const
	{
		return (DetailIntCount + DetailStringCount + DetailBoolCount) > 0;
	}
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

private:
	static TArray<FMyBondageLifeRuleItem> Items;
};
