// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UAnimMontage;
class USkeletalMesh;
class UTexture2D;

/** One row of /Game/DataTable/DT_Restraint. */
struct FMyBondageLifeRestraintItem
{
	/** Row name, also the id stored on the account (e.g. "1"). */
	FName ItemId;

	/** Display name shown in the wardrobe. */
	FString Name;

	/** Enum_RestraintSlot value as an integer (0..11). */
	int32 Slot = 0;

	TSoftObjectPtr<USkeletalMesh> Mesh;
	TSoftObjectPtr<UTexture2D> Icon;
	TSoftObjectPtr<UAnimMontage> UpAnmi;
	TSoftObjectPtr<UAnimMontage> DownAnmi;

	bool bRestraintArm = false;
	bool bRestraintLeg = false;
	bool bCantMove = false;
	bool bInPetMode = false;
};

/**
 *  Read-only view of the restraint data table.
 *
 *  The table uses a Blueprint struct whose property names carry a rename suffix
 *  ("RestranitArm?_12_AB12..."), so fields are matched by prefix and type.
 */
class MYBONDAGELIFE_API FMyBondageLifeRestraintCatalog
{
public:

	static constexpr int32 SlotCount = 12;

	static void Refresh();
	static const TArray<FMyBondageLifeRestraintItem>& GetItems();

	/** Item by row name; null when unknown. */
	static const FMyBondageLifeRestraintItem* Find(FName ItemId);

	/** Items that belong to one slot, sorted by id. */
	static TArray<const FMyBondageLifeRestraintItem*> GetItemsForSlot(int32 Slot);

	/** Display name of a slot ("嘴部", "项圈", ...). */
	static FString GetSlotName(int32 Slot);

	/** Loads the mesh of an equipped item. */
	static USkeletalMesh* LoadMesh(FName ItemId);

	/** Loads the icon of an item (may be null). */
	static UTexture2D* LoadIcon(FName ItemId);

	/** Name for display, falling back to the row name. */
	static FString GetDisplayName(FName ItemId);

private:

	static void EnsureLoaded();

	static TArray<FMyBondageLifeRestraintItem> Items;
	static bool bLoaded;
};
