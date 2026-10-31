// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Garbler/MyBondageLifeGarblerTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyBondageLifeGarblerLibrary.generated.h"

class UMyBondageLifeGarblerSubsystem;

/**
 *  给蓝图 / 控件提供的静态工具函数。
 *
 *  UMG 里拿不到 GameInstance 时可以先 GetGarblerSubsystem，再调用它的 GarbleText；
 *  只是想试算一次（比如做一个"试听"按钮）则直接用 GarbleTextOnce，不影响当前状态。
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeGarblerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** 取当前场景的混淆器子系统（拿不到时返回 nullptr）。 */
	UFUNCTION(BlueprintPure, Category = "Garbler",
		meta = (WorldContext = "WorldContextObject", ToolTip = "取得混淆器子系统；失败返回空。"))
	static UMyBondageLifeGarblerSubsystem* GetGarblerSubsystem(const UObject* WorldContextObject);

	/** 一次性混淆：不影响当前佩戴状态。 */
	UFUNCTION(BlueprintPure, Category = "Garbler",
		meta = (ToolTip = "按指定堵嘴试算一次，不改变当前状态。"))
	static FString GarbleTextOnce(const FString& Text, const TArray<FName>& GagProfiles,
		const FMyBondageLifeGarblerOptions& Options);

	/** 音素表里所有可用的堵嘴档位 id。 */
	UFUNCTION(BlueprintPure, Category = "Garbler")
	static TArray<FName> GetGarblerProfiles();

	/** 档位显示名（"BallGag" -> "口球"）。 */
	UFUNCTION(BlueprintPure, Category = "Garbler")
	static FString GetGarblerProfileName(FName GagProfile);
};
