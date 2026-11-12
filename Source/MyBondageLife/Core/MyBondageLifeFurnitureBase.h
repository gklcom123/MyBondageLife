// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyBondageLifeFurnitureBase.generated.h"

class USceneComponent;
class UAnimMontage;

/**
 * 固定式家具父类（牢笼等）。
 *
 * 继承者在蓝图里配置：
 *  - InteractionPoint：人物被固定的位置与朝向（自身朝向）
 *  - Anmi：交互期间循环播放的动画蒙太奇
 *  - Name：界面显示名（W_InterFurniture_Name）
 *  - FurnitureId：服务端持久化用的稳定 ID（留空则回退 Actor 名称）
 */
UCLASS(Blueprintable)
class MYBONDAGELIFE_API AMyBondageLifeFurnitureBase : public AActor
{
	GENERATED_BODY()

public:
	AMyBondageLifeFurnitureBase();

	/** 人物被固定到的位置（用它的朝向）。 */
	UFUNCTION(BlueprintPure, Category = "Furniture")
	FTransform GetInteractionTransform() const;

	/** 服务端持久化用的 ID：优先 FurnitureId，否则 Actor 名称。 */
	UFUNCTION(BlueprintPure, Category = "Furniture")
	FString GetFurnitureKey() const;

	UFUNCTION(BlueprintPure, Category = "Furniture")
	FString GetFurnitureDisplayName() const { return Name.IsEmpty() ? GetName() : Name; }

	UFUNCTION(BlueprintPure, Category = "Furniture")
	UAnimMontage* GetInteractionMontage() const { return Anmi; }

	/** 交互判定点（InteractionPoint 的世界坐标）。 */
	UFUNCTION(BlueprintPure, Category = "Furniture")
	FVector GetInteractionLocation() const;

public:
	/**
	 * 人物固定点：在**子蓝图**里自己放一个 SceneComponent（名字随意），
	 * 然后把它的引用赋给这个变量即可。父类不创建任何组件，避免与子蓝图同名冲突。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furniture")
	TObjectPtr<USceneComponent> InteractionPoint;

	/** 交互时用于接管视角的相机位置（子蓝图里放一个 Camera/Scene 组件后赋值）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furniture")
	TObjectPtr<USceneComponent> InteractionCamera;

	/** 交互期间循环播放的蒙太奇。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furniture")
	TObjectPtr<UAnimMontage> Anmi;

	/** 界面显示名。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furniture")
	FString Name;

	/** 稳定 ID（1–64 位字母数字下划线）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furniture")
	FName FurnitureId;

	/** 交互距离（厘米）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furniture")
	float InteractRange = 220.0f;
};
