// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/MyBondageLifeFurnitureBase.h"
#include "Camera/CameraComponent.h"

#include "Components/SceneComponent.h"

AMyBondageLifeFurnitureBase::AMyBondageLifeFurnitureBase()
{
	PrimaryActorTick.bCanEverTick = false;
	// 父类不创建任何组件：InteractionPoint 由子蓝图放置后赋给变量。
}

USceneComponent* AMyBondageLifeFurnitureBase::GetInteractionCameraComponent() const
{
	if (InteractionCameraOverride)
	{
		return InteractionCameraOverride;
	}

	// 优先：组件名包含 InteractionCamera；否则用第一个 CameraComponent 兜底。
	TArray<UActorComponent*> AllComponents;
	GetComponents(AllComponents);

	USceneComponent* FirstCamera = nullptr;
	for (UActorComponent* Component : AllComponents)
	{
		USceneComponent* Scene = Cast<USceneComponent>(Component);
		if (!Scene)
		{
			continue;
		}

		if (Scene->GetName().Contains(TEXT("InteractionCamera"), ESearchCase::IgnoreCase))
		{
			return Scene;
		}
		if (!FirstCamera && Scene->IsA<UCameraComponent>())
		{
			FirstCamera = Scene;
		}
	}

	return FirstCamera;
}

FTransform AMyBondageLifeFurnitureBase::GetInteractionTransform() const
{
	USceneComponent* Point = InteractionPointOverride;
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
