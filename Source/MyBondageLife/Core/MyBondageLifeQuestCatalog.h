// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** One row of /Game/DataTable/DT_Quest. */
struct FMyBondageLifeQuestDef
{
	/** 行名，唯一任务 ID（如 1001）。 */
	FName Id;

	FString Name;
	FString Description;

	/** 事件类型（如 GagAndSpeak）。 */
	FString QuestString;

	int32 Times = 1;

	bool bDaily = true;

	int32 RewardMoney = 0;
};

class FMyBondageLifeQuestCatalog
{
public:
	static void Refresh();

	static const TArray<FMyBondageLifeQuestDef>& GetItems();
	static const FMyBondageLifeQuestDef* Find(FName Id);
};
