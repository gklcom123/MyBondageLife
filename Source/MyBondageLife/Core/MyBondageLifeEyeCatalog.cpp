// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeEyeCatalog.h"

#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "MyBondageLife.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* EyeTablePath = TEXT("/Game/DataTable/DT_Eye.DT_Eye");

	int32 IdFromRowName(const FName& RowName)
	{
		const FString Text = RowName.ToString();
		if (Text.IsEmpty())
		{
			return 0;
		}
		for (const TCHAR Ch : Text)
		{
			if (Ch < TEXT('0') || Ch > TEXT('9'))
			{
				return 0;
			}
		}
		return FCString::Atoi(*Text);
	}

	int32 ReadRowId(const UScriptStruct* RowStruct, const uint8* RowData)
	{
		for (TFieldIterator<FIntProperty> It(RowStruct); It; ++It)
		{
			if (It->GetName().StartsWith(TEXT("ID"), ESearchCase::IgnoreCase))
			{
				return It->GetPropertyValue_InContainer(RowData);
			}
		}
		return 0;
	}

	/** Finds a texture property whose name contains the given token. */
	UTexture2D* ReadTexture(const UScriptStruct* RowStruct, const uint8* RowData, const TCHAR* Token)
	{
		for (TFieldIterator<FObjectProperty> It(RowStruct); It; ++It)
		{
			if (!It->GetName().Contains(Token, ESearchCase::IgnoreCase))
			{
				continue;
			}
			if (UTexture2D* Texture = Cast<UTexture2D>(It->GetObjectPropertyValue_InContainer(RowData)))
			{
				return Texture;
			}
		}

		for (TFieldIterator<FSoftObjectProperty> It(RowStruct); It; ++It)
		{
			if (!It->GetName().Contains(Token, ESearchCase::IgnoreCase))
			{
				continue;
			}
			const FSoftObjectPtr Value = It->GetPropertyValue_InContainer(RowData);
			if (!Value.IsNull())
			{
				return Cast<UTexture2D>(Value.LoadSynchronous());
			}
		}
		return nullptr;
	}

	void ReadRowName(const UScriptStruct* RowStruct, const uint8* RowData, FMyBondageLifeEyeItem& Item)
	{
		for (TFieldIterator<FTextProperty> It(RowStruct); It; ++It)
		{
			if (It->GetName().Contains(TEXT("Name"), ESearchCase::IgnoreCase))
			{
				Item.Name = It->GetPropertyValue_InContainer(RowData).ToString();
				if (!Item.Name.IsEmpty())
				{
					return;
				}
			}
		}

		for (TFieldIterator<FStrProperty> It(RowStruct); It; ++It)
		{
			if (It->GetName().Contains(TEXT("Name"), ESearchCase::IgnoreCase))
			{
				Item.Name = It->GetPropertyValue_InContainer(RowData);
				if (!Item.Name.IsEmpty())
				{
					return;
				}
			}
		}
	}
}

TArray<FMyBondageLifeEyeItem> FMyBondageLifeEyeCatalog::Items;
bool FMyBondageLifeEyeCatalog::bLoaded = false;

void FMyBondageLifeEyeCatalog::Refresh()
{
	Items.Reset();
	bLoaded = true;

	const UDataTable* Table = LoadObject<UDataTable>(nullptr, EyeTablePath);
	if (!Table)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Eye: %s not found."), EyeTablePath);
		return;
	}

	const UScriptStruct* RowStruct = Table->GetRowStruct();
	if (!RowStruct)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Eye: table has no row struct."));
		return;
	}

	for (const FName& RowName : Table->GetRowNames())
	{
		const uint8* RowData = Table->FindRowUnchecked(RowName);
		if (!RowData)
		{
			continue;
		}

		FMyBondageLifeEyeItem Item;
		Item.RowName = RowName;
		const int32 NamedId = IdFromRowName(RowName);
		Item.Id = NamedId > 0 ? NamedId : ReadRowId(RowStruct, RowData);
		if (Item.Id <= 0)
		{
			UE_LOG(LogMyBondageLife, Warning, TEXT("Eye: row '%s' has no usable ID, skipped."), *RowName.ToString());
			continue;
		}

		if (UTexture2D* Base = ReadTexture(RowStruct, RowData, TEXT("BaseColor"))) { Item.BaseColor = Base; }
		if (UTexture2D* Emissive = ReadTexture(RowStruct, RowData, TEXT("Emissive"))) { Item.Emissive = Emissive; }
		if (UTexture2D* UV = ReadTexture(RowStruct, RowData, TEXT("UV"))) { Item.UV = UV; }
		ReadRowName(RowStruct, RowData, Item);

		Items.Add(Item);
	}

	Items.Sort([](const FMyBondageLifeEyeItem& A, const FMyBondageLifeEyeItem& B)
	{
		return A.Id < B.Id;
	});

	UE_LOG(LogMyBondageLife, Log, TEXT("Eye: %d item(s) loaded from %s."), Items.Num(), EyeTablePath);
}

void FMyBondageLifeEyeCatalog::EnsureLoaded()
{
	if (!bLoaded)
	{
		Refresh();
	}
}

const TArray<FMyBondageLifeEyeItem>& FMyBondageLifeEyeCatalog::GetItems()
{
	EnsureLoaded();
	return Items;
}

int32 FMyBondageLifeEyeCatalog::GetItemCount()
{
	EnsureLoaded();
	return Items.Num();
}

bool FMyBondageLifeEyeCatalog::HasItem(int32 EyeId)
{
	return Find(EyeId) != nullptr;
}

const FMyBondageLifeEyeItem* FMyBondageLifeEyeCatalog::Find(int32 EyeId)
{
	EnsureLoaded();
	return Items.FindByPredicate([EyeId](const FMyBondageLifeEyeItem& Item)
	{
		return Item.Id == EyeId;
	});
}

int32 FMyBondageLifeEyeCatalog::ClampId(int32 EyeId)
{
	EnsureLoaded();
	if (Items.Num() == 0)
	{
		return 1;
	}
	if (EyeId < Items[0].Id)
	{
		return Items[0].Id;
	}
	for (const FMyBondageLifeEyeItem& Item : Items)
	{
		if (Item.Id == EyeId)
		{
			return Item.Id;
		}
	}
	for (int32 Index = Items.Num() - 1; Index >= 0; --Index)
	{
		if (Items[Index].Id <= EyeId)
		{
			return Items[Index].Id;
		}
	}
	return Items[0].Id;
}

int32 FMyBondageLifeEyeCatalog::StepId(int32 EyeId, int32 Delta)
{
	EnsureLoaded();
	if (Items.Num() == 0)
	{
		return 1;
	}
	const int32 Current = ClampId(EyeId);
	int32 CurrentIndex = 0;
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		if (Items[Index].Id == Current)
		{
			CurrentIndex = Index;
			break;
		}
	}
	return Items[FMath::Clamp(CurrentIndex + Delta, 0, Items.Num() - 1)].Id;
}

UTexture2D* FMyBondageLifeEyeCatalog::LoadBaseColor(int32 EyeId)
{
	if (const FMyBondageLifeEyeItem* Item = Find(EyeId)) { return Item->BaseColor.LoadSynchronous(); }
	return nullptr;
}

UTexture2D* FMyBondageLifeEyeCatalog::LoadEmissive(int32 EyeId)
{
	if (const FMyBondageLifeEyeItem* Item = Find(EyeId)) { return Item->Emissive.LoadSynchronous(); }
	return nullptr;
}

UTexture2D* FMyBondageLifeEyeCatalog::LoadUV(int32 EyeId)
{
	if (const FMyBondageLifeEyeItem* Item = Find(EyeId)) { return Item->UV.LoadSynchronous(); }
	return nullptr;
}
