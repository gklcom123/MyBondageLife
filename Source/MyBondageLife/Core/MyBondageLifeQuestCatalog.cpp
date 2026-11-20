// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/MyBondageLifeQuestCatalog.h"

#include "MyBondageLife.h"
#include "Engine/DataTable.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* QuestTablePath = TEXT("/Game/DataTable/DT_Quest.DT_Quest");

	TArray<FMyBondageLifeQuestDef> GItems;

	bool Matches(const FProperty* Property, const TCHAR* Prefix)
	{
		return Property && Property->GetName().StartsWith(Prefix, ESearchCase::IgnoreCase);
	}

	FString ReadText(const UScriptStruct* RowStruct, const uint8* RowData, const TCHAR* Prefix)
	{
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			if (!Matches(*It, Prefix)) { continue; }
			if (const FTextProperty* P = CastField<FTextProperty>(*It)) { return P->GetPropertyValue_InContainer(RowData).ToString(); }
			if (const FStrProperty* P = CastField<FStrProperty>(*It)) { return P->GetPropertyValue_InContainer(RowData); }
			if (const FNameProperty* P = CastField<FNameProperty>(*It)) { return P->GetPropertyValue_InContainer(RowData).ToString(); }
		}
		return FString();
	}

	int32 ReadInt(const UScriptStruct* RowStruct, const uint8* RowData, const TCHAR* Prefix)
	{
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			if (!Matches(*It, Prefix)) { continue; }
			if (const FIntProperty* P = CastField<FIntProperty>(*It)) { return P->GetPropertyValue_InContainer(RowData); }
			if (const FInt64Property* P = CastField<FInt64Property>(*It)) { return static_cast<int32>(P->GetPropertyValue_InContainer(RowData)); }
			if (const FFloatProperty* P = CastField<FFloatProperty>(*It)) { return FMath::RoundToInt(P->GetPropertyValue_InContainer(RowData)); }
			if (const FDoubleProperty* P = CastField<FDoubleProperty>(*It)) { return FMath::RoundToInt(P->GetPropertyValue_InContainer(RowData)); }
		}
		return 0;
	}

	bool ReadBool(const UScriptStruct* RowStruct, const uint8* RowData, const TCHAR* Prefix)
	{
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			if (!Matches(*It, Prefix)) { continue; }
			if (const FBoolProperty* P = CastField<FBoolProperty>(*It)) { return P->GetPropertyValue_InContainer(RowData); }
		}
		return false;
	}
}

void FMyBondageLifeQuestCatalog::Refresh()
{
	GItems.Reset();

	UDataTable* Table = LoadObject<UDataTable>(nullptr, QuestTablePath);
	if (!Table)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Quest: DT_Quest not found"));
		return;
	}

	const UScriptStruct* RowStruct = Table->GetRowStruct();
	if (!RowStruct) { return; }

	for (const FName& RowName : Table->GetRowNames())
	{
		const uint8* RowData = Table->FindRowUnchecked(RowName);
		if (!RowData) { continue; }

		FMyBondageLifeQuestDef Item;
		Item.Id = RowName;
		Item.Name = ReadText(RowStruct, RowData, TEXT("Name"));
		Item.Description = ReadText(RowStruct, RowData, TEXT("Desc"));
		Item.QuestString = ReadText(RowStruct, RowData, TEXT("QuestString"));
		Item.Times = FMath::Max(1, ReadInt(RowStruct, RowData, TEXT("Times")));
		Item.bDaily = ReadBool(RowStruct, RowData, TEXT("DailyQuest"));
		Item.RewardMoney = ReadInt(RowStruct, RowData, TEXT("RewardMoney"));

		UE_LOG(LogMyBondageLife, Log, TEXT("Quest: id=%s type=%s times=%d daily=%d reward=%d"),
			*Item.Id.ToString(), *Item.QuestString, Item.Times, Item.bDaily ? 1 : 0, Item.RewardMoney);

		GItems.Add(MoveTemp(Item));
	}
}

const TArray<FMyBondageLifeQuestDef>& FMyBondageLifeQuestCatalog::GetItems()
{
	if (GItems.Num() == 0)
	{
		Refresh();
	}
	return GItems;
}

const FMyBondageLifeQuestDef* FMyBondageLifeQuestCatalog::Find(FName Id)
{
	for (const FMyBondageLifeQuestDef& Item : GetItems())
	{
		if (Item.Id == Id)
		{
			return &Item;
		}
	}
	return nullptr;
}
