// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/MyBondageLifeFurnitureBase.h"
#include "Camera/CameraComponent.h"

#include "Components/SceneComponent.h"

#include "UObject/Class.h"

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
TArray<FString> AMyBondageLifeFurnitureBase::GetRestraintSlotOptions() const
{
	TArray<FString> Options;
	if (const UEnum* Enum = LoadObject<UEnum>(nullptr, TEXT("/Game/DataTable/Enum_RestraintSlot.Enum_RestraintSlot")))
	{
		for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index)
		{
			Options.Add(Enum->GetDisplayNameTextByIndex(Index).ToString());
		}
	}
	return Options;
}

TArray<uint8> AMyBondageLifeFurnitureBase::GetIgnoredRestraintSlots() const
{
	TArray<uint8> Result;
	UEnum* Enum = LoadObject<UEnum>(nullptr, TEXT("/Game/DataTable/Enum_RestraintSlot.Enum_RestraintSlot"));
	if (!Enum)
	{
		return Result;
	}

	for (const FName& SlotName : IgnoreRestraintPart)
	{
		if (SlotName.IsNone())
		{
			continue;
		}

		for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index)
		{
			const FString DisplayName = Enum->GetDisplayNameTextByIndex(Index).ToString();
			const FString InternalName = Enum->GetNameStringByIndex(Index);
			if (DisplayName == SlotName.ToString() || InternalName == SlotName.ToString())
			{
				Result.Add(static_cast<uint8>(Index));
				break;
			}
		}
	}
	return Result;
}
