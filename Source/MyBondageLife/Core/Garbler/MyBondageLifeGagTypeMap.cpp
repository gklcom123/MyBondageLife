// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/Garbler/MyBondageLifeGagTypeMap.h"

#include "Core/Garbler/MyBondageLifeGarbler.h"
#include "MyBondageLife.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const TCHAR* GagTypeEnumPath = TEXT("/Game/DataTable/Enum_GagType.Enum_GagType");

	/**
	 *  枚举显示名匹配失败时的兜底顺序，与 Enum_GagType 的条目顺序一致：
	 *  0 开口环 / 1 口球 / 2 面板口枷 / 3 布条封口 / 4 填塞 / 5 密封头套。
	 */
	const FName GagProfilesByIndex[] =
	{
		FName(TEXT("RingGag")),
		FName(TEXT("BallGag")),
		FName(TEXT("PanelGag")),
		FName(TEXT("CleaveGag")),
		FName(TEXT("StuffGag")),
		FName(TEXT("HoodGag")),
	};
}

FName FMyBondageLifeGagTypeMap::ToProfileId(int32 EnumValue)
{
	if (EnumValue < 0)
	{
		return NAME_None;
	}

	// 1) 用枚举的显示名匹配档位（"口球" 等），这样即使以后重排枚举也不会串档。
	if (UEnum* GagEnum = LoadObject<UEnum>(nullptr, GagTypeEnumPath))
	{
		const FString DisplayName = GagEnum->GetDisplayNameTextByValue(EnumValue).ToString();
		if (!DisplayName.IsEmpty())
		{
			const FName ByDisplayName = FMyBondageLifeGarbler::ResolveProfileId(DisplayName);
			if (!ByDisplayName.IsNone())
			{
				return ByDisplayName;
			}
		}
	}

	// 2) 退回固定顺序。
	const int32 Count = UE_ARRAY_COUNT(GagProfilesByIndex);
	if (EnumValue < Count)
	{
		return GagProfilesByIndex[EnumValue];
	}

	UE_LOG(LogMyBondageLife, Warning, TEXT("Garbler: Enum_GagType 值 %d 没有对应的档位。"), EnumValue);
	return NAME_None;
}

FName FMyBondageLifeGagTypeMap::GetDefaultProfileId()
{
	// 没配 GagType 的旧口部道具，按最通用的口球处理。
	return FName(TEXT("BallGag"));
}

FString FMyBondageLifeGagTypeMap::GetEnumPath()
{
	return GagTypeEnumPath;
}
