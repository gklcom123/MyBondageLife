// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UTexture2D;

/** One row of /Game/DataTable/DT_SkinColor. */
struct FMyBondageLifeSkinColorItem
{
	int32 Id = 1;
	TSoftObjectPtr<UTexture2D> BodyTx;
	TSoftObjectPtr<UTexture2D> HeadTx;
	FString Name;
	FName RowName;
};

class MYBONDAGELIFE_API FMyBondageLifeSkinColorCatalog
{
public:

	static void Refresh();

	static const TArray<FMyBondageLifeSkinColorItem>& GetItems();

	static int32 GetItemCount();

	static bool HasItem(int32 SkinColorId);

	static const FMyBondageLifeSkinColorItem* Find(int32 SkinColorId);

	static int32 ClampId(int32 SkinColorId);

	static int32 StepId(int32 SkinColorId, int32 Delta);

	static UTexture2D* LoadBodyTexture(int32 SkinColorId);

	static UTexture2D* LoadHeadTexture(int32 SkinColorId);

private:

	static void EnsureLoaded();

	static TArray<FMyBondageLifeSkinColorItem> Items;
	static bool bLoaded;
};
