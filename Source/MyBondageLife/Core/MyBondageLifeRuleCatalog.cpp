// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/MyBondageLifeRuleCatalog.h"

#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "MyBondageLife.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* RuleTablePath = TEXT("/Game/DataTable/DT_Rule.DT_Rule");

	FString ReadText(const UScriptStruct* RowStruct, const uint8* RowData, std::initializer_list<const TCHAR*> Prefixes)
	{
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			const FString PropName = It->GetName();
			bool bMatch = false;
			for (const TCHAR* Prefix : Prefixes)
			{
				if (PropName.StartsWith(Prefix, ESearchCase::IgnoreCase))
				{
					bMatch = true;
					break;
				}
			}
			if (!bMatch)
			{
				continue;
			}

			if (const FStrProperty* StringProperty = CastField<FStrProperty>(*It))
			{
				return StringProperty->GetPropertyValue_InContainer(RowData);
			}
			if (const FNameProperty* NameProperty = CastField<FNameProperty>(*It))
			{
				return NameProperty->GetPropertyValue_InContainer(RowData).ToString();
			}
			if (const FTextProperty* TextProperty = CastField<FTextProperty>(*It))
			{
				return TextProperty->GetPropertyValue_InContainer(RowData).ToString();
			}
		}
		return FString();
	}

	void ReadDetailPanel(const UScriptStruct* RowStruct, const uint8* RowData, TSoftClassPtr<UUserWidget>& Out)
	{
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			if (!It->GetName().StartsWith(TEXT("DetailPanel"), ESearchCase::IgnoreCase))
			{
				continue;
			}

			if (const FSoftClassProperty* SoftClass = CastField<FSoftClassProperty>(*It))
			{
				Out = TSoftClassPtr<UUserWidget>(SoftClass->GetPropertyValue_InContainer(RowData).ToSoftObjectPath());
			}
			else if (const FClassProperty* ClassProperty = CastField<FClassProperty>(*It))
			{
				if (UClass* Value = Cast<UClass>(ClassProperty->GetObjectPropertyValue_InContainer(RowData)))
				{
					Out = TSoftClassPtr<UUserWidget>(FSoftObjectPath(Value));
				}
			}
			return;
		}
	}
}

TArray<FMyBondageLifeRuleItem> FMyBondageLifeRuleCatalog::Items;
static bool GRulesLoaded = false;

void FMyBondageLifeRuleCatalog::Refresh()
{
	Items.Reset();
	GRulesLoaded = true;

	const UDataTable* Table = LoadObject<UDataTable>(nullptr, RuleTablePath);
	if (!Table)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Rule: %s not found."), RuleTablePath);
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

		FMyBondageLifeRuleItem Item;
		Item.Name = ReadText(RowStruct, RowData, { TEXT("Name") });
		Item.Desc = ReadText(RowStruct, RowData, { TEXT("Desc") });
		Item.RuleId = ReadText(RowStruct, RowData, { TEXT("Rule_Effect_String") });
		if (Item.RuleId.IsEmpty())
		{
			Item.RuleId = RowName.ToString();
		}
		ReadDetailPanel(RowStruct, RowData, Item.DetailPanel);

		Items.Add(Item);
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("Rule: loaded %d row(s)."), Items.Num());
}

void FMyBondageLifeRuleCatalog::EnsureLoaded()
{
	if (!GRulesLoaded)
	{
		Refresh();
	}
}

const TArray<FMyBondageLifeRuleItem>& FMyBondageLifeRuleCatalog::GetItems()
{
	EnsureLoaded();
	return Items;
}

const FMyBondageLifeRuleItem* FMyBondageLifeRuleCatalog::Find(const FString& RuleId)
{
	EnsureLoaded();
	for (const FMyBondageLifeRuleItem& Item : Items)
	{
		if (Item.RuleId.Equals(RuleId, ESearchCase::IgnoreCase))
		{
			return &Item;
		}
	}
	return nullptr;
}

const FMyBondageLifeRuleItem* FMyBondageLifeRuleCatalog::FindByEffect(const FString& Effect)
{
	return Find(Effect);
}
