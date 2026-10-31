// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 *  Enum_GagType（蓝图枚举）→ Garbler 档位 id 的映射。
 *
 *  枚举条目是 NewEnumerator0..5，显示名是
 *  开口环 / 口球 / 面板口枷 / 布条封口 / 填塞 / 密封头套，
 *  正好对应 Garbler 的 6 个档位。
 *
 *  优先用枚举的"显示名"去匹配档位（以后调整枚举顺序也不会错位），
 *  显示名拿不到（例如改了本地化文本）时退回固定顺序 0..5。
 */
class MYBONDAGELIFE_API FMyBondageLifeGagTypeMap
{
public:

	/** 枚举值 -> 档位 id；无法识别时返回 NAME_None。 */
	static FName ToProfileId(int32 EnumValue);

	/** 旧数据完全没有配 GagType 时用的兜底档位。 */
	static FName GetDefaultProfileId();

	/** 枚举资产路径（排查"到底读的哪个枚举"时用）。 */
	static FString GetEnumPath();
};
