// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeMakeupCatalog.h"

#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "MyBondageLife.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* MakeupTablePath = TEXT("/Game/DataTable/DT_Makeup.DT_Makeup");

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

	void ReadRowTexture(const UScriptStruct* RowStruct, const uint8* RowData, FMyBondageLifeMakeupItem& Item)
	{
		for (TFieldIterator<FSoftObjectProperty> It(RowStruct); It; ++It)
		{
			const FSoftObjectPtr Value = It->GetPropertyValue_InContainer(RowData);
			if (!Value.IsNull())
			{
				Item.FaceTx = TSoftObjectPtr<UTexture2D>(Value.ToSoftObjectPath());
				return;
			}
		}

		for (TFieldIterator<FObjectProperty> It(RowStruct); It; ++It)
		{
			if (UTexture2D* Texture = Cast<UTexture2D>(It->GetObjectPropertyValue_InContainer(RowData)))
			{
				Item.FaceTx = Texture;
				return;
			}
		}
	}

	void ReadRowName(const UScriptStruct* RowStruct, const uint8* RowData, FMyBondageLifeMakeupItem& Item)
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

TArray<FMyBondageLifeMakeupItem> FMyBondageLifeMakeupCatalog::Items;
bool FMyBondageLifeMakeupCatalog::bLoaded = false;

void FMyBondageLifeMakeupCatalog::Refresh()
{
	Items.Reset();
	bLoaded = true;

	const UDataTable* Table = LoadObject<UDataTable>(nullptr, MakeupTablePath);
	if (!Table)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Makeup: %s not found."), MakeupTablePath);
		return;
	}

	const UScriptStruct* RowStruct = Table->GetRowStruct();
	if (!RowStruct)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Makeup: table has no row struct."));
		return;
	}

	for (const FName& RowName : Table->GetRowNames())
	{
		const uint8* RowData = Table->FindRowUnchecked(RowName);
		if (!RowData)
		{
			continue;
		}

		FMyBondageLifeMakeupItem Item;
		Item.RowName = RowName;
		const int32 NamedId = IdFromRowName(RowName);
		Item.Id = NamedId > 0 ? NamedId : ReadRowId(RowStruct, RowData);

		if (Item.Id <= 0)
		{
			UE_LOG(LogMyBondageLife, Warning, TEXT("Makeup: row '%s' has no usable ID, skipped."), *RowName.ToString());
			continue;
		}

		ReadRowTexture(RowStruct, RowData, Item);
		ReadRowName(RowStruct, RowData, Item);

		if (Item.FaceTx.IsNull())
		{
			UE_LOG(LogMyBondageLife, Warning, TEXT("Makeup: row %s (ID %d) has no texture."), *RowName.ToString(), Item.Id);
		}

		Items.Add(Item);
	}

	Items.Sort([](const FMyBondageLifeMakeupItem& A, const FMyBondageLifeMakeupItem& B)
	{
		return A.Id < B.Id;
	});

	UE_LOG(LogMyBondageLife, Log, TEXT("Makeup: %d item(s) loaded from %s."), Items.Num(), MakeupTablePath);
}

void FMyBondageLifeMakeupCatalog::EnsureLoaded()
{
	if (!bLoaded)
	{
		Refresh();
	}
}

const TArray<FMyBondageLifeMakeupItem>& FMyBondageLifeMakeupCatalog::GetItems()
{
	EnsureLoaded();
	return Items;
}

int32 FMyBondageLifeMakeupCatalog::GetItemCount()
{
	EnsureLoaded();
	return Items.Num();
}

bool FMyBondageLifeMakeupCatalog::HasItem(int32 MakeupId)
{
	return Find(MakeupId) != nullptr;
}

const FMyBondageLifeMakeupItem* FMyBondageLifeMakeupCatalog::Find(int32 MakeupId)
{
	EnsureLoaded();
	return Items.FindByPredicate([MakeupId](const FMyBondageLifeMakeupItem& Item)
	{
		return Item.Id == MakeupId;
	});
}

int32 FMyBondageLifeMakeupCatalog::ClampId(int32 MakeupId)
{
	EnsureLoaded();
	if (Items.Num() == 0)
	{
		return 1;
	}
	if (MakeupId < Items[0].Id)
	{
		return Items[0].Id;
	}
	for (const FMyBondageLifeMakeupItem& Item : Items)
	{
		if (Item.Id == MakeupId)
		{
			return Item.Id;
		}
	}
	for (int32 Index = Items.Num() - 1; Index >= 0; --Index)
	{
		if (Items[Index].Id <= MakeupId)
		{
			return Items[Index].Id;
		}
	}
	return Items[0].Id;
}

int32 FMyBondageLifeMakeupCatalog::StepId(int32 MakeupId, int32 Delta)
{
	EnsureLoaded();
	if (Items.Num() == 0)
	{
		return 1;
	}

	const int32 Current = ClampId(MakeupId);
	int32 CurrentIndex = 0;
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		if (Items[Index].Id == Current)
		{
			CurrentIndex = Index;
			break;
		}
	}

	const int32 TargetIndex = FMath::Clamp(CurrentIndex + Delta, 0, Items.Num() - 1);
	return Items[TargetIndex].Id;
}

UTexture2D* FMyBondageLifeMakeupCatalog::LoadFaceTexture(int32 MakeupId)
{
	if (const FMyBondageLifeMakeupItem* Item = Find(MakeupId))
	{
		return Item->FaceTx.LoadSynchronous();
	}
	return nullptr;
}
