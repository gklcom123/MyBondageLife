// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;

/** One row of DT_Skin (ST_Skin). */
struct FMyBondageLifeSkinItem
{
	int32 Id = 1;
	FString Name;
	TSoftObjectPtr<UMaterialInterface> BodyMaterial;
	TSoftObjectPtr<UMaterialInterface> YatoeMaterial;
};

/**
 *  Skin catalogue: reads /Game/DataTable/DT_Skin once and answers queries.
 *  Row id 1 is the default skin.
 */
class FMyBondageLifeSkinCatalog
{
public:
	static void Refresh();
	static void EnsureLoaded();

	static const TArray<FMyBondageLifeSkinItem>& GetItems();
	static const FMyBondageLifeSkinItem* Find(int32 SkinId);

	/** Display name of a skin ("1 号" fallback). */
	static FString GetSkinName(int32 SkinId);

	/** Ids sorted ascending (for the left/right arrows). */
	static TArray<int32> GetSkinIds();

	/** Next / previous id with wrap-around. */
	static int32 GetNextSkinId(int32 CurrentSkinId, int32 Step);

private:
	static TArray<FMyBondageLifeSkinItem> Items;
};
