// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/MyBondageLifeRuleCatalog.h"

#include "Engine/DataTable.h"
#include "MyBondageLife.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* RuleTablePath = TEXT("/Game/DataTable/DT_Rule.DT_Rule");

	bool MatchesPrefix(const FProperty* Property, const TCHAR* Prefix)
	{
		return Property->GetName().StartsWith(Prefix, ESearchCase::IgnoreCase);
	}

	FString ReadText(const UScriptStruct* RowStruct, const uint8* RowData, std::initializer_list<const TCHAR*> Prefixes)
	{
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			bool bMatch = false;
			for (const TCHAR* Prefix : Prefixes)
			{
				if (MatchesPrefix(*It, Prefix))
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

	/** Number of elements in an array property whose name starts with the prefix. */
	int32 ReadArrayCount(const UScriptStruct* RowStruct, const uint8* RowData, const TCHAR* Prefix)
	{
		for (TFieldIterator<FArrayProperty> It(RowStruct); It; ++It)
		{
			if (!MatchesPrefix(*It, Prefix))
			{
				continue;
			}

			FScriptArrayHelper Helper(*It, static_cast<uint8*>(It->ContainerPtrToValuePtr<void>(const_cast<uint8*>(RowData))));
			return Helper.Num();
		}
		return 0;
	}

	/** Text/fstring elements of an array property, in order. */
	TArray<FString> ReadTextArray(const UScriptStruct* RowStruct, const uint8* RowData, const TCHAR* Prefix)
	{
		TArray<FString> Out;

		for (TFieldIterator<FArrayProperty> It(RowStruct); It; ++It)
		{
			if (!MatchesPrefix(*It, Prefix))
			{
				continue;
			}

			FScriptArrayHelper Helper(*It, static_cast<uint8*>(It->ContainerPtrToValuePtr<void>(const_cast<uint8*>(RowData))));
			const FProperty* Inner = It->Inner;

			for (int32 Index = 0; Index < Helper.Num(); ++Index)
			{
				const uint8* ElementPtr = Helper.GetRawPtr(Index);
				if (const FTextProperty* TextProperty = CastField<FTextProperty>(Inner))
				{
					Out.Add(TextProperty->GetPropertyValue(ElementPtr).ToString());
				}
				else if (const FStrProperty* StringProperty = CastField<FStrProperty>(Inner))
				{
					Out.Add(StringProperty->GetPropertyValue(ElementPtr));
				}
				else if (const FNameProperty* NameProperty = CastField<FNameProperty>(Inner))
				{
					Out.Add(NameProperty->GetPropertyValue(ElementPtr).ToString());
				}
				else
				{
					Out.Add(FString());
				}
			}
			break;
		}

		return Out;
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

		Item.DetailIntCount = ReadArrayCount(RowStruct, RowData, TEXT("Detail_Ints"));
		Item.DetailStringCount = ReadArrayCount(RowStruct, RowData, TEXT("Detail_Strings"));
		Item.DetailBoolCount = ReadArrayCount(RowStruct, RowData, TEXT("Detail_Bools"));
		Item.DetailDescs = ReadTextArray(RowStruct, RowData, TEXT("Detail_Descs"));

		Items.Add(Item);
	}

	// 一次性打印结构体字段名，便于确认 Detail_* 的真实名字与类型。
	for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
	{
		UE_LOG(LogMyBondageLife, Log, TEXT("Rule: DT field %s : %s"), *It->GetName(), *It->GetClass()->GetName());
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
