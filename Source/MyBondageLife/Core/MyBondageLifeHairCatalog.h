// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class USkeletalMesh;

/** One row of /Game/DataTable/DT_Hair. */
struct FMyBondageLifeHairStyle
{
	/** The row's ID field; this is the number stored on the account. */
	int32 Id = 1;

	/** Mesh worn for this style (may be null when the row is unfinished). */
	TSoftObjectPtr<USkeletalMesh> Mesh;

	/** Row name, only used for logging. */
	FName RowName;
};

/**
 *  Read-only view of the hair data table.
 *
 *  The table uses a Blueprint struct, so the rows are read through reflection
 *  (FIntProperty for the ID, FObjectProperty for the mesh) instead of linking
 *  against a generated struct header.
 */
class MYBONDAGELIFE_API FMyBondageLifeHairCatalog
{
public:

	/** Re-reads the table; call once at startup or after editing the asset. */
	static void Refresh();

	/** Styles sorted by ID, ascending. */
	static const TArray<FMyBondageLifeHairStyle>& GetStyles();

	static int32 GetStyleCount();

	/** True when the given ID exists in the table. */
	static bool HasStyle(int32 HairId);

	/** Finds a style by its ID. */
	static const FMyBondageLifeHairStyle* FindStyle(int32 HairId);

	/** Nearest existing ID (used to fall back when the stored value is stale). */
	static int32 ClampId(int32 HairId);

	/** ID Delta steps away in the sorted list; wraps are not allowed (clamps). */
	static int32 StepId(int32 HairId, int32 Delta);

	/** Loads the mesh synchronously; null when the style has no mesh. */
	static USkeletalMesh* LoadMesh(int32 HairId);

private:

	static void EnsureLoaded();

	static TArray<FMyBondageLifeHairStyle> Styles;
	static bool bLoaded;
};
