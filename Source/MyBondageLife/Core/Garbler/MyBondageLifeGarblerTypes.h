// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MyBondageLifeGarblerTypes.generated.h"

/**
 * 混淆器的可调参数。所有字段都有默认值，蓝图里可以直接用默认构造。
 */
USTRUCT(BlueprintType)
struct MYBONDAGELIFE_API FMyBondageLifeGarblerOptions
{
	GENERATED_BODY()

	/** 拼音/词典里查不到的内容（生僻字、外文姓名、错字）是否也一起混淆。关掉则原样输出。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garbler")
	bool bGarbleUnknownWords = true;

	/** 是否保护 *...* 之间的 RP 描述段（不混淆）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garbler")
	bool bPreserveRoleplaySegments = true;

	/** 是否保护 :Emoji: 形式的表情符号（不混淆）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garbler")
	bool bPreserveEmotes = true;


	/**
	 * 随机种子。0 = 每次都不一样（"mhm" 这类兜底发音是随机的）；
	 * 非 0 = 同一句话每次得到完全相同的结果，方便做预览/测试。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garbler")
	int32 RandomSeed = 0;
};
