// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/MyBondageLifeFurnitureBase.h"

#include "Components/SceneComponent.h"

AMyBondageLifeFurnitureBase::AMyBondageLifeFurnitureBase()
{
	PrimaryActorTick.bCanEverTick = false;
	// 父类不创建任何组件：InteractionPoint 由子蓝图放置后赋给变量。
}

FTransform AMyBondageLifeFurnitureBase::GetInteractionTransform() const
{
	if (InteractionPoint)
	{
		return InteractionPoint->GetComponentTransform();
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
