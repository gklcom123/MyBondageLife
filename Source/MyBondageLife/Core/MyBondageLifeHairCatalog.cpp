// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeHairCatalog.h"

#include "Engine/DataTable.h"
#include "Engine/SkeletalMesh.h"
#include "MyBondageLife.h"
#include "UObject/UnrealType.h"

namespace
{
	/** Path of the hair table, kept in one place. */
	const TCHAR* HairTablePath = TEXT("/Game/DataTable/DT_Hair.DT_Hair");

	/** Reads the ID out of a row; several spellings are accepted. */
	int32 ReadRowId(const UScriptStruct* RowStruct, const uint8* RowData)
	{
		static const TCHAR* IdNames[] = { TEXT("ID"), TEXT("Id"), TEXT("id"), TEXT("HairID"), TEXT("HairId") };

		for (const TCHAR* Name : IdNames)
		{
			if (const FIntProperty* Property = FindFProperty<FIntProperty>(RowStruct, FName(Name)))
			{
				return Property->GetPropertyValue_InContainer(RowData);
			}
		}

		return 0;
	}

	/** Reads the mesh out of a row; several spellings are accepted. */
	USkeletalMesh* ReadRowMesh(const UScriptStruct* RowStruct, const uint8* RowData)
	{
		static const TCHAR* MeshNames[] = { TEXT("SkeletalMesh"), TEXT("HairMesh"), TEXT("Mesh") };

		for (const TCHAR* Name : MeshNames)
		{
			if (const FObjectProperty* Property = FindFProperty<FObjectProperty>(RowStruct, FName(Name)))
			{
				return Cast<USkeletalMesh>(Property->GetObjectPropertyValue_InContainer(RowData));
			}
		}

		return nullptr;
	}
}

TArray<FMyBondageLifeHairStyle> FMyBondageLifeHairCatalog::Styles;
bool FMyBondageLifeHairCatalog::bLoaded = false;

void FMyBondageLifeHairCatalog::Refresh()
{
	Styles.Reset();
	bLoaded = true;

	const UDataTable* Table = LoadObject<UDataTable>(nullptr, HairTablePath);
	if (!Table)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Hair: %s not found."), HairTablePath);
		return;
	}

	const UScriptStruct* RowStruct = Table->GetRowStruct();
	if (!RowStruct)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Hair: table has no row struct."));
		return;
	}

	for (const FName& RowName : Table->GetRowNames())
	{
		const uint8* RowData = Table->FindRowUnchecked(RowName);
		if (!RowData)
		{
			continue;
		}

		FMyBondageLifeHairStyle Style;
		Style.RowName = RowName;
		Style.Id = ReadRowId(RowStruct, RowData);
		Style.Mesh = ReadRowMesh(RowStruct, RowData);

		if (Style.Id <= 0)
		{
			UE_LOG(LogMyBondageLife, Warning, TEXT("Hair: row '%s' has no usable ID, skipped."), *RowName.ToString());
			continue;
		}

		Styles.Add(Style);
	}

	Styles.Sort([](const FMyBondageLifeHairStyle& A, const FMyBondageLifeHairStyle& B)
	{
		return A.Id < B.Id;
	});

	UE_LOG(LogMyBondageLife, Log, TEXT("Hair: %d style(s) loaded from %s."), Styles.Num(), HairTablePath);
}

void FMyBondageLifeHairCatalog::EnsureLoaded()
{
	if (!bLoaded)
	{
		Refresh();
	}
}

const TArray<FMyBondageLifeHairStyle>& FMyBondageLifeHairCatalog::GetStyles()
{
	EnsureLoaded();
	return Styles;
}

int32 FMyBondageLifeHairCatalog::GetStyleCount()
{
	EnsureLoaded();
	return Styles.Num();
}

bool FMyBondageLifeHairCatalog::HasStyle(int32 HairId)
{
	return FindStyle(HairId) != nullptr;
}

const FMyBondageLifeHairStyle* FMyBondageLifeHairCatalog::FindStyle(int32 HairId)
{
	EnsureLoaded();

	for (const FMyBondageLifeHairStyle& Style : Styles)
	{
		if (Style.Id == HairId)
		{
			return &Style;
		}
	}

	return nullptr;
}

int32 FMyBondageLifeHairCatalog::ClampId(int32 HairId)
{
	EnsureLoaded();

	if (Styles.Num() == 0)
	{
		return 1;
	}

	if (FindStyle(HairId))
	{
		return HairId;
	}

	// Nearest ID at or below the requested one, else the first style.
	int32 Best = Styles[0].Id;
	for (const FMyBondageLifeHairStyle& Style : Styles)
	{
		if (Style.Id <= HairId)
		{
			Best = Style.Id;
		}
	}

	return Best;
}

int32 FMyBondageLifeHairCatalog::StepId(int32 HairId, int32 Delta)
{
	EnsureLoaded();

	if (Styles.Num() == 0)
	{
		return 1;
	}

	int32 Index = 0;
	for (int32 i = 0; i < Styles.Num(); ++i)
	{
		if (Styles[i].Id == HairId)
		{
			Index = i;
			break;
		}
	}

	const int32 NewIndex = FMath::Clamp(Index + Delta, 0, Styles.Num() - 1);
	return Styles[NewIndex].Id;
}

USkeletalMesh* FMyBondageLifeHairCatalog::LoadMesh(int32 HairId)
{
	const FMyBondageLifeHairStyle* Style = FindStyle(HairId);
	if (!Style)
	{
		Style = Styles.Num() > 0 ? &Styles[0] : nullptr;
	}

	return Style ? Style->Mesh.LoadSynchronous() : nullptr;
}
