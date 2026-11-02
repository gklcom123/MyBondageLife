// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class USkeletalMesh;
class UTexture2D;

/** One row of DT_Outfit (ST_Outfit). */
struct FMyBondageLifeOutfitItem
{
	FName ItemId;
	int32 Slot = 0;
	FString Name;
	TSoftObjectPtr<USkeletalMesh> Mesh;
	TSoftObjectPtr<UTexture2D> Icon;
	TArray<FString> HideBodyParts;
	float FeetOffset = 0.0f;
};

/**
 *  Clothing catalogue: reads /Game/DataTable/DT_Outfit once and answers queries.
 *  Slot indices follow Enum_OutfitSlot (0..10).
 */
class FMyBondageLifeOutfitCatalog
{
public:
	static constexpr int32 SlotCount = 11;

	static void Refresh();
	static void EnsureLoaded();

	static const TArray<FMyBondageLifeOutfitItem>& GetItems();
	static const FMyBondageLifeOutfitItem* Find(FName ItemId);
	static TArray<const FMyBondageLifeOutfitItem*> GetItemsForSlot(int32 Slot);

	/** Display name of a slot ("帽子", "项链", ...). */
	static FString GetSlotName(int32 Slot);

	/** Total feet offset of the given worn items (heels raise the character). */
	static float SumFeetOffsets(const TArray<FName>& ItemIds);

private:
	static TArray<FMyBondageLifeOutfitItem> Items;
	static bool bLoaded;
};
