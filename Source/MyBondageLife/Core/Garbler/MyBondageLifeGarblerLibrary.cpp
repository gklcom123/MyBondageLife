// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/Garbler/MyBondageLifeGarblerLibrary.h"

#include "Core/Garbler/MyBondageLifeGarbler.h"
#include "Core/Garbler/MyBondageLifeGarblerSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UMyBondageLifeGarblerSubsystem* UMyBondageLifeGarblerLibrary::GetGarblerSubsystem(const UObject* WorldContextObject)
{
	if (!WorldContextObject || !GEngine)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UMyBondageLifeGarblerSubsystem>() : nullptr;
}

FString UMyBondageLifeGarblerLibrary::GarbleTextOnce(const FString& Text, const TArray<FName>& GagProfiles,
	const FMyBondageLifeGarblerOptions& Options)
{
	TArray<FName> Resolved;
	for (const FName& Entry : GagProfiles)
	{
		const FName Id = FMyBondageLifeGarbler::ResolveProfileId(Entry.ToString());
		if (!Id.IsNone() && !Resolved.Contains(Id))
		{
			Resolved.Add(Id);
		}
	}
	return FMyBondageLifeGarbler::Garble(Text, Resolved, Options);
}

TArray<FName> UMyBondageLifeGarblerLibrary::GetGarblerProfiles()
{
	return FMyBondageLifeGarbler::GetProfileIds();
}

FString UMyBondageLifeGarblerLibrary::GetGarblerProfileName(FName GagProfile)
{
	return FMyBondageLifeGarbler::GetProfileDisplayName(GagProfile);
}
