// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/MyBondageLifeSkinCatalog.h"

#include "Engine/DataTable.h"
#include "Materials/MaterialInterface.h"
#include "MyBondageLife.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* SkinTablePath = TEXT("/Game/DataTable/DT_Skin.DT_Skin");

	bool MatchesName(const FProperty* Property, std::initializer_list<const TCHAR*> Prefixes)
	{
		const FString Name = Property->GetName();
		for (const TCHAR* Prefix : Prefixes)
		{
			if (Name.StartsWith(Prefix, ESearchCase::IgnoreCase))
			{
				return true;
			}
		}
		return false;
	}

	int32 ReadInt(const UScriptStruct* RowStruct, const uint8* RowData, std::initializer_list<const TCHAR*> Prefixes)
	{
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			if (!MatchesName(*It, Prefixes))
			{
				continue;
			}

			if (const FIntProperty* IntProperty = CastField<FIntProperty>(*It))
			{
				return IntProperty->GetPropertyValue_InContainer(RowData);
			}
			if (const FByteProperty* ByteProperty = CastField<FByteProperty>(*It))
			{
				return static_cast<int32>(ByteProperty->GetPropertyValue_InContainer(RowData));
			}
		}
		return 0;
	}

	FString ReadDisplayName(const UScriptStruct* RowStruct, const uint8* RowData)
	{
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			if (!MatchesName(*It, { TEXT("Name") }))
			{
				continue;
			}

			if (const FTextProperty* TextProperty = CastField<FTextProperty>(*It))
			{
				return TextProperty->GetPropertyValue_InContainer(RowData).ToString();
			}
			if (const FStrProperty* StringProperty = CastField<FStrProperty>(*It))
			{
				return StringProperty->GetPropertyValue_InContainer(RowData);
			}
			if (const FNameProperty* NameProperty = CastField<FNameProperty>(*It))
			{
				return NameProperty->GetPropertyValue_InContainer(RowData).ToString();
			}
		}
		return FString();
	}

	void ReadMaterial(const UScriptStruct* RowStruct, const uint8* RowData,
		std::initializer_list<const TCHAR*> Prefixes, TSoftObjectPtr<UMaterialInterface>& OutMaterial)
	{
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			if (!MatchesName(*It, Prefixes))
			{
				continue;
			}

			if (const FSoftObjectProperty* SoftProperty = CastField<FSoftObjectProperty>(*It))
			{
				const FSoftObjectPtr Value = SoftProperty->GetPropertyValue_InContainer(RowData);
				if (!Value.IsNull())
				{
					OutMaterial = TSoftObjectPtr<UMaterialInterface>(Value.ToSoftObjectPath());
				}
				return;
			}

			if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(*It))
			{
				if (UObject* Value = ObjectProperty->GetObjectPropertyValue_InContainer(RowData))
				{
					OutMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(Value));
				}
				return;
			}
		}
	}
}

TArray<FMyBondageLifeSkinItem> FMyBondageLifeSkinCatalog::Items;
static bool GSkinsLoaded = false;

void FMyBondageLifeSkinCatalog::Refresh()
{
	Items.Reset();
	GSkinsLoaded = true;

	const UDataTable* Table = LoadObject<UDataTable>(nullptr, SkinTablePath);
	if (!Table)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Skin: %s not found."), SkinTablePath);
		return;
	}

	const UScriptStruct* RowStruct = Table->GetRowStruct();
	if (!RowStruct)
	{
		return;
	}

	for (const FName& RowName : Table->GetRowNames())
	{
		const uint8* RowData = Table->FindRowUnchecked(RowName);
		if (!RowData)
		{
			continue;
		}

		FMyBondageLifeSkinItem Item;
		Item.Id = ReadInt(RowStruct, RowData, { TEXT("ID") });
		if (Item.Id <= 0)
		{
			Item.Id = FCString::Atoi(*RowName.ToString());
		}
		Item.Name = ReadDisplayName(RowStruct, RowData);
		ReadMaterial(RowStruct, RowData, { TEXT("Skin_BodyMat"), TEXT("BodyMat") }, Item.BodyMaterial);
		ReadMaterial(RowStruct, RowData, { TEXT("Skin_YatoeMat"), TEXT("YatoeMat") }, Item.YatoeMaterial);

		Items.Add(Item);
	}

	Items.Sort([](const FMyBondageLifeSkinItem& A, const FMyBondageLifeSkinItem& B)
	{
		return A.Id < B.Id;
	});

	UE_LOG(LogMyBondageLife, Log, TEXT("Skin: loaded %d rows."), Items.Num());
}

void FMyBondageLifeSkinCatalog::EnsureLoaded()
{
	if (!GSkinsLoaded)
	{
		Refresh();
	}
}

const TArray<FMyBondageLifeSkinItem>& FMyBondageLifeSkinCatalog::GetItems()
{
	EnsureLoaded();
	return Items;
}

const FMyBondageLifeSkinItem* FMyBondageLifeSkinCatalog::Find(int32 SkinId)
{
	EnsureLoaded();
	for (const FMyBondageLifeSkinItem& Item : Items)
	{
		if (Item.Id == SkinId)
		{
			return &Item;
		}
	}
	return nullptr;
}

FString FMyBondageLifeSkinCatalog::GetSkinName(int32 SkinId)
{
	if (const FMyBondageLifeSkinItem* Item = Find(SkinId))
	{
		return Item->Name.IsEmpty() ? FString::Printf(TEXT("%d 号"), SkinId) : Item->Name;
	}
	return FString::Printf(TEXT("%d 号"), SkinId);
}

TArray<int32> FMyBondageLifeSkinCatalog::GetSkinIds()
{
	EnsureLoaded();

	TArray<int32> Ids;
	for (const FMyBondageLifeSkinItem& Item : Items)
	{
		Ids.Add(Item.Id);
	}
	return Ids;
}

int32 FMyBondageLifeSkinCatalog::GetNextSkinId(int32 CurrentSkinId, int32 Step)
{
	const TArray<int32> Ids = GetSkinIds();
	if (Ids.Num() == 0)
	{
		return 1;
	}

	int32 Index = Ids.IndexOfByKey(CurrentSkinId);
	if (Index == INDEX_NONE)
	{
		return Ids[0];
	}

	Index = (Index + Step % Ids.Num() + Ids.Num()) % Ids.Num();
	return Ids[Index];
}
