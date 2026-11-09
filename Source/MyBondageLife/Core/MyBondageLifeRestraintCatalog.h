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

	/** Enum_RestraintSlot value as an integer (0..16). */
	int32 Slot = 0;

	TSoftObjectPtr<USkeletalMesh> Mesh;
	TSoftObjectPtr<UTexture2D> Icon;
	TSoftObjectPtr<UAnimMontage> UpAnmi;

	/** Head only montage (gag). */
	TSoftObjectPtr<UAnimMontage> GagAnmi;
	TSoftObjectPtr<UAnimMontage> DownAnmi;

	bool bRestraintArm = false;
	bool bInGag = false;

	/** 眼罩：BlindLevel 1..3 表示视野受限强度。 */
	bool bIsBlind = false;
	int32 BlindLevel = 0;

	/** 该拘束要隐藏的身体材质插槽名（upbody/leg/yab_fac/hand/...）。 */
	TArray<FString> HideBodyParts;
	bool bRestraintLeg = false;
	bool bCantMove = false;
	bool bInPetMode = false;

	/** Enum_GagType 的整数值（只对口部道具有效）。 */
	int32 GagType = 0;

	/** 该行是否真的带 GagType 列；旧数据可能没有这一列。 */
	bool bHasGagType = false;

	/** 性欲值增长速度，纯客户端字段（服务端不存）。 */
	float Libido = 0.0f;

	/** 穿戴所需时间（秒）；<=0 表示瞬间穿戴。 */
	float WearTime = 0.0f;

	/** true = 不出现在拘束选择列表，但仍可被穿戴/脱下。 */
	bool bCantVisiable = false;
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

	static constexpr int32 SlotCount = 17;

	static void Refresh();
	static const TArray<FMyBondageLifeRestraintItem>& GetItems();

	/** Item by row name; null when unknown. */
	static const FMyBondageLifeRestraintItem* Find(FName ItemId);

	/** Items that belong to one slot, sorted by id. */
	static TArray<const FMyBondageLifeRestraintItem*> GetItemsForSlot(int32 Slot);

	/** Display name of a slot ("嘴部", "项圈", ...). */
	static FString GetSlotName(int32 Slot);

	/** Bitmask of Color1/2/3 found on the item's materials. */
	static int32 GetColorChannelMask(FName ItemId);

	/** Worn mesh of an item (null when the row has none). */
	static class USkeletalMesh* LoadWornMesh(FName ItemId);

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
