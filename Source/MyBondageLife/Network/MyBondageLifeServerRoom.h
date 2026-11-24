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
	FString HostAccount;

	FString HostCharacterName;

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	FString Description;

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	FString Language;

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	bool bHasPassword = false;

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	bool bIsFull = false;

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	FString VisitMode;

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	TArray<FString> Admins;

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	TArray<FString> Whitelist;

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	TArray<FString> Blacklist;

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
		FString LanguageText = TEXT("中文");
		if (Language == TEXT("en")) { LanguageText = TEXT("English"); }
		else if (Language == TEXT("ko")) { LanguageText = TEXT("한국어"); }
		else if (Language == TEXT("ja")) { LanguageText = TEXT("日本語"); }

		FString ModeText = TEXT("公开");
		if (VisitMode == TEXT("admin")) { ModeText = TEXT("仅管理员"); }
		else if (VisitMode == TEXT("admin_whitelist")) { ModeText = TEXT("管理员+白名单"); }

		FString Line1 = FString::Printf(TEXT("%s  %s  %s  %d/%d  %s"),
			*RoomName,
			*LanguageText,
			bHasPassword ? TEXT("已加密") : TEXT(""),
			CurrentPlayers,
			MaxPlayers,
			*ModeText);

		if (!Description.IsEmpty())
		{
			Line1 += TEXT("\n") + Description;
		}
		return Line1;
	}};
