// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeRestraintCatalog.h"

#include "Animation/AnimMontage.h"
#include "Engine/DataTable.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "MyBondageLife.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* RestraintTablePath = TEXT("/Game/DataTable/DT_Restraint.DT_Restraint");

	/** True when the property name starts with one of the accepted prefixes. */
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

	/** True when a property with one of the accepted prefixes exists at all. */
	bool HasProperty(const UScriptStruct* RowStruct, std::initializer_list<const TCHAR*> Prefixes)
	{
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			if (MatchesName(*It, Prefixes))
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

	/** Reads the display name (text / string / name property). */
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
	/** Reads an object reference of the requested class (hard or soft reference). */
	void ReadObjectRef(
		const UScriptStruct* RowStruct,
		const uint8* RowData,
		std::initializer_list<const TCHAR*> Prefixes,
		UClass* ExpectedClass,
		FSoftObjectPtr& OutPtr)
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
					OutPtr = Value;
				}
				return;
			}

			if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(*It))
			{
				if (UObject* Value = ObjectProperty->GetObjectPropertyValue_InContainer(RowData))
				{
					if (Value->IsA(ExpectedClass))
					{
						OutPtr = FSoftObjectPtr(Value);
					}
				}
				return;
			}
		}
	}
}

TArray<FMyBondageLifeRestraintItem> FMyBondageLifeRestraintCatalog::Items;
bool FMyBondageLifeRestraintCatalog::bLoaded = false;

void FMyBondageLifeRestraintCatalog::Refresh()
{
	Items.Reset();
	bLoaded = true;

	const UDataTable* Table = LoadObject<UDataTable>(nullptr, RestraintTablePath);
	if (!Table)
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("Restraint: %s not found."), RestraintTablePath);
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

		FMyBondageLifeRestraintItem Item;
		Item.ItemId = RowName;
		Item.Name = ReadName(RowStruct, RowData);
		Item.Slot = FMath::Clamp(ReadInt(RowStruct, RowData, { TEXT("Slot") }), 0, SlotCount - 1);

		// The struct has a typo ("RestranitArm?"), so accept both spellings.
		Item.bRestraintArm = ReadBool(RowStruct, RowData, { TEXT("RestranitArm"), TEXT("RestraintArm"), TEXT("RestranitArm?"), TEXT("RestraintArm?") });
		Item.bInGag = ReadBool(RowStruct, RowData, { TEXT("InGag"), TEXT("InGag?") });
		Item.bRestraintLeg = ReadBool(RowStruct, RowData, { TEXT("RestranitLeg"), TEXT("RestraintLeg"), TEXT("RestranitLeg?"), TEXT("RestraintLeg?") });
		Item.bCantMove = ReadBool(RowStruct, RowData, { TEXT("CantMove"), TEXT("CantMove?") });
		Item.bInPetMode = ReadBool(RowStruct, RowData, { TEXT("InPetMode") });

		// 口部道具的枚举类型（Enum_GagType）。旧表没有这一列时 bHasGagType 为 false。
		Item.bHasGagType = HasProperty(RowStruct, { TEXT("GagType") });
		Item.GagType = Item.bHasGagType ? ReadInt(RowStruct, RowData, { TEXT("GagType") }) : 0;
		Item.Muffle = ReadInt(RowStruct, RowData, { TEXT("Muffle") });

		FSoftObjectPtr Temp;
		ReadObjectRef(RowStruct, RowData, { TEXT("Mesh") }, USkeletalMesh::StaticClass(), Temp);
		Item.Mesh = TSoftObjectPtr<USkeletalMesh>(Temp.ToSoftObjectPath());
		Temp.Reset();

		ReadObjectRef(RowStruct, RowData, { TEXT("Icon") }, UTexture2D::StaticClass(), Temp);
		Item.Icon = TSoftObjectPtr<UTexture2D>(Temp.ToSoftObjectPath());
		Temp.Reset();

		ReadObjectRef(RowStruct, RowData, { TEXT("UpAnmi") }, UAnimMontage::StaticClass(), Temp);
		Item.UpAnmi = TSoftObjectPtr<UAnimMontage>(Temp.ToSoftObjectPath());
		Temp.Reset();

		ReadObjectRef(RowStruct, RowData, { TEXT("DownAnmi") }, UAnimMontage::StaticClass(), Temp);
		Item.DownAnmi = TSoftObjectPtr<UAnimMontage>(Temp.ToSoftObjectPath());

		Temp.Reset();
		ReadObjectRef(RowStruct, RowData, { TEXT("GagAnmi") }, UAnimMontage::StaticClass(), Temp);
		Item.GagAnmi = TSoftObjectPtr<UAnimMontage>(Temp.ToSoftObjectPath());


		UE_LOG(LogMyBondageLife, Log, TEXT("Restraint: item %s slot=%d mesh=%d icon=%d upAnmi=%d"),
			*Item.ItemId.ToString(), Item.Slot, Item.Mesh.IsNull() ? 0 : 1, Item.Icon.IsNull() ? 0 : 1, Item.UpAnmi.IsNull() ? 0 : 1);

		Items.Add(Item);
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("Restraint: %d item(s) loaded."), Items.Num());
}

void FMyBondageLifeRestraintCatalog::EnsureLoaded()
{
	if (!bLoaded)
	{
		Refresh();
	}
}

const TArray<FMyBondageLifeRestraintItem>& FMyBondageLifeRestraintCatalog::GetItems()
{
	EnsureLoaded();
	return Items;
}

const FMyBondageLifeRestraintItem* FMyBondageLifeRestraintCatalog::Find(FName ItemId)
{
	EnsureLoaded();

	for (const FMyBondageLifeRestraintItem& Item : Items)
	{
		if (Item.ItemId == ItemId)
		{
			return &Item;
		}
	}

	return nullptr;
}

TArray<const FMyBondageLifeRestraintItem*> FMyBondageLifeRestraintCatalog::GetItemsForSlot(int32 Slot)
{
	EnsureLoaded();

	TArray<const FMyBondageLifeRestraintItem*> Result;
	for (const FMyBondageLifeRestraintItem& Item : Items)
	{
		if (Item.Slot == Slot)
		{
			Result.Add(&Item);
		}
	}

	Result.Sort([](const FMyBondageLifeRestraintItem& A, const FMyBondageLifeRestraintItem& B)
	{
		return A.ItemId.LexicalLess(B.ItemId);
	});

	return Result;
}

FString FMyBondageLifeRestraintCatalog::GetSlotName(int32 Slot)
{
	// "嘴部 / 项圈 / 上身拘束 / 下身拘束 / 脚步拘束 / 固定位拘束 / 上身配件1-3 / 下身配件1-3"
	static const TCHAR* Names[SlotCount] = {
		TEXT("\u5634\u90E8"),
		TEXT("\u9879\u5708"),
		TEXT("\u4E0A\u8EAB\u62D8\u675F"),
		TEXT("\u4E0B\u8EAB\u62D8\u675F"),
		TEXT("\u811A\u6B65\u62D8\u675F"),
		TEXT("\u56FA\u5B9A\u4F4D\u62D8\u675F"),
		TEXT("\u4E0A\u8EAB\u914D\u4EF61"),
		TEXT("\u4E0A\u8EAB\u914D\u4EF62"),
		TEXT("\u4E0A\u8EAB\u914D\u4EF63"),
		TEXT("\u4E0B\u8EAB\u914D\u4EF61"),
		TEXT("\u4E0B\u8EAB\u914D\u4EF62"),
		TEXT("\u4E0B\u8EAB\u914D\u4EF63"),
	};

	return FMath::IsWithinInclusive(Slot, 0, SlotCount - 1) ? FString(Names[Slot]) : FString();
}

USkeletalMesh* FMyBondageLifeRestraintCatalog::LoadMesh(FName ItemId)
{
	const FMyBondageLifeRestraintItem* Item = Find(ItemId);
	return Item ? Item->Mesh.LoadSynchronous() : nullptr;
}

UTexture2D* FMyBondageLifeRestraintCatalog::LoadIcon(FName ItemId)
{
	const FMyBondageLifeRestraintItem* Item = Find(ItemId);
	return Item ? Item->Icon.LoadSynchronous() : nullptr;
}

FString FMyBondageLifeRestraintCatalog::GetDisplayName(FName ItemId)
{
	if (const FMyBondageLifeRestraintItem* Item = Find(ItemId))
	{
		if (!Item->Name.IsEmpty())
		{
			return Item->Name;
		}
	}

	return ItemId.ToString();
}
