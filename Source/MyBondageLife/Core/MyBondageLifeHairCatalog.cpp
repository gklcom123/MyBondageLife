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

	/** Reads an integer ID out of a row. */
	int32 ReadRowId(const UScriptStruct* RowStruct, const uint8* RowData)
	{
		// Blueprint struct properties carry a rename suffix ("ID_5_826C..."), so the
		// name is matched by prefix and the type does the rest.
		for (TFieldIterator<FIntProperty> It(RowStruct); It; ++It)
		{
			if (It->GetName().StartsWith(TEXT("ID"), ESearchCase::IgnoreCase))
			{
				return It->GetPropertyValue_InContainer(RowData);
			}
		}

		return 0;
	}

	/** Reads the mesh out of a row; handles both hard and soft object references. */
	void ReadRowMesh(const UScriptStruct* RowStruct, const uint8* RowData, FMyBondageLifeHairStyle& Style)
	{
		auto IsMeshField = [](const FProperty* Property)
		{
			const FString Name = Property->GetName();
			return Name.Contains(TEXT("Mesh"), ESearchCase::IgnoreCase)
				|| Name.Contains(TEXT("Hair"), ESearchCase::IgnoreCase);
		};

		// Preferred: a soft object pointer whose name mentions the mesh.
		for (TFieldIterator<FSoftObjectProperty> It(RowStruct); It; ++It)
		{
			if (!IsMeshField(*It))
			{
				continue;
			}

			const FSoftObjectPtr Value = It->GetPropertyValue_InContainer(RowData);
			if (!Value.IsNull())
			{
				Style.Mesh = TSoftObjectPtr<USkeletalMesh>(Value.ToSoftObjectPath());
				return;
			}
		}

		for (TFieldIterator<FObjectProperty> It(RowStruct); It; ++It)
		{
			if (!IsMeshField(*It))
			{
				continue;
			}

			if (USkeletalMesh* Mesh = Cast<USkeletalMesh>(It->GetObjectPropertyValue_InContainer(RowData)))
			{
				Style.Mesh = Mesh;
				return;
			}
		}

		// Fallback: any object reference at all (single-mesh struct).
		for (TFieldIterator<FSoftObjectProperty> It(RowStruct); It; ++It)
		{
			const FSoftObjectPtr Value = It->GetPropertyValue_InContainer(RowData);
			if (USkeletalMesh* Mesh = Cast<USkeletalMesh>(Value.LoadSynchronous()))
			{
				Style.Mesh = Mesh;
				return;
			}
		}

		for (TFieldIterator<FObjectProperty> It(RowStruct); It; ++It)
		{
			if (USkeletalMesh* Mesh = Cast<USkeletalMesh>(It->GetObjectPropertyValue_InContainer(RowData)))
			{
				Style.Mesh = Mesh;
				return;
			}
		}
	}

	/** Comma separated property list of the row struct, for diagnostics. */
	FString DescribeRowStruct(const UScriptStruct* RowStruct)
	{
		FString Names;
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			Names += It->GetName();
			Names += TEXT(" ");
		}
		return Names;
	}

	/** "1" -> 1; anything else -> 0. */
	int32 IdFromRowName(const FName& RowName)
	{
		const FString Text = RowName.ToString();
		if (Text.IsEmpty() || !FChar::IsDigit(Text[0]))
		{
			return 0;
		}

		return FCString::Atoi(*Text);
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

		// The table names its rows after the style ID ("1", "2", ...), so the row name
		// is the number stored on the account; the ID property is only a fallback.
		const int32 NamedId = IdFromRowName(RowName);
		Style.Id = NamedId > 0 ? NamedId : ReadRowId(RowStruct, RowData);
		ReadRowMesh(RowStruct, RowData, Style);

		if (Style.Id <= 0)
		{
			UE_LOG(LogMyBondageLife, Warning, TEXT("Hair: row '%s' has no usable ID, skipped."), *RowName.ToString());
			continue;
		}

		if (Style.Mesh.IsNull())
		{
			UE_LOG(LogMyBondageLife, Warning, TEXT("Hair: row %s (ID %d) has no mesh; struct properties: %s"),
				*RowName.ToString(), Style.Id, *DescribeRowStruct(RowStruct));
		}

		Styles.Add(Style);
	}

	Styles.Sort([](const FMyBondageLifeHairStyle& A, const FMyBondageLifeHairStyle& B)
	{
		return A.Id < B.Id;
	});

	UE_LOG(LogMyBondageLife, Log, TEXT("Hair: %d style(s) loaded from %s."), Styles.Num(), HairTablePath);

	if (Styles.Num() == 0)
	{
		// One-time diagnostic so a changed struct can be spotted from the log.
		FString PropertyNames;
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			PropertyNames += It->GetName();
			PropertyNames += TEXT(" ");
		}

		UE_LOG(LogMyBondageLife, Warning, TEXT("Hair: row struct '%s' properties: %s"),
			*RowStruct->GetName(), *PropertyNames);
	}
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
	if (!Style && Styles.Num() > 0)
	{
		Style = &Styles[0];
	}

	return Style ? Style->Mesh.LoadSynchronous() : nullptr;
}
