// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UAnimMontage;
class UTexture2D;

/** One row of DT_Emote (ST_Emote). */
struct FMyBondageLifeEmoteItem
{
	FName EmoteId;
	FString Name;
	TSoftObjectPtr<UAnimMontage> Montage;
	int32 StateCondition = 0;      // Enum_CharState bitmask
	bool bArmBound = false;
	bool bLegBound = false;
	TSoftObjectPtr<UTexture2D> Icon;
};

/**
 *  Emote catalogue: reads /Game/DataTable/DT_Emote once.
 *  StateCondition bits: 1=Standing, 2=Sitting(chair), 4=Sitting(ground), 8=PetMode.
 */
class FMyBondageLifeEmoteCatalog
{
public:
	static void Refresh();
	static void EnsureLoaded();

	static const TArray<FMyBondageLifeEmoteItem>& GetItems();
	static const FMyBondageLifeEmoteItem* Find(FName EmoteId);

	/** True when the item's StateCondition allows the given state bit. */
	static bool MatchesState(const FMyBondageLifeEmoteItem& Item, int32 StateBit);

private:
	static TArray<FMyBondageLifeEmoteItem> Items;
};
