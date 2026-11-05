// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/MyBondageLifeEmoteCatalog.h"

#include "Animation/AnimMontage.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "MyBondageLife.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* EmoteTablePath = TEXT("/Game/DataTable/DT_Emote.DT_Emote");

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

			if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(*It))
			{
				if (const FNumericProperty* Underlying = EnumProperty->GetUnderlyingProperty())
				{
					return static_cast<int32>(Underlying->GetSignedIntPropertyValue(
						EnumProperty->ContainerPtrToValuePtr<void>(RowData)));
				}
			}
			if (const FByteProperty* ByteProperty = CastField<FByteProperty>(*It))
			{
				return static_cast<int32>(ByteProperty->GetPropertyValue_InContainer(RowData));
			}
			if (const FIntProperty* IntProperty = CastField<FIntProperty>(*It))
			{
				return IntProperty->GetPropertyValue_InContainer(RowData);
			}
		}
		return 0;
	}

	bool ReadBool(const UScriptStruct* RowStruct, const uint8* RowData, std::initializer_list<const TCHAR*> Prefixes)
	{
		for (TFieldIterator<FBoolProperty> It(RowStruct); It; ++It)
		{
			if (MatchesName(*It, Prefixes))
			{
				return It->GetPropertyValue_InContainer(RowData);
			}
		}
		return false;
	}

	FString ReadNameText(const UScriptStruct* RowStruct, const uint8* RowData)
	{
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			if (!MatchesName(*It, { TEXT("Name") }))
			{
				continue;
			}
			if (const FNameProperty* NameProperty = CastField<FNameProperty>(*It))
			{
				return NameProperty->GetPropertyValue_InContainer(RowData).ToString();
			}
			if (const FTextProperty* TextProperty = CastField<FTextProperty>(*It))
			{
				return TextProperty->GetPropertyValue_InContainer(RowData).ToString();
			}
			if (const FStrProperty* StringProperty = CastField<FStrProperty>(*It))
			{
				return StringProperty->GetPropertyValue_InContainer(RowData);
			}
		}
		return FString();
	}

	void ReadObject(const UScriptStruct* RowStruct, const uint8* RowData,
		std::initializer_list<const TCHAR*> Prefixes, UClass* ExpectedClass, TSoftObjectPtr<>& OutPtr)
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
					OutPtr = TSoftObjectPtr<>(Value.ToSoftObjectPath());
				}
				return;
			}
			if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(*It))
			{
				if (UObject* Value = ObjectProperty->GetObjectPropertyValue_InContainer(RowData))
				{
					if (Value->IsA(ExpectedClass))
					{
						OutPtr = TSoftObjectPtr<>(FSoftObjectPath(Value));
					}
				}
				return;
			}
		}
	}
}

TArray<FMyBondageLifeEmoteItem> FMyBondageLifeEmoteCatalog::Items;
static bool GEmotesLoaded = false;

void FMyBondageLifeEmoteCatalog::Refresh()
{
	Items.Reset();
	GEmotesLoaded = true;

	const UDataTable* Table = LoadObject<UDataTable>(nullptr, EmoteTablePath);
	if (!Table)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Emote: %s not found."), EmoteTablePath);
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

		FMyBondageLifeEmoteItem Item;
		Item.EmoteId = RowName;
		Item.Name = ReadNameText(RowStruct, RowData);
		Item.StateCondition = ReadInt(RowStruct, RowData, { TEXT("StateCondition") });
		Item.bArmBound = ReadBool(RowStruct, RowData, { TEXT("CanPlay_ArmBound") });
		Item.bLegBound = ReadBool(RowStruct, RowData, { TEXT("CanPlay_LegBound") });

		TSoftObjectPtr<> MontagePtr;
		ReadObject(RowStruct, RowData, { TEXT("Anmi") }, UAnimMontage::StaticClass(), MontagePtr);
		Item.Montage = TSoftObjectPtr<UAnimMontage>(MontagePtr.ToSoftObjectPath());

		TSoftObjectPtr<> IconPtr;
		ReadObject(RowStruct, RowData, { TEXT("Icon") }, UTexture2D::StaticClass(), IconPtr);
		Item.Icon = TSoftObjectPtr<UTexture2D>(IconPtr.ToSoftObjectPath());

		Items.Add(Item);
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("Emote: loaded %d row(s)."), Items.Num());
}

void FMyBondageLifeEmoteCatalog::EnsureLoaded()
{
	if (!GEmotesLoaded)
	{
		Refresh();
	}
}

const TArray<FMyBondageLifeEmoteItem>& FMyBondageLifeEmoteCatalog::GetItems()
{
	EnsureLoaded();
	return Items;
}

const FMyBondageLifeEmoteItem* FMyBondageLifeEmoteCatalog::Find(FName EmoteId)
{
	EnsureLoaded();
	for (const FMyBondageLifeEmoteItem& Item : Items)
	{
		if (Item.EmoteId == EmoteId)
		{
			return &Item;
		}
	}
	return nullptr;
}

bool FMyBondageLifeEmoteCatalog::MatchesState(const FMyBondageLifeEmoteItem& Item, int32 StateBit)
{
	if (Item.StateCondition == 0)
	{
		return true;   // 未配置 = 任意状态
	}
	return (Item.StateCondition & StateBit) != 0;
}
