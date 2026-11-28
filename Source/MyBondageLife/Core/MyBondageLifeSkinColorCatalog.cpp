// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeSkinColorCatalog.h"

#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "MyBondageLife.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* SkinColorTablePath = TEXT("/Game/DataTable/DT_SkinColor.DT_SkinColor");

	int32 IdFromRowName(const FName& RowName)
	{
		const FString Text = RowName.ToString();
		if (Text.IsEmpty()) { return 0; }
		for (const TCHAR Ch : Text)
		{
			if (Ch < TEXT('0') || Ch > TEXT('9')) { return 0; }
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

	TSoftObjectPtr<UTexture2D> FindTexture(const UScriptStruct* RowStruct, const uint8* RowData, const TCHAR* Token)
	{
		for (TFieldIterator<FObjectProperty> It(RowStruct); It; ++It)
		{
			if (!It->GetName().Contains(Token, ESearchCase::IgnoreCase)) { continue; }
			if (UTexture2D* Texture = Cast<UTexture2D>(It->GetObjectPropertyValue_InContainer(RowData)))
			{
				return TSoftObjectPtr<UTexture2D>(Texture);
			}
		}
		for (TFieldIterator<FSoftObjectProperty> It(RowStruct); It; ++It)
		{
			if (!It->GetName().Contains(Token, ESearchCase::IgnoreCase)) { continue; }
			const FSoftObjectPtr Value = It->GetPropertyValue_InContainer(RowData);
			if (!Value.IsNull())
			{
				return TSoftObjectPtr<UTexture2D>(Value.ToSoftObjectPath());
			}
		}
		return nullptr;
	}

	void ReadRowName(const UScriptStruct* RowStruct, const uint8* RowData, FMyBondageLifeSkinColorItem& Item)
	{
		for (TFieldIterator<FTextProperty> It(RowStruct); It; ++It)
		{
			if (It->GetName().Contains(TEXT("Name"), ESearchCase::IgnoreCase))
			{
				Item.Name = It->GetPropertyValue_InContainer(RowData).ToString();
				if (!Item.Name.IsEmpty()) { return; }
			}
		}
		for (TFieldIterator<FStrProperty> It(RowStruct); It; ++It)
		{
			if (It->GetName().Contains(TEXT("Name"), ESearchCase::IgnoreCase))
			{
				Item.Name = It->GetPropertyValue_InContainer(RowData);
				if (!Item.Name.IsEmpty()) { return; }
			}
		}
	}
}

TArray<FMyBondageLifeSkinColorItem> FMyBondageLifeSkinColorCatalog::Items;
bool FMyBondageLifeSkinColorCatalog::bLoaded = false;

void FMyBondageLifeSkinColorCatalog::Refresh()
{
	Items.Reset();
	bLoaded = true;

	const UDataTable* Table = LoadObject<UDataTable>(nullptr, SkinColorTablePath);
	if (!Table)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("SkinColor: %s not found."), SkinColorTablePath);
		return;
	}

	const UScriptStruct* RowStruct = Table->GetRowStruct();
	if (!RowStruct) { return; }

	for (const FName& RowName : Table->GetRowNames())
	{
		const uint8* RowData = Table->FindRowUnchecked(RowName);
		if (!RowData) { continue; }

		FMyBondageLifeSkinColorItem Item;
		Item.RowName = RowName;
		const int32 NamedId = IdFromRowName(RowName);
		Item.Id = NamedId > 0 ? NamedId : ReadRowId(RowStruct, RowData);
		if (Item.Id <= 0) { continue; }

		Item.BodyTx = FindTexture(RowStruct, RowData, TEXT("BodyTx"));
		Item.HeadTx = FindTexture(RowStruct, RowData, TEXT("HeadTx"));
		ReadRowName(RowStruct, RowData, Item);
		Items.Add(Item);
	}

	Items.Sort([](const FMyBondageLifeSkinColorItem& A, const FMyBondageLifeSkinColorItem& B)
	{
		return A.Id < B.Id;
	});

	UE_LOG(LogMyBondageLife, Log, TEXT("SkinColor: %d item(s) loaded."), Items.Num());
}

void FMyBondageLifeSkinColorCatalog::EnsureLoaded()
{
	if (!bLoaded) { Refresh(); }
}

const TArray<FMyBondageLifeSkinColorItem>& FMyBondageLifeSkinColorCatalog::GetItems()
{
	EnsureLoaded();
	return Items;
}

int32 FMyBondageLifeSkinColorCatalog::GetItemCount()
{
	EnsureLoaded();
	return Items.Num();
}

bool FMyBondageLifeSkinColorCatalog::HasItem(int32 SkinColorId)
{
	return Find(SkinColorId) != nullptr;
}

const FMyBondageLifeSkinColorItem* FMyBondageLifeSkinColorCatalog::Find(int32 SkinColorId)
{
	EnsureLoaded();
	return Items.FindByPredicate([SkinColorId](const FMyBondageLifeSkinColorItem& Item)
	{
		return Item.Id == SkinColorId;
	});
}

int32 FMyBondageLifeSkinColorCatalog::ClampId(int32 SkinColorId)
{
	EnsureLoaded();
	if (Items.Num() == 0) { return 1; }
	if (SkinColorId < Items[0].Id) { return Items[0].Id; }
	for (const FMyBondageLifeSkinColorItem& Item : Items)
	{
		if (Item.Id == SkinColorId) { return Item.Id; }
	}
	for (int32 Index = Items.Num() - 1; Index >= 0; --Index)
	{
		if (Items[Index].Id <= SkinColorId) { return Items[Index].Id; }
	}
	return Items[0].Id;
}

int32 FMyBondageLifeSkinColorCatalog::StepId(int32 SkinColorId, int32 Delta)
{
	EnsureLoaded();
	if (Items.Num() == 0) { return 1; }
	const int32 Current = ClampId(SkinColorId);
	int32 CurrentIndex = 0;
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		if (Items[Index].Id == Current) { CurrentIndex = Index; break; }
	}
	return Items[FMath::Clamp(CurrentIndex + Delta, 0, Items.Num() - 1)].Id;
}

UTexture2D* FMyBondageLifeSkinColorCatalog::LoadBodyTexture(int32 SkinColorId)
{
	if (const FMyBondageLifeSkinColorItem* Item = Find(SkinColorId)) { return Item->BodyTx.LoadSynchronous(); }
	return nullptr;
}

UTexture2D* FMyBondageLifeSkinColorCatalog::LoadHeadTexture(int32 SkinColorId)
{
	if (const FMyBondageLifeSkinColorItem* Item = Find(SkinColorId)) { return Item->HeadTx.LoadSynchronous(); }
	return nullptr;
}
