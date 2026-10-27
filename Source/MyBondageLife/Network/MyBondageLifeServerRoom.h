// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MyBondageLifeServerRoom.generated.h"

/** A room as reported by the backend room service. */
USTRUCT(BlueprintType)
struct FMyBondageLifeServerRoom
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	FString RoomId;

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	FString RoomName;

	/** Host's EOS ProductUserId; the client connects to "EOS:<HostPuid>". */
	UPROPERTY(BlueprintReadOnly, Category = "Room")
	FString HostPuid;

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	FString HostCharacterName;

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	FString SocketName;

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	FString MapName;

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	FString Version;

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	int32 MaxPlayers = 0;

	/**
	 * Address handed to ClientTravel.
	 *
	 * EOS addresses must be wrapped in square brackets, otherwise FURL parses
	 * "EOS:<PUID>" as a malformed host:port pair and travel fails with InvalidURL.
	 * This matches FOnlineSessionEOS::GetResolvedConnectString.
	 */
	FString GetEOSAddress() const
	{
		return FString::Printf(TEXT("[EOS:%s]"), *HostPuid);
	}

	FString ToDisplayString() const
	{
		return FString::Printf(TEXT("%s      %s      %d/%d"),
			*RoomName,
			HostCharacterName.IsEmpty() ? TEXT("房主") : *HostCharacterName,
			CurrentPlayers,
			MaxPlayers);
	}
};
