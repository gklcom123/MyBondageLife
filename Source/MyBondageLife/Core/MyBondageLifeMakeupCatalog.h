// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UTexture2D;

/** One row of /Game/DataTable/DT_Makeup. */
struct FMyBondageLifeMakeupItem
{
	/** Row's ID field; this is the number stored on the account. */
	int32 Id = 1;

	/** Face texture applied to the head material's "Makeup" parameter. */
	TSoftObjectPtr<UTexture2D> FaceTx;

	/** 妆容覆盖遮罩（ST_Makeup.FaceTxMask）。 */
	TSoftObjectPtr<UTexture2D> FaceTxMask;

	/** Display name (DT_Makeup.Name). */
	FString Name;

	/** Row name, only used for logging. */
	FName RowName;
};

/**
 *  Read-only view of DT_Makeup.
 *
 *  The table uses a Blueprint struct, so rows are read through reflection
 *  (FIntProperty for the ID, FObjectProperty/FSoftObjectProperty for the texture,
 *  FTextProperty/FStrProperty for the name).
 */
class MYBONDAGELIFE_API FMyBondageLifeMakeupCatalog
{
public:

	static void Refresh();

	static const TArray<FMyBondageLifeMakeupItem>& GetItems();

	static int32 GetItemCount();

	static bool HasItem(int32 MakeupId);

	static const FMyBondageLifeMakeupItem* Find(int32 MakeupId);

	/** Nearest existing ID (fallback when the stored value is stale). */
	static int32 ClampId(int32 MakeupId);

	/** ID Delta steps away in the sorted list; clamps at the ends. */
	static int32 StepId(int32 MakeupId, int32 Delta);

	/** Loads the face texture synchronously; null when the row has none. */
	static UTexture2D* LoadFaceTexture(int32 MakeupId);
	static UTexture2D* LoadFaceMask(int32 MakeupId);

private:

	static void EnsureLoaded();

	static TArray<FMyBondageLifeMakeupItem> Items;
	static bool bLoaded;
};
