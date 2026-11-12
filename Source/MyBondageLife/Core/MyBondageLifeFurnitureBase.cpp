// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/MyBondageLifeFurnitureBase.h"

#include "Components/SceneComponent.h"

AMyBondageLifeFurnitureBase::AMyBondageLifeFurnitureBase()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	InteractionPoint = CreateDefaultSubobject<USceneComponent>(TEXT("InteractionPoint"));
	InteractionPoint->SetupAttachment(SceneRoot);
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
