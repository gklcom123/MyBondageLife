// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/MyBondageLifeFurnitureBase.h"

#include "Components/SceneComponent.h"

AMyBondageLifeFurnitureBase::AMyBondageLifeFurnitureBase()
{
	PrimaryActorTick.bCanEverTick = false;
	// 父类不创建任何组件：InteractionPoint 由子蓝图放置后赋给变量。
}

USceneComponent* AMyBondageLifeFurnitureBase::GetInteractionCameraComponent() const
{
	if (InteractionCamera)
	{
		return InteractionCamera;
	}

	// 蓝图里把组件命名为 InteractionCamera 也能直接用。
	TArray<USceneComponent*> Components;
	GetComponents<USceneComponent>(Components);
	for (USceneComponent* Component : Components)
	{
		if (Component && Component->GetName().Equals(TEXT("InteractionCamera"), ESearchCase::IgnoreCase))
		{
			return Component;
		}
	}
	return nullptr;
}

FTransform AMyBondageLifeFurnitureBase::GetInteractionTransform() const
{
	USceneComponent* Point = InteractionPoint;
	if (!Point)
	{
		TArray<USceneComponent*> Components;
		GetComponents<USceneComponent>(Components);
		for (USceneComponent* Component : Components)
		{
			if (Component && Component->GetName().Equals(TEXT("InteractionPoint"), ESearchCase::IgnoreCase))
			{
				Point = Component;
				break;
			}
	}
	}

	if (Point)
	{
		return Point->GetComponentTransform();
	}
	return GetActorTransform();
}

FString AMyBondageLifeFurnitureBase::GetFurnitureKey() const
{
	return FurnitureId.IsNone() ? GetName() : FurnitureId.ToString();
}

FVector AMyBondageLifeFurnitureBase::GetInteractionLocation() const
{
	return GetInteractionTransform().GetLocation();
}
