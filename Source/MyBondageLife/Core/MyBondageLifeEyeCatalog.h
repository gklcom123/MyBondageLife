// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UTexture2D;

/** One row of /Game/DataTable/DT_Eye. */
struct FMyBondageLifeEyeItem
{
	/** Row's ID field; this is the number stored on the account. */
	int32 Id = 1;

	/** Base colour texture. */
	TSoftObjectPtr<UTexture2D> BaseColor;

	/** Emissive texture. */
	TSoftObjectPtr<UTexture2D> Emissive;

	/** UV/mask texture. */
	TSoftObjectPtr<UTexture2D> UV;

	/** Display name (DT_Eye.Name). */
	FString Name;

	/** Row name, only used for logging. */
	FName RowName;
};

/** Read-only view of DT_Eye (rows read through reflection). */
class MYBONDAGELIFE_API FMyBondageLifeEyeCatalog
{
public:

	static void Refresh();

	static const TArray<FMyBondageLifeEyeItem>& GetItems();

	static int32 GetItemCount();

	static bool HasItem(int32 EyeId);

	static const FMyBondageLifeEyeItem* Find(int32 EyeId);

	static int32 ClampId(int32 EyeId);

	static int32 StepId(int32 EyeId, int32 Delta);

	static UTexture2D* LoadBaseColor(int32 EyeId);

	static UTexture2D* LoadEmissive(int32 EyeId);

	static UTexture2D* LoadUV(int32 EyeId);

private:

	static void EnsureLoaded();

	static TArray<FMyBondageLifeEyeItem> Items;
	static bool bLoaded;
};
