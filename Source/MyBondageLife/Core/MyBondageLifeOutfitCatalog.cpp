// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/MyBondageLifeOutfitCatalog.h"

#include "Engine/DataTable.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "MyBondageLife.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* OutfitTablePath = TEXT("/Game/DataTable/DT_Outfit.DT_Outfit");

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
					const void* ValuePtr = EnumProperty->ContainerPtrToValuePtr<void>(RowData);
					return static_cast<int32>(Underlying->GetSignedIntPropertyValue(ValuePtr));
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

	float ReadFloat(const UScriptStruct* RowStruct, const uint8* RowData, std::initializer_list<const TCHAR*> Prefixes)
	{
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			if (!MatchesName(*It, Prefixes))
			{
				continue;
			}

			if (const FFloatProperty* FloatProperty = CastField<FFloatProperty>(*It))
			{
				return FloatProperty->GetPropertyValue_InContainer(RowData);
			}
			if (const FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(*It))
			{
				return static_cast<float>(DoubleProperty->GetPropertyValue_InContainer(RowData));
			}
		}
		return 0.0f;
	}

	FString ReadName(const UScriptStruct* RowStruct, const uint8* RowData)
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

	void ReadStringArray(const UScriptStruct* RowStruct, const uint8* RowData,
		std::initializer_list<const TCHAR*> Prefixes, TArray<FString>& OutValues)
	{
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			if (!MatchesName(*It, Prefixes))
			{
				continue;
			}

			const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(*It);
			if (!ArrayProperty || !CastField<FStrProperty>(ArrayProperty->Inner))
			{
				return;
			}

			FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(RowData));
			for (int32 Index = 0; Index < Helper.Num(); ++Index)
			{
				const FString Value = *reinterpret_cast<const FString*>(Helper.GetRawPtr(Index));
				if (!Value.IsEmpty())
				{
					OutValues.Add(Value);
				}
			}
			return;
		}
	}

	void ReadSoftObject(const UScriptStruct* RowStruct, const uint8* RowData,
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

TArray<FMyBondageLifeOutfitItem> FMyBondageLifeOutfitCatalog::Items;
bool FMyBondageLifeOutfitCatalog::bLoaded = false;

void FMyBondageLifeOutfitCatalog::Refresh()
{
	Items.Reset();
	bLoaded = true;

	const UDataTable* Table = LoadObject<UDataTable>(nullptr, OutfitTablePath);
	if (!Table)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Outfit: %s not found."), OutfitTablePath);
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

		FMyBondageLifeOutfitItem Item;
		Item.ItemId = RowName;
		Item.Name = ReadName(RowStruct, RowData);
		Item.Slot = FMath::Clamp(ReadInt(RowStruct, RowData, { TEXT("Slot") }), 0, SlotCount - 1);
		Item.FeetOffset = ReadFloat(RowStruct, RowData, { TEXT("FeetOffset") });
		ReadStringArray(RowStruct, RowData, { TEXT("HideBodyPart"), TEXT("HideBodyPart?") }, Item.HideBodyParts);

		TSoftObjectPtr<> MeshPtr;
		ReadSoftObject(RowStruct, RowData, { TEXT("Mesh") }, USkeletalMesh::StaticClass(), MeshPtr);
		Item.Mesh = TSoftObjectPtr<USkeletalMesh>(MeshPtr.ToSoftObjectPath());

		TSoftObjectPtr<> IconPtr;
		ReadSoftObject(RowStruct, RowData, { TEXT("Icon") }, UTexture2D::StaticClass(), IconPtr);
		Item.Icon = TSoftObjectPtr<UTexture2D>(IconPtr.ToSoftObjectPath());

		Items.Add(Item);
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("Outfit: loaded %d rows."), Items.Num());
}

void FMyBondageLifeOutfitCatalog::EnsureLoaded()
{
	if (!bLoaded)
	{
		Refresh();
	}
}

const TArray<FMyBondageLifeOutfitItem>& FMyBondageLifeOutfitCatalog::GetItems()
{
	EnsureLoaded();
	return Items;
}

const FMyBondageLifeOutfitItem* FMyBondageLifeOutfitCatalog::Find(FName ItemId)
{
	EnsureLoaded();
	for (const FMyBondageLifeOutfitItem& Item : Items)
	{
		if (Item.ItemId == ItemId)
		{
			return &Item;
		}
	}
	return nullptr;
}

TArray<const FMyBondageLifeOutfitItem*> FMyBondageLifeOutfitCatalog::GetItemsForSlot(int32 Slot)
{
	EnsureLoaded();

	TArray<const FMyBondageLifeOutfitItem*> Result;
	for (const FMyBondageLifeOutfitItem& Item : Items)
	{
		if (Item.Slot == Slot)
		{
			Result.Add(&Item);
		}
	}

	Result.Sort([](const FMyBondageLifeOutfitItem& A, const FMyBondageLifeOutfitItem& B)
	{
		return A.ItemId.LexicalLess(B.ItemId);
	});

	return Result;
}

float FMyBondageLifeOutfitCatalog::SumFeetOffsets(const TArray<FName>& ItemIds)
{
	EnsureLoaded();

	float Total = 0.0f;
	for (const FName& ItemId : ItemIds)
	{
		if (const FMyBondageLifeOutfitItem* Item = Find(ItemId))
		{
			Total += Item->FeetOffset;
		}
	}
	return Total;
}

FString FMyBondageLifeOutfitCatalog::GetSlotName(int32 Slot)
{
	static const TCHAR* Names[SlotCount] = {
		TEXT("\u5E3D\u5B50"),
		TEXT("\u9879\u94FE"),
		TEXT("\u4E0A\u8863"),
		TEXT("\u624B\u5957"),
		TEXT("\u675F\u8170"),
		TEXT("\u88E4\u5B50"),
		TEXT("\u5185\u8863"),
		TEXT("\u80F8\u7F69"),
		TEXT("\u5185\u88E4"),
		TEXT("\u889C\u5B50"),
		TEXT("\u978B\u5B50"),
	};

	return FMath::IsWithinInclusive(Slot, 0, SlotCount - 1) ? FString(Names[Slot]) : FString();
}
