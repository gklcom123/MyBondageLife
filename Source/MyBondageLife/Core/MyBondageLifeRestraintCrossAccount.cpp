// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/MyBondageLifeAccountSubsystem.h"

#include "MyBondageLife.h"

/**
 *  Cross-account wardrobe access.
 *
 *  The permission rules mirror docs/FriendWardrobeAPI.md:
 *    - any friend can *view* another wardrobe (handled by the UI),
 *    - equipping/removing requires actor class 1 or 2 and target class 2..4,
 *    - locking has its own rules, enforced by the server.
 */
int32 UMyBondageLifeAccountSubsystem::GetFriendClass(const FString& InAccountName) const
{
	for (const FMyBondageLifeFriendEntry& Entry : Friends)
	{
		if (Entry.AccountName.Equals(InAccountName, ESearchCase::IgnoreCase))
		{
			return Entry.CharacterClass;
		}
	}

	// Not a friend (or not loaded yet): unknown.
	return 0;
}

bool UMyBondageLifeAccountSubsystem::CanEditRestraintsOf(const FString& InAccountName) const
{
	if (InAccountName.IsEmpty() || InAccountName.Equals(AccountName, ESearchCase::IgnoreCase))
	{
		return true;
	}

	if (!IsFriend(InAccountName))
	{
		return false;
	}

	const int32 ActorClass = GetCharacterClass();
	const int32 OwnerClass = GetFriendClass(InAccountName);

	// Only dominants and switches may configure other people.
	if (ActorClass != 1 && ActorClass != 2)
	{
		return false;
	}

	// Targets must be switch / submissive / slave.
	return OwnerClass >= 2 && OwnerClass <= 4;
}

void UMyBondageLifeAccountSubsystem::RefreshRestraintsFor(const FString& InAccountName)
{
	const FString Target = InAccountName.IsEmpty() ? AccountName : InAccountName;
	ViewedAccount = Target;

	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);

	SendFriendApiRequest(TEXT("GET"), FString::Printf(TEXT("/users/%s/restraints"), *Target), nullptr,
		[WeakThis, Target](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self || !bOk || !Json.IsValid())
			{
				return;
			}

			Self->ViewedRestraints.Reset();
			Self->ViewedLocks.Reset();

			const TArray<TSharedPtr<FJsonValue>>* Equipped = nullptr;
			if (!Json->TryGetArrayField(TEXT("equipped"), Equipped) || !Equipped)
			{
				Self->OnRestraintsChanged.Broadcast();
				return;
			}

			for (const TSharedPtr<FJsonValue>& Value : *Equipped)
			{
				const TSharedPtr<FJsonObject> Item = Value.IsValid() ? Value->AsObject() : nullptr;
				if (!Item.IsValid())
				{
					continue;
				}

				double SlotValue = 0.0;
				FString ItemId;
				if (!Item->TryGetNumberField(TEXT("slot"), SlotValue) || !Item->TryGetStringField(TEXT("itemId"), ItemId))
				{
					continue;
				}

				const int32 SlotIndex = FMath::Clamp(static_cast<int32>(SlotValue), 0, 11);
				Self->ViewedRestraints.Add(SlotIndex, ItemId);

				const TSharedPtr<FJsonObject>* LockObject = nullptr;
				if (!Item->TryGetObjectField(TEXT("lock"), LockObject) || !LockObject || !LockObject->IsValid())
				{
					continue;
				}

				FMyBondageLifeRestraintLock Lock;
				double LockType = 0.0;
				LockObject->Get()->TryGetNumberField(TEXT("lockType"), LockType);
				Lock.LockType = static_cast<int32>(LockType);
				LockObject->Get()->TryGetStringField(TEXT("lockedBy"), Lock.LockedBy);
				LockObject->Get()->TryGetStringField(TEXT("lockedByName"), Lock.LockedByName);
				LockObject->Get()->TryGetBoolField(TEXT("canUnlock"), Lock.bCanUnlock);
				LockObject->Get()->TryGetStringField(TEXT("unlockReason"), Lock.UnlockReason);

				FString ExpiresAt;
				if (LockObject->Get()->TryGetStringField(TEXT("expiresAt"), ExpiresAt) && !ExpiresAt.IsEmpty())
				{
					Lock.bHasExpiry = FDateTime::ParseIso8601(*ExpiresAt, Lock.ExpiresAt);
				}

				if (Lock.LockType > 0)
				{
					Self->ViewedLocks.Add(SlotIndex, Lock);
				}
			}

			UE_LOG(LogMyBondageLife, Log, TEXT("Restraint: view %s -> %d slot(s)."), *Target, Self->ViewedRestraints.Num());
			Self->OnRestraintsChanged.Broadcast();
		});
}

FString UMyBondageLifeAccountSubsystem::GetViewedRestraint(int32 Slot) const
{
	if (const FString* Found = ViewedRestraints.Find(Slot))
	{
		return *Found;
	}
	return FString();
}

FMyBondageLifeRestraintLock UMyBondageLifeAccountSubsystem::GetViewedLock(int32 Slot) const
{
	if (const FMyBondageLifeRestraintLock* Found = ViewedLocks.Find(Slot))
	{
		return *Found;
	}
	return FMyBondageLifeRestraintLock();
}

void UMyBondageLifeAccountSubsystem::EquipRestraintFor(const FString& InAccountName, int32 Slot, const FString& ItemId)
{
	if (Slot < 0 || Slot > 11 || ItemId.IsEmpty())
	{
		return;
	}

	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("itemId"), ItemId);

	const FString Path = FString::Printf(TEXT("/users/%s/restraints/%d"), *InAccountName, Slot);
	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);
	const FString Target = InAccountName;

	SendFriendApiRequest(TEXT("PUT"), Path, Body,
		[WeakThis, Target](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			UE_LOG(LogMyBondageLife, Log, TEXT("Restraint: equip on %s %s (%d)."),
				*Target, bOk ? TEXT("ok") : TEXT("failed"), StatusCode);
			Self->RefreshRestraintsFor(Target);
		});
}

void UMyBondageLifeAccountSubsystem::UnequipRestraintFor(const FString& InAccountName, int32 Slot)
{
	if (Slot < 0 || Slot > 11)
	{
		return;
	}

	const FString Path = FString::Printf(TEXT("/users/%s/restraints/%d"), *InAccountName, Slot);
	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);
	const FString Target = InAccountName;

	SendFriendApiRequest(TEXT("DELETE"), Path, MakeShared<FJsonObject>(),
		[WeakThis, Target](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			UE_LOG(LogMyBondageLife, Log, TEXT("Restraint: unequip on %s %s (%d)."),
				*Target, bOk ? TEXT("ok") : TEXT("failed"), StatusCode);
			Self->RefreshRestraintsFor(Target);
		});
}

void UMyBondageLifeAccountSubsystem::LockRestraintFor(const FString& InAccountName, int32 Slot, int32 LockType, int32 DurationSeconds, const FString& Password)
{
	if (Slot < 0 || Slot > 11 || LockType < 1 || LockType > 5)
	{
		return;
	}

	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetNumberField(TEXT("lockType"), LockType);
	if (DurationSeconds > 0)
	{
		Body->SetNumberField(TEXT("durationSeconds"), DurationSeconds);
	}
	if (!Password.IsEmpty())
	{
		Body->SetStringField(TEXT("password"), Password);
	}

	const FString Path = FString::Printf(TEXT("/users/%s/restraints/%d/lock"), *InAccountName, Slot);
	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);
	const FString Target = InAccountName;

	SendFriendApiRequest(TEXT("POST"), Path, Body,
		[WeakThis, Target](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			UE_LOG(LogMyBondageLife, Log, TEXT("Restraint: lock on %s %s (%d)."),
				*Target, bOk ? TEXT("ok") : TEXT("failed"), StatusCode);
			Self->RefreshRestraintsFor(Target);
		});
}

void UMyBondageLifeAccountSubsystem::UnlockRestraintFor(const FString& InAccountName, int32 Slot, const FString& Password)
{
	if (Slot < 0 || Slot > 11)
	{
		return;
	}

	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	if (!Password.IsEmpty())
	{
		Body->SetStringField(TEXT("password"), Password);
	}

	const FString Path = FString::Printf(TEXT("/users/%s/restraints/%d/lock"), *InAccountName, Slot);
	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);
	const FString Target = InAccountName;

	SendFriendApiRequest(TEXT("DELETE"), Path, Body,
		[WeakThis, Target](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			UE_LOG(LogMyBondageLife, Log, TEXT("Restraint: unlock on %s %s (%d)."),
				*Target, bOk ? TEXT("ok") : TEXT("failed"), StatusCode);
			Self->RefreshRestraintsFor(Target);
		});
}
