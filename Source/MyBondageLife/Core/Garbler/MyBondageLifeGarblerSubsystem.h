// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Garbler/MyBondageLifeGarblerTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MyBondageLifeGarblerSubsystem.generated.h"

/** 当前佩戴的堵嘴发生变化时广播。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMyBondageLifeActiveGagsChanged);

/**
 *  堵嘴文本混淆器（蓝图接口）。
 *
 *  典型用法：
 *      1. 玩家戴上/摘下堵嘴时调用 SetGagActive / SetActiveGags；
 *      2. 发送聊天、喊叫、气泡文字之前，把原文丢给 GarbleText 得到要显示的字符串。
 *
 *  没有佩戴任何堵嘴（或表里查不到）时 GarbleText 原样返回输入，调用方不需要自己判断。
 */
UCLASS()
class MYBONDAGELIFE_API UMyBondageLifeGarblerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ---------------------------------------------------------------------
	//  文本
	// ---------------------------------------------------------------------

	/**
	 *  把一句话混淆成当前堵嘴下的发音。
	 *  没有佩戴堵嘴时原样返回；中英混排、标点、*RP 段* 都会按规则处理。
	 */
	UFUNCTION(BlueprintCallable, Category = "Garbler|Text",
		meta = (ToolTip = "把文本按当前佩戴的堵嘴混淆成听不清的发音；没戴堵嘴时原样返回。"))
	FString GarbleText(const FString& Text) const;

	/** 指定堵嘴 + 指定参数的一次性混淆，不改变当前状态（适合做预览、对比）。 */
	UFUNCTION(BlueprintCallable, Category = "Garbler|Text",
		meta = (ToolTip = "指定堵嘴进行一次性混淆，不改变当前佩戴状态。"))
	FString GarbleTextWith(const FString& Text, const TArray<FName>& GagProfiles, const FMyBondageLifeGarblerOptions& Options) const;

	/** 调试：显示中间结果（汉字 -> 拼音 -> 音素），用来检查音素表。 */
	UFUNCTION(BlueprintPure, Category = "Garbler|Debug",
		meta = (ToolTip = "显示中间结果，例如 你好 -> 你[ni=n-i] 好[hao=h-ao]。"))
	FString ToPhonemes(const FString& Text) const;

	// ---------------------------------------------------------------------
	//  堵嘴状态
	// ---------------------------------------------------------------------

	/** 直接覆盖当前佩戴列表（可传档位 id、中文名或别名，例如 "口球" / "BallGag"）。 */
	UFUNCTION(BlueprintCallable, Category = "Garbler|Gags")
	void SetActiveGags(const TArray<FName>& GagProfiles);

	/** 单独戴上 / 摘下某个堵嘴。 */
	UFUNCTION(BlueprintCallable, Category = "Garbler|Gags")
	void SetGagActive(FName GagProfile, bool bActive);

	/** 全部摘下。 */
	UFUNCTION(BlueprintCallable, Category = "Garbler|Gags")
	void ClearActiveGags();

	UFUNCTION(BlueprintPure, Category = "Garbler|Gags")
	TArray<FName> GetActiveGags() const { return ActiveGags; }

	/** 当前是否处于"说不出话"状态（至少戴了一个堵嘴）。 */
	UFUNCTION(BlueprintPure, Category = "Garbler|Gags")
	bool IsGagged() const { return ActiveGags.Num() > 0; }

	/** 音素表里可用的所有档位 id。 */
	UFUNCTION(BlueprintPure, Category = "Garbler|Gags")
	TArray<FName> GetAvailableGags() const;

	UFUNCTION(BlueprintPure, Category = "Garbler|Gags")
	FString GetGagDisplayName(FName GagProfile) const;

	/** 这个档位是不是"完全无声"（只剩 …… 残响）。 */
	UFUNCTION(BlueprintPure, Category = "Garbler|Gags")
	bool IsSilentGag(FName GagProfile) const;

	// ---------------------------------------------------------------------
	//  数据
	// ---------------------------------------------------------------------

	/** 改完 json 后调用即可生效，不需要重启。 */
	UFUNCTION(BlueprintCallable, Category = "Garbler|Data")
	bool ReloadTables();

	/** 当前读取的音素表目录（排查"到底读的哪份表"）。 */
	UFUNCTION(BlueprintPure, Category = "Garbler|Data")
	FString GetDataDirectory() const;

	UPROPERTY(BlueprintAssignable, Category = "Garbler")
	FMyBondageLifeActiveGagsChanged OnActiveGagsChanged;

	/** GarbleText 使用的默认参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garbler")
	FMyBondageLifeGarblerOptions DefaultOptions;

private:


	UPROPERTY(VisibleAnywhere, Category = "Garbler")
	TArray<FName> ActiveGags;
};

