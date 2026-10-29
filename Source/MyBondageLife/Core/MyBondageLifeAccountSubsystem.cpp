// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeAccountSubsystem.h"

#include "Dom/JsonObject.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "MyBondageLife.h"
#include "MyBondageLifeServerConfig.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	/** Maps the service role string to the enum used by the UI. */
	EMyBondageLifeContractRole ParseContractRole(const FString& RoleText)
	{
		if (RoleText == TEXT("temporary_master")) { return EMyBondageLifeContractRole::TemporaryMaster; }
		if (RoleText == TEXT("temporary_slave")) { return EMyBondageLifeContractRole::TemporarySlave; }
		if (RoleText == TEXT("master")) { return EMyBondageLifeContractRole::Master; }
		if (RoleText == TEXT("slave")) { return EMyBondageLifeContractRole::Slave; }
		return EMyBondageLifeContractRole::None;
	}

	/** Reads an ISO-8601 timestamp field; a null time when absent. */
	FDateTime ParseIsoField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		FString Text;
		FDateTime Value;
		if (Object.IsValid() && Object->TryGetStringField(Field, Text) && FDateTime::ParseIso8601(*Text, Value))
		{
			return Value;
		}
		return FDateTime();
	}

	/** Reads one entry of /contracts/me (asSlave object or one asMaster item). */
	FMyBondageLifeContractInfo ParseContractEntry(const TSharedPtr<FJsonObject>& Object, bool bAsMaster)
	{
		FMyBondageLifeContractInfo Info;
		if (!Object.IsValid())
		{
			return Info;
		}

		Object->TryGetStringField(TEXT("contractId"), Info.ContractId);

		FString StatusText;
		Object->TryGetStringField(TEXT("status"), StatusText);
		const bool bFormal = StatusText == TEXT("formal");

		const TSharedPtr<FJsonObject>* PeerObject = nullptr;
		if (Object->TryGetObjectField(bAsMaster ? TEXT("slave") : TEXT("master"), PeerObject) && PeerObject && PeerObject->IsValid())
		{
			(*PeerObject)->TryGetStringField(TEXT("account"), Info.PeerAccount);
			(*PeerObject)->TryGetStringField(TEXT("characterName"), Info.PeerCharacterName);

			double ClassValue = 1.0;
			if ((*PeerObject)->TryGetNumberField(TEXT("characterClass"), ClassValue))
			{
				Info.PeerClass = FMath::Clamp(static_cast<int32>(ClassValue), 1, 4);
			}
		}

		Info.Role = bFormal
			? (bAsMaster ? EMyBondageLifeContractRole::Master : EMyBondageLifeContractRole::Slave)
			: (bAsMaster ? EMyBondageLifeContractRole::TemporaryMaster : EMyBondageLifeContractRole::TemporarySlave);

		const FDateTime TrialStartedAt = ParseIsoField(Object, TEXT("trialStartedAt"));
		const FDateTime FormalStartedAt = ParseIsoField(Object, TEXT("formalStartedAt"));
		Info.Since = bFormal ? FormalStartedAt : TrialStartedAt;
		Info.TrialEndsAt = ParseIsoField(Object, TEXT("trialEndsAt"));

		return Info;
	}

	/** Extracts the error code of a failed backend call. */
	FString ContractErrorCode(const TSharedPtr<FJsonObject>& Json)
	{
		FString Code;
		if (Json.IsValid())
		{
			const TSharedPtr<FJsonObject>* ErrorObject = nullptr;
			if (Json->TryGetObjectField(TEXT("error"), ErrorObject) && ErrorObject && ErrorObject->IsValid())
			{
				(*ErrorObject)->TryGetStringField(TEXT("code"), Code);
			}
		}
		return Code;
	}
	/** Request timeout; a slower answer is reported as "server not responding". */
	constexpr float AuthRequestTimeoutSeconds = 8.0f;
}

FString UMyBondageLifeAccountSubsystem::GetServerUrl()
{
	// Endpoint comes from Config/ServerConfig.ini, never hard coded.
	return FMyBondageLifeServerConfig::GetBaseUrl();
}

void UMyBondageLifeAccountSubsystem::RequestLogin(const FString& Account, const FString& Password)
{
	if (bAuthInFlight)
	{
		return;
	}

	TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("account"), Account);
	Payload->SetStringField(TEXT("password"), Password);

	SendAuthRequest(TEXT("login"), Payload, false);
}

void UMyBondageLifeAccountSubsystem::RequestRegister(const FString& Account, const FString& Password, const FString& InCharacterName, int32 InCharacterClass)
{
	if (bAuthInFlight)
	{
		return;
	}

	TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("account"), Account);
	Payload->SetStringField(TEXT("password"), Password);
	Payload->SetStringField(TEXT("characterName"), InCharacterName);
	Payload->SetNumberField(TEXT("characterClass"), InCharacterClass);

	SendAuthRequest(TEXT("register"), Payload, true);
}

void UMyBondageLifeAccountSubsystem::SendAuthRequest(const FString& Path, const TSharedRef<FJsonObject>& Payload, bool bIsRegistration)
{
	FString Body;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(Payload, Writer);

	const FString Url = FString::Printf(TEXT("%s/auth/%s"), *GetServerUrl(), *Path);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	Request->SetContentAsString(Body);
	Request->SetTimeout(AuthRequestTimeoutSeconds);

	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);
	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis, bIsRegistration](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bConnectedSuccessfully)
		{
			if (UMyBondageLifeAccountSubsystem* Self = WeakThis.Get())
			{
				Self->HandleAuthResponse(HttpRequest, HttpResponse, bConnectedSuccessfully, bIsRegistration);
			}
		});

	bAuthInFlight = true;

	UE_LOG(LogMyBondageLife, Log, TEXT("Account: POST %s (%s)"), *Url, bIsRegistration ? TEXT("register") : TEXT("login"));

	if (!Request->ProcessRequest())
	{
		bAuthInFlight = false;
		FailAuth(FText::FromString(TEXT("\u65E0\u6CD5\u53D1\u9001\u8BF7\u6C42\uFF0C\u8BF7\u68C0\u67E5\u7F51\u7EDC")));
	}
}

void UMyBondageLifeAccountSubsystem::HandleAuthResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, bool bIsRegistration)
{
	bAuthInFlight = false;

	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		// "服务器无响应"
		FailAuth(FText::FromString(TEXT("\u670D\u52A1\u5668\u65E0\u54CD\u5E94\uFF0C\u8BF7\u786E\u8BA4\u670D\u52A1\u5668\u5DF2\u542F\u52A8\u6216\u7A0D\u540E\u91CD\u8BD5")));
		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	const FString Body = Response->GetContentAsString();

	if (StatusCode >= 200 && StatusCode < 300)
	{
		TSharedPtr<FJsonObject> Json;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
		if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid() || !ApplyAuthPayload(Json))
		{
			FailAuth(FText::FromString(TEXT("\u670D\u52A1\u5668\u8FD4\u56DE\u7684\u6570\u636E\u65E0\u6CD5\u89E3\u6790")));
			return;
		}

		UE_LOG(LogMyBondageLife, Log, TEXT("Account: %s succeeded for '%s' (character '%s', class %d)"),
			bIsRegistration ? TEXT("registration") : TEXT("login"), *AccountName, *CharacterName, CharacterClass);

		OnAuthSucceeded.Broadcast();
		return;
	}

	// Error payload: { "error": { "code": "...", "message": "..." } }
	FString ErrorCode;
	FString ServerMessage;
	TSharedPtr<FJsonObject> Json;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
	if (FJsonSerializer::Deserialize(Reader, Json) && Json.IsValid())
	{
		const TSharedPtr<FJsonObject>* ErrorObject = nullptr;
		if (Json->TryGetObjectField(TEXT("error"), ErrorObject) && ErrorObject && ErrorObject->IsValid())
		{
			(*ErrorObject)->TryGetStringField(TEXT("code"), ErrorCode);
			(*ErrorObject)->TryGetStringField(TEXT("message"), ServerMessage);
		}
	}

	UE_LOG(LogMyBondageLife, Warning, TEXT("Account: request failed (%d) code='%s' message='%s'"),
		StatusCode, *ErrorCode, *ServerMessage);

	FailAuth(MakeErrorText(StatusCode, ErrorCode, ServerMessage, bIsRegistration));
}

bool UMyBondageLifeAccountSubsystem::ApplyAuthPayload(const TSharedPtr<FJsonObject>& Payload)
{
	if (!Payload->TryGetStringField(TEXT("token"), Token) || Token.IsEmpty())
	{
		return false;
	}

	const TSharedPtr<FJsonObject>* UserObject = nullptr;
	if (!Payload->TryGetObjectField(TEXT("user"), UserObject) || !UserObject || !UserObject->IsValid())
	{
		return false;
	}

	(*UserObject)->TryGetStringField(TEXT("id"), AccountId);
	(*UserObject)->TryGetStringField(TEXT("account"), AccountName);
	(*UserObject)->TryGetStringField(TEXT("characterName"), CharacterName);

	double ClassValue = 1.0;
	if ((*UserObject)->TryGetNumberField(TEXT("characterClass"), ClassValue))
	{
		CharacterClass = FMath::Clamp(static_cast<int32>(ClassValue), 1, 4);
	}

	// Body height multiplier: 0.9 / 1.0 / 1.1 (server keeps the whitelist).
	double HeightValue = 1.0;
	if ((*UserObject)->TryGetNumberField(TEXT("heightScale"), HeightValue))
	{
		HeightScale = FMath::Clamp(static_cast<float>(HeightValue), 0.5f, 2.0f);
	}

	// Hair style: the DT_Hair row ID, 1 based.
	double HairValue = 1.0;
	if ((*UserObject)->TryGetNumberField(TEXT("hairIndex"), HairValue))
	{
		HairIndex = FMath::Clamp(static_cast<int32>(HairValue), 1, 64);
	}

	// e.g. "2026-10-26T10:39:29.323Z" - the account creation date shown in the
	// personal info panel.
	FString CreatedAtString;
	if ((*UserObject)->TryGetStringField(TEXT("createdAt"), CreatedAtString))
	{
		FDateTime ParsedCreatedAt;
		if (FDateTime::ParseIso8601(*CreatedAtString, ParsedCreatedAt))
		{
			CreatedAt = ParsedCreatedAt;
		}
		else
		{
			UE_LOG(LogMyBondageLife, Warning, TEXT("Account: could not parse createdAt '%s'."), *CreatedAtString);
		}
	}

	if (CharacterName.IsEmpty())
	{
		CharacterName = TEXT("\u89D2\u8272\u540D");
	}

	bLoggedIn = true;


	// Friends are served by the backend: start polling and load the list.

	StartFriendPolling();

	RefreshFriendsFromServer();

	RefreshContractState();
	RefreshRestraints();
	return true;
}

void UMyBondageLifeAccountSubsystem::FailAuth(const FText& Message)
{
	OnAuthFailed.Broadcast(Message);
}

void UMyBondageLifeAccountSubsystem::LogOut()
{
	UE_LOG(LogMyBondageLife, Log, TEXT("Account: signed out '%s'."), *AccountName);

	// Tell the service this account is going offline before the token is dropped.

	SendPresenceOffline();


	bLoggedIn = false;
	StopFriendPolling();

	Friends.Reset();

	SeenRequestIds.Reset();
	AccountName.Reset();
	AccountId.Reset();
	Token.Reset();
	CharacterName.Reset();
	CharacterClass = 1;
	CreatedAt = FDateTime();
}

FString UMyBondageLifeAccountSubsystem::GetDefaultRoomName() const
{
	// "<角色名>的房间"; the account name is only a fallback before the
	// character identity is known.
	const FString RoomOwnerName = CharacterName.IsEmpty()
		? (AccountName.IsEmpty() ? FString(TEXT("Player")) : AccountName)
		: CharacterName;

	return FString::Printf(TEXT("%s\u7684\u623F\u95F4"), *RoomOwnerName);
}

FText UMyBondageLifeAccountSubsystem::MakeErrorText(int32 StatusCode, const FString& ErrorCode, const FString& ServerMessage, bool bIsRegistration)
{
	if (ErrorCode == TEXT("ACCOUNT_TAKEN"))
	{
		return FText::FromString(TEXT("\u8BE5\u8D26\u53F7\u5DF2\u88AB\u6CE8\u518C\uFF0C\u8BF7\u66F4\u6362\u8D26\u53F7ID"));
	}

	if (ErrorCode == TEXT("INVALID_CREDENTIALS"))
	{
		// The service deliberately does not say which half was wrong (anti-enumeration).
		return FText::FromString(TEXT("\u8D26\u53F7\u6216\u5BC6\u7801\u9519\u8BEF"));
	}

	if (ErrorCode == TEXT("VALIDATION_ERROR"))
	{
		if (ServerMessage.Contains(TEXT("password")))
		{
			return FText::FromString(TEXT("\u5BC6\u7801\u957F\u5EA6\u4E0D\u7B26\u5408\u670D\u52A1\u5668\u8981\u6C42\uFF08\u81F3\u5C11 8 \u4F4D\uFF09"));
		}
		if (ServerMessage.Contains(TEXT("account")))
		{
			return FText::FromString(TEXT("\u8D26\u53F7ID \u683C\u5F0F\u4E0D\u5BF9\uFF08\u5B57\u6BCD\u3001\u6570\u5B57\u6216\u4E0B\u5212\u7EBF\uFF09"));
		}
		if (ServerMessage.Contains(TEXT("characterName")))
		{
			return FText::FromString(TEXT("\u89D2\u8272\u540D\u4E0D\u5408\u6CD5\uFF08\u6700\u591A 8 \u4E2A\u5B57\u7B26\uFF09"));
		}
		if (ServerMessage.Contains(TEXT("characterClass")))
		{
			return FText::FromString(TEXT("\u89D2\u8272\u7C7B\u522B\u5FC5\u987B\u662F 1\u30012\u30013 \u6216 4"));
		}

		return FText::FromString(FString::Printf(TEXT("\u63D0\u4EA4\u7684\u6570\u636E\u4E0D\u5408\u6CD5\uFF1A%s"), *ServerMessage));
	}

	if (StatusCode == 401)
	{
		return FText::FromString(TEXT("\u8D26\u53F7\u6216\u5BC6\u7801\u9519\u8BEF"));
	}
	if (StatusCode == 409)
	{
		return FText::FromString(TEXT("\u8BE5\u8D26\u53F7\u5DF2\u88AB\u6CE8\u518C\uFF0C\u8BF7\u66F4\u6362\u8D26\u53F7ID"));
	}
	if (StatusCode == 404)
	{
		return FText::FromString(TEXT("\u670D\u52A1\u7AEF\u4E0D\u5B58\u5728\u8BE5\u63A5\u53E3\uFF0C\u8BF7\u786E\u8BA4\u670D\u52A1\u5668\u7248\u672C"));
	}
	if (StatusCode >= 500)
	{
		return FText::FromString(TEXT("\u670D\u52A1\u5668\u5185\u90E8\u9519\u8BEF\uFF0C\u8BF7\u7A0D\u540E\u91CD\u8BD5"));
	}

	if (!ServerMessage.IsEmpty())
	{
		return FText::FromString(FString::Printf(TEXT("%s\uFF08%s\uFF09"), *ServerMessage, *ErrorCode));
	}

	return FText::FromString(FString::Printf(
		TEXT("%s\u5931\u8D25\uFF08HTTP %d\uFF09"),
		bIsRegistration ? TEXT("\u6CE8\u518C") : TEXT("\u767B\u5F55"), StatusCode));
}

FString UMyBondageLifeAccountSubsystem::MakeCharacterClassLabel(int32 ClassValue)
{
	switch (ClassValue)
	{
	case 1: return TEXT("\u652F\u914D\u8005");   // 支配者
	case 2: return TEXT("\u8F6C\u6362\u8005");   // 转换者
	case 3: return TEXT("\u81E3\u670D\u8005");   // 臣服者
	case 4: return TEXT("\u5974\u96B6");         // 奴隶
	default: return TEXT("---");
	}
}

FString UMyBondageLifeAccountSubsystem::GetCharacterClassLabel() const
{
	switch (CharacterClass)
	{
	case 1: return TEXT("\u652F\u914D\u8005");   // 支配者
	case 2: return TEXT("\u8F6C\u6362\u8005");   // 转换者
	case 3: return TEXT("\u81E3\u670D\u8005");   // 臣服者
	case 4: return TEXT("\u5974\u96B6");         // 奴隶
	default: return TEXT("---");
	}
}

bool UMyBondageLifeAccountSubsystem::IsFriend(const FString& InAccountName) const
{
	return Friends.ContainsByPredicate([&InAccountName](const FMyBondageLifeFriendEntry& Entry)
	{
		return Entry.AccountName == InAccountName;
	});
}

void UMyBondageLifeAccountSubsystem::AddFriend(const FMyBondageLifeFriendEntry& Entry)
{
	if (Entry.AccountName.IsEmpty())
	{
		return;
	}

	if (FMyBondageLifeFriendEntry* Existing = Friends.FindByPredicate(
		[&Entry](const FMyBondageLifeFriendEntry& Candidate)
		{
			return Candidate.AccountName == Entry.AccountName;
		}))
	{
		*Existing = Entry;
	}
	else
	{
		Friends.Add(Entry);
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("Friends: added '%s' (%s)."), *Entry.CharacterName, *Entry.AccountName);
	OnFriendsChanged.Broadcast();
}

void UMyBondageLifeAccountSubsystem::SetFriendOnline(const FString& InAccountName, bool bInOnline)
{
	FMyBondageLifeFriendEntry* Existing = Friends.FindByPredicate(
		[&InAccountName](const FMyBondageLifeFriendEntry& Candidate)
		{
			return Candidate.AccountName == InAccountName;
		});

	if (!Existing || Existing->bOnline == bInOnline)
	{
		return;
	}

	Existing->bOnline = bInOnline;
	OnFriendsChanged.Broadcast();
}

// ---------------------------------------------------------------------------
// Friends (REST: /friends, /friends/requests, /presence/heartbeat)
// ---------------------------------------------------------------------------

void UMyBondageLifeAccountSubsystem::SendFriendApiRequest(
	const FString& Verb,
	const FString& Path,
	const TSharedPtr<FJsonObject>& Body,
	TFunction<void(bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)>&& OnDone)
{
	if (!bLoggedIn || Token.IsEmpty())
	{
		if (OnDone)
		{
			OnDone(false, 0, nullptr);
		}
		return;
	}

	FString BodyString;
	if (Body.IsValid())
	{
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
		FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);
	}

	const FString Url = FString::Printf(TEXT("%s%s"), *GetServerUrl(), *Path);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(Verb);
	// Only announce a JSON body when one is actually sent: Fastify rejects a request
	// Fastify needs a JSON parser for every body-carrying verb: a DELETE without a
	// body is rejected (415), so verbs other than GET always send "{}".
	if (BodyString.IsEmpty() && !Verb.Equals(TEXT("GET"), ESearchCase::IgnoreCase))
	{
		BodyString = TEXT("{}");
	}

	if (!BodyString.IsEmpty())
	{
		Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		Request->SetContentAsString(BodyString);
	}
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Token));
	Request->SetTimeout(AuthRequestTimeoutSeconds);

	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);
	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis, Verb, Path, OnDone](FHttpRequestPtr, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			if (!bConnectedSuccessfully || !Response.IsValid())
			{
				UE_LOG(LogMyBondageLife, Warning, TEXT("Friends: %s %s failed (no response)."), *Verb, *Path);
				if (OnDone)
				{
					OnDone(false, 0, nullptr);
				}
				return;
			}

			const int32 StatusCode = Response->GetResponseCode();
			const FString BodyText = Response->GetContentAsString();

			TSharedPtr<FJsonObject> Json;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(BodyText);
			if (!FJsonSerializer::Deserialize(Reader, Json))
			{
				Json = nullptr;
			}

			if (StatusCode >= 200 && StatusCode < 300)
			{
				UE_LOG(LogMyBondageLife, Log, TEXT("Friends: %s %s -> %d"), *Verb, *Path, StatusCode);
				if (OnDone)
				{
					OnDone(true, StatusCode, Json);
				}
				return;
			}

			FString ErrorCode;
			FString ErrorMessage;
			if (Json.IsValid())
			{
				const TSharedPtr<FJsonObject>* ErrorObject = nullptr;
				if (Json->TryGetObjectField(TEXT("error"), ErrorObject) && ErrorObject && ErrorObject->IsValid())
				{
					(*ErrorObject)->TryGetStringField(TEXT("code"), ErrorCode);
					(*ErrorObject)->TryGetStringField(TEXT("message"), ErrorMessage);
				}
			}

			UE_LOG(LogMyBondageLife, Warning, TEXT("Friends: %s %s -> %d %s %s"),
				*Verb, *Path, StatusCode, *ErrorCode, *ErrorMessage);

			if (OnDone)
			{
				OnDone(false, StatusCode, Json);
			}
		});

	Request->ProcessRequest();
}

void UMyBondageLifeAccountSubsystem::RefreshFriendsFromServer()
{
	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);

	SendFriendApiRequest(TEXT("GET"), TEXT("/friends"), nullptr,
		[WeakThis](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self || !bOk || !Json.IsValid())
			{
				return;
			}

			const TArray<TSharedPtr<FJsonValue>>* FriendsJson = nullptr;
			if (!Json->TryGetArrayField(TEXT("friends"), FriendsJson) || !FriendsJson)
			{
				return;
			}

			TArray<FMyBondageLifeFriendEntry> NewFriends;
			for (const TSharedPtr<FJsonValue>& Value : *FriendsJson)
			{
				const TSharedPtr<FJsonObject> Item = Value.IsValid() ? Value->AsObject() : nullptr;
				if (!Item.IsValid())
				{
					continue;
				}

				FMyBondageLifeFriendEntry Entry;
				Item->TryGetStringField(TEXT("account"), Entry.AccountName);
				Item->TryGetStringField(TEXT("characterName"), Entry.CharacterName);

				double ClassValue = 1.0;
				if (Item->TryGetNumberField(TEXT("characterClass"), ClassValue))
				{
					Entry.CharacterClass = FMath::Clamp(static_cast<int32>(ClassValue), 1, 4);
				}

								Item->TryGetBoolField(TEXT("online"), Entry.bOnline);

				// Optional contract block: role is relative to the signed-in player.
				const TSharedPtr<FJsonObject>* ContractObject = nullptr;
				if (Item->TryGetObjectField(TEXT("contract"), ContractObject) && ContractObject && ContractObject->IsValid())
				{
					FString RoleText;
					(*ContractObject)->TryGetStringField(TEXT("role"), RoleText);
					Entry.ContractRole = ParseContractRole(RoleText);
					(*ContractObject)->TryGetStringField(TEXT("contractId"), Entry.ContractId);
					Entry.ContractSince = ParseIsoField(*ContractObject, TEXT("since"));
					Entry.ContractTrialEndsAt = ParseIsoField(*ContractObject, TEXT("trialEndsAt"));
				}

				if (!Entry.AccountName.IsEmpty())
				{
					NewFriends.Add(Entry);
				}
			}

			Self->Friends = NewFriends;
			Self->OnFriendsChanged.Broadcast();
		});
}

void UMyBondageLifeAccountSubsystem::RequestAddFriend(const FString& TargetAccount)
{
	if (TargetAccount.IsEmpty())
	{
		return;
	}

	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("account"), TargetAccount);

	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);

	SendFriendApiRequest(TEXT("POST"), TEXT("/friends/requests"), Body,
		[WeakThis, TargetAccount](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			Self->OnFriendRequestSent.Broadcast(bOk, TargetAccount);
		});
}

void UMyBondageLifeAccountSubsystem::RespondToFriendRequest(const FString& RequestId, bool bAccept, const FString& RequesterCharacterName)
{
	if (RequestId.IsEmpty())
	{
		return;
	}

	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetBoolField(TEXT("accept"), bAccept);

	const FString Path = FString::Printf(TEXT("/friends/requests/%s/respond"), *RequestId);

	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);

	SendFriendApiRequest(TEXT("POST"), Path, Body,
		[WeakThis, bAccept, RequesterCharacterName](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			UE_LOG(LogMyBondageLife, Log, TEXT("Friends: answered request (%s) -> %s"),
				bAccept ? TEXT("accept") : TEXT("decline"), bOk ? TEXT("ok") : TEXT("failed"));

			if (bOk)
			{
				Self->RefreshFriendsFromServer();
			}

			Self->OnFriendRequestAnswered.Broadcast(bOk && bAccept);
		});
}

void UMyBondageLifeAccountSubsystem::RemoveFriendOnServer(const FString& TargetAccount)
{
	if (TargetAccount.IsEmpty())
	{
		return;
	}

	const FString Path = FString::Printf(TEXT("/friends/%s"), *TargetAccount);

	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);

	SendFriendApiRequest(TEXT("DELETE"), Path, nullptr,
		[WeakThis](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (Self && bOk)
			{
				Self->RefreshFriendsFromServer();
			}
		});
}

void UMyBondageLifeAccountSubsystem::PollFriendRequests()
{
	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);

	SendFriendApiRequest(TEXT("GET"), TEXT("/friends/requests?direction=incoming&status=pending"), nullptr,
		[WeakThis](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self || !bOk || !Json.IsValid())
			{
				return;
			}

			const TArray<TSharedPtr<FJsonValue>>* Requests = nullptr;
			if (!Json->TryGetArrayField(TEXT("requests"), Requests) || !Requests)
			{
				return;
			}

			for (const TSharedPtr<FJsonValue>& Value : *Requests)
			{
				const TSharedPtr<FJsonObject> Item = Value.IsValid() ? Value->AsObject() : nullptr;
				if (!Item.IsValid())
				{
					continue;
				}

				FString RequestId;
				Item->TryGetStringField(TEXT("requestId"), RequestId);
				if (RequestId.IsEmpty() || Self->SeenRequestIds.Contains(RequestId))
				{
					continue;
				}

				FString FromName = TEXT("---");
				FString FromAccount;
				int32 FromClass = 1;

				const TSharedPtr<FJsonObject>* FromObject = nullptr;
				if (Item->TryGetObjectField(TEXT("from"), FromObject) && FromObject && FromObject->IsValid())
				{
					(*FromObject)->TryGetStringField(TEXT("characterName"), FromName);
					(*FromObject)->TryGetStringField(TEXT("account"), FromAccount);

					double ClassValue = 1.0;
					if ((*FromObject)->TryGetNumberField(TEXT("characterClass"), ClassValue))
					{
						FromClass = FMath::Clamp(static_cast<int32>(ClassValue), 1, 4);
					}
				}

				Self->SeenRequestIds.Add(RequestId);
				Self->OnFriendRequestReceived.Broadcast(RequestId, FromName);
			}
		});
}

void UMyBondageLifeAccountSubsystem::SendPresenceHeartbeat()
{
	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("state"), TEXT("in_game"));

	SendFriendApiRequest(TEXT("POST"), TEXT("/presence/heartbeat"), Body, nullptr);
}

void UMyBondageLifeAccountSubsystem::StartFriendPolling()
{
	if (FriendPollHandle.IsValid())
	{
		return;
	}

	// One ticker covers the three periodic calls: requests, friend list, heartbeat.
	FriendPollHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this,
		[this](float DeltaTime)
		{
			const double Now = FPlatformTime::Seconds();

			if (Now - LastRequestPollSeconds >= 8.0)
			{
				LastRequestPollSeconds = Now;
				PollFriendRequests();
				PollContractRequests();

			if (Now - LastContractRefreshSeconds >= 20.0)
			{
				LastContractRefreshSeconds = Now;
				RefreshContractState();
	RefreshRestraints();
			}
			}

			if (Now - LastFriendsRefreshSeconds >= 10.0)
			{
				LastFriendsRefreshSeconds = Now;
				RefreshFriendsFromServer();
			}

			if (Now - LastHeartbeatSeconds >= 20.0)
			{
				LastHeartbeatSeconds = Now;
				SendPresenceHeartbeat();
			}

			return true;
		}), 1.0f);
}

void UMyBondageLifeAccountSubsystem::StopFriendPolling()
{
	if (FriendPollHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(FriendPollHandle);
		FriendPollHandle.Reset();
	}
}

void UMyBondageLifeAccountSubsystem::SendPresenceOffline()
{
	if (!bLoggedIn || Token.IsEmpty())
	{
		return;
	}

	// Fire and forget: the service marks lastSeenAt as expired so friends see the
	// offline state immediately instead of waiting for the presence window.
	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	SendFriendApiRequest(TEXT("POST"), TEXT("/presence/offline"), Body, nullptr);

	UE_LOG(LogMyBondageLife, Log, TEXT("Friends: presence offline sent."));
}

void UMyBondageLifeAccountSubsystem::RequestUpdateHeight(float NewHeightScale)
{
	const float ClampedScale = FMath::Clamp(NewHeightScale, 0.5f, 2.0f);

	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetNumberField(TEXT("heightScale"), ClampedScale);

	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);

	SendFriendApiRequest(TEXT("PATCH"), TEXT("/users/me"), Body,
		[WeakThis, ClampedScale](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			if (!bOk)
			{
				UE_LOG(LogMyBondageLife, Warning, TEXT("Height: update rejected (%d)."), StatusCode);
				return;
			}

			Self->HeightScale = ClampedScale;
			Self->OnHeightChanged.Broadcast(Self->HeightScale);

			UE_LOG(LogMyBondageLife, Log, TEXT("Height: stored %.2f."), Self->HeightScale);
		});
}

// ---------------------------------------------------------------------------
// Contracts (REST: /contracts/me, /contracts/requests, /contracts/{id})
// ---------------------------------------------------------------------------

void UMyBondageLifeAccountSubsystem::RefreshContractState()
{
	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);

	SendFriendApiRequest(TEXT("GET"), TEXT("/contracts/me"), nullptr,
		[WeakThis](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self || !bOk || !Json.IsValid())
			{
				return;
			}

			Self->bHasMaster = false;
			Self->ContractAsSlave = FMyBondageLifeContractInfo();
			Self->ContractsAsMaster.Reset();

			const TSharedPtr<FJsonObject>* SlaveObject = nullptr;
			if (Json->TryGetObjectField(TEXT("asSlave"), SlaveObject) && SlaveObject && SlaveObject->IsValid())
			{
				Self->ContractAsSlave = ParseContractEntry(*SlaveObject, false);
				Self->bHasMaster = !Self->ContractAsSlave.ContractId.IsEmpty();
			}

			const TArray<TSharedPtr<FJsonValue>>* MasterArray = nullptr;
			if (Json->TryGetArrayField(TEXT("asMaster"), MasterArray) && MasterArray)
			{
				for (const TSharedPtr<FJsonValue>& Value : *MasterArray)
				{
					const TSharedPtr<FJsonObject> Item = Value.IsValid() ? Value->AsObject() : nullptr;
					if (Item.IsValid())
					{
						Self->ContractsAsMaster.Add(ParseContractEntry(Item, true));
					}
				}
			}

			// Any change invalidates the cached public profiles (they carry contract info).
			Self->UserProfiles.Reset();
			Self->UserProfileFetchSeconds.Reset();

			Self->OnContractsChanged.Broadcast();
		});
}

void UMyBondageLifeAccountSubsystem::RequestContract(const FString& TargetAccount, bool bFormal)
{
	if (TargetAccount.IsEmpty())
	{
		return;
	}

	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("account"), TargetAccount);
	Body->SetStringField(TEXT("kind"), bFormal ? TEXT("formal") : TEXT("trial"));

	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);

	SendFriendApiRequest(TEXT("POST"), TEXT("/contracts/requests"), Body,
		[WeakThis](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			const FString ErrorCode = bOk ? FString() : ContractErrorCode(Json);
			UE_LOG(LogMyBondageLife, Log, TEXT("Contract: request sent (%s%s)."),
				bOk ? TEXT("ok") : TEXT("failed"), bOk ? TEXT("") : *FString::Printf(TEXT(", %s"), *ErrorCode));

			Self->OnContractActionFinished.Broadcast(bOk, ErrorCode, 1);

			if (bOk)
			{
				Self->RefreshContractState();
			}
		});
}

void UMyBondageLifeAccountSubsystem::RespondToContractRequest(const FString& RequestId, bool bAccept)
{
	if (RequestId.IsEmpty())
	{
		return;
	}

	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetBoolField(TEXT("accept"), bAccept);

	const FString Path = FString::Printf(TEXT("/contracts/requests/%s/respond"), *RequestId);
	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);

	SendFriendApiRequest(TEXT("POST"), Path, Body,
		[WeakThis, bAccept](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			const FString ErrorCode = bOk ? FString() : ContractErrorCode(Json);
			UE_LOG(LogMyBondageLife, Log, TEXT("Contract: respond %s (%s%s)."),
				bAccept ? TEXT("accept") : TEXT("decline"),
				bOk ? TEXT("ok") : TEXT("failed"),
				bOk ? TEXT("") : *FString::Printf(TEXT(", %s"), *ErrorCode));

			Self->OnContractActionFinished.Broadcast(bOk, ErrorCode, bAccept ? 2 : 4);

			if (bOk)
			{
				Self->RefreshContractState();
				Self->RefreshFriendsFromServer();
			}
		});
}

void UMyBondageLifeAccountSubsystem::DissolveContract(const FString& ContractId)
{
	if (ContractId.IsEmpty())
	{
		return;
	}

	const FString Path = FString::Printf(TEXT("/contracts/%s"), *ContractId);
	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);

	SendFriendApiRequest(TEXT("DELETE"), Path, nullptr,
		[WeakThis](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			const FString ErrorCode = bOk ? FString() : ContractErrorCode(Json);
			UE_LOG(LogMyBondageLife, Log, TEXT("Contract: dissolve (%s%s)."),
				bOk ? TEXT("ok") : TEXT("failed"),
				bOk ? TEXT("") : *FString::Printf(TEXT(", %s"), *ErrorCode));

			Self->OnContractActionFinished.Broadcast(bOk, ErrorCode, 3);

			if (bOk)
			{
				Self->RefreshContractState();
				Self->RefreshFriendsFromServer();
			}
		});
}

void UMyBondageLifeAccountSubsystem::PollContractRequests()
{
	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);

	SendFriendApiRequest(TEXT("GET"), TEXT("/contracts/requests?direction=incoming&status=pending"), nullptr,
		[WeakThis](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self || !bOk || !Json.IsValid())
			{
				return;
			}

			const TArray<TSharedPtr<FJsonValue>>* Requests = nullptr;
			if (!Json->TryGetArrayField(TEXT("requests"), Requests) || !Requests)
			{
				return;
			}

			for (const TSharedPtr<FJsonValue>& Value : *Requests)
			{
				const TSharedPtr<FJsonObject> Item = Value.IsValid() ? Value->AsObject() : nullptr;
				if (!Item.IsValid())
				{
					continue;
				}

				FString RequestId;
				Item->TryGetStringField(TEXT("requestId"), RequestId);
				if (RequestId.IsEmpty() || Self->SeenContractRequestIds.Contains(RequestId))
				{
					continue;
				}

				FString KindText;
				Item->TryGetStringField(TEXT("kind"), KindText);

				FString FromName = TEXT("---");
				const TSharedPtr<FJsonObject>* FromObject = nullptr;
				if (Item->TryGetObjectField(TEXT("from"), FromObject) && FromObject && FromObject->IsValid())
				{
					(*FromObject)->TryGetStringField(TEXT("characterName"), FromName);
				}

				Self->SeenContractRequestIds.Add(RequestId);
				Self->OnContractRequestReceived.Broadcast(RequestId, FromName, KindText == TEXT("formal"));
			}
		});
}

EMyBondageLifeContractRole UMyBondageLifeAccountSubsystem::GetContractRoleWith(const FString& InAccountName) const
{
	for (const FMyBondageLifeFriendEntry& Entry : Friends)
	{
		if (Entry.AccountName == InAccountName)
		{
			return Entry.ContractRole;
		}
	}

	return EMyBondageLifeContractRole::None;
}

bool UMyBondageLifeAccountSubsystem::GetFriendEntry(const FString& InAccountName, FMyBondageLifeFriendEntry& OutEntry) const
{
	for (const FMyBondageLifeFriendEntry& Entry : Friends)
	{
		if (Entry.AccountName == InAccountName)
		{
			OutEntry = Entry;
			return true;
		}
	}
	return false;
}

void UMyBondageLifeAccountSubsystem::MarkAccountOwned(const FString& InAccountName)
{
	if (!InAccountName.IsEmpty() && !KnownOwnedAccounts.Contains(InAccountName))
	{
		KnownOwnedAccounts.Add(InAccountName);
	}
}

bool UMyBondageLifeAccountSubsystem::IsAccountKnownOwned(const FString& InAccountName) const
{
	return KnownOwnedAccounts.Contains(InAccountName);
}

void UMyBondageLifeAccountSubsystem::RequestUserProfile(const FString& InAccountName)
{
	if (InAccountName.IsEmpty())
	{
		return;
	}

	// Small cache: the info panel is reopened often and the data changes rarely.
	// A cache hit stays silent on purpose - broadcasting here would re-enter the
	// panel refresh, which asks again, and so on forever.
	const double Now = FPlatformTime::Seconds();
	if (const double* LastFetch = UserProfileFetchSeconds.Find(InAccountName))
	{
		if (Now - *LastFetch < 10.0 && UserProfiles.Contains(InAccountName))
		{
			return;
		}
	}

	const FString Path = FString::Printf(TEXT("/users/%s"), *InAccountName);
	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);

	SendFriendApiRequest(TEXT("GET"), Path, nullptr,
		[WeakThis, InAccountName](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self || !bOk || !Json.IsValid())
			{
				return;
			}

			FMyBondageLifeUserProfile Profile;
			Json->TryGetStringField(TEXT("account"), Profile.Account);
			Json->TryGetStringField(TEXT("characterName"), Profile.CharacterName);

			double ClassValue = 1.0;
			if (Json->TryGetNumberField(TEXT("characterClass"), ClassValue))
			{
				Profile.CharacterClass = FMath::Clamp(static_cast<int32>(ClassValue), 1, 4);
			}

			Json->TryGetBoolField(TEXT("online"), Profile.bOnline);

			double HairValue = 1.0;
			if (Json->TryGetNumberField(TEXT("hairIndex"), HairValue))
			{
				Profile.HairIndex = FMath::Clamp(static_cast<int32>(HairValue), 1, 64);
			}
			Profile.CreatedAt = ParseIsoField(Json, TEXT("createdAt"));

			const TSharedPtr<FJsonObject>* ContractObject = nullptr;
			if (Json->TryGetObjectField(TEXT("contract"), ContractObject) && ContractObject && ContractObject->IsValid())
			{
				FString RoleText;
				(*ContractObject)->TryGetStringField(TEXT("role"), RoleText);
				Profile.bHasMaster = true;
				Profile.bMasterIsFormal = RoleText == TEXT("slave");
				Profile.ContractSince = ParseIsoField(*ContractObject, TEXT("since"));

				const TSharedPtr<FJsonObject>* MasterObject = nullptr;
				if ((*ContractObject)->TryGetObjectField(TEXT("master"), MasterObject) && MasterObject && MasterObject->IsValid())
				{
					(*MasterObject)->TryGetStringField(TEXT("account"), Profile.MasterAccount);
					(*MasterObject)->TryGetStringField(TEXT("characterName"), Profile.MasterCharacterName);
				}
			}

			Self->UserProfiles.Add(InAccountName, Profile);
			Self->UserProfileFetchSeconds.Add(InAccountName, FPlatformTime::Seconds());
			Self->OnUserProfileUpdated.Broadcast(InAccountName);
		});
}

bool UMyBondageLifeAccountSubsystem::GetUserProfile(const FString& InAccountName, FMyBondageLifeUserProfile& OutProfile) const
{
	if (const FMyBondageLifeUserProfile* Found = UserProfiles.Find(InAccountName))
	{
		OutProfile = *Found;
		return true;
	}
	return false;
}

void UMyBondageLifeAccountSubsystem::RequestUpdateHair(int32 NewHairIndex)
{
	const int32 ClampedIndex = FMath::Clamp(NewHairIndex, 1, 64);

	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetNumberField(TEXT("hairIndex"), ClampedIndex);

	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);

	SendFriendApiRequest(TEXT("PATCH"), TEXT("/users/me"), Body,
		[WeakThis, ClampedIndex](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			if (!bOk)
			{
				UE_LOG(LogMyBondageLife, Warning, TEXT("Hair: update rejected (%d)."), StatusCode);
				return;
			}

			Self->HairIndex = ClampedIndex;
			Self->OnHairChanged.Broadcast(Self->HairIndex);

			UE_LOG(LogMyBondageLife, Log, TEXT("Hair: stored style %d."), Self->HairIndex);
		});
}

// ---------------------------------------------------------------------------
// Restraints (GET/PUT/DELETE /users/me/restraints)
// ---------------------------------------------------------------------------

void UMyBondageLifeAccountSubsystem::RefreshRestraints()
{
	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);

	SendFriendApiRequest(TEXT("GET"), TEXT("/users/me/restraints"), nullptr,
		[WeakThis](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self || !bOk || !Json.IsValid())
			{
				return;
			}

			Self->EquippedRestraints.Reset();

			Self->EquippedRestraints.Reset();
			Self->RestraintLocks.Reset();

			const TArray<TSharedPtr<FJsonValue>>* Equipped = nullptr;
			if (Json->TryGetArrayField(TEXT("equipped"), Equipped) && Equipped)
			{
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
					Self->EquippedRestraints.Add(SlotIndex, ItemId);

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
						Self->RestraintLocks.Add(SlotIndex, Lock);
					}
				}
			}

			UE_LOG(LogMyBondageLife, Log, TEXT("Restraint: %d slot(s) equipped."), Self->EquippedRestraints.Num());
			Self->OnRestraintsChanged.Broadcast();
		});
}

void UMyBondageLifeAccountSubsystem::EquipRestraint(int32 Slot, const FString& ItemId)
{
	if (Slot < 0 || Slot > 11 || ItemId.IsEmpty())
	{
		return;
	}

	// Optimistic local update so the character changes immediately.
	EquippedRestraints.Add(Slot, ItemId);
	OnRestraintsChanged.Broadcast();

	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("itemId"), ItemId);

	const FString Path = FString::Printf(TEXT("/users/me/restraints/%d"), Slot);
	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);

	SendFriendApiRequest(TEXT("PUT"), Path, Body,
		[WeakThis](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			UE_LOG(LogMyBondageLife, Log, TEXT("Restraint: equip %s (%d)."), bOk ? TEXT("ok") : TEXT("failed"), StatusCode);

			// Re-sync so a rejected item does not stay on screen.
			if (!bOk)
			{
				Self->RefreshRestraints();
			}
		});
}

void UMyBondageLifeAccountSubsystem::UnequipRestraint(int32 Slot)
{
	if (Slot < 0 || Slot > 11)
	{
		return;
	}

	EquippedRestraints.Remove(Slot);
	OnRestraintsChanged.Broadcast();

	const FString Path = FString::Printf(TEXT("/users/me/restraints/%d"), Slot);
	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);

	SendFriendApiRequest(TEXT("DELETE"), Path, nullptr,
		[WeakThis](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (Self && !bOk)
			{
				Self->RefreshRestraints();
			}
		});
}

FString UMyBondageLifeAccountSubsystem::GetEquippedRestraint(int32 Slot) const
{
	if (const FString* Found = EquippedRestraints.Find(Slot))
	{
		return *Found;
	}
	return FString();
}

bool UMyBondageLifeAccountSubsystem::HasRestraintLock(int32 Slot) const
{
	return RestraintLocks.Contains(Slot);
}

FMyBondageLifeRestraintLock UMyBondageLifeAccountSubsystem::GetRestraintLock(int32 Slot) const
{
	if (const FMyBondageLifeRestraintLock* Found = RestraintLocks.Find(Slot))
	{
		return *Found;
	}
	return FMyBondageLifeRestraintLock();
}

void UMyBondageLifeAccountSubsystem::LockRestraint(int32 Slot, int32 LockType, int32 DurationSeconds, const FString& Password)
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

	// The lock routes do not resolve "me": use the real account name.
	const FString Owner = AccountName.IsEmpty() ? TEXT("me") : AccountName;
	const FString Path = FString::Printf(TEXT("/users/%s/restraints/%d/lock"), *Owner, Slot);
	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);

	SendFriendApiRequest(TEXT("POST"), Path, Body,
		[WeakThis](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			UE_LOG(LogMyBondageLife, Log, TEXT("Restraint: lock %s (%d)."), bOk ? TEXT("ok") : TEXT("failed"), StatusCode);

			// Always re-read: the server owns the rules (canUnlock, expiry, ...).
			Self->RefreshRestraints();
		});
}

void UMyBondageLifeAccountSubsystem::UnlockRestraint(int32 Slot, const FString& Password)
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

	const FString Owner = AccountName.IsEmpty() ? TEXT("me") : AccountName;
	const FString Path = FString::Printf(TEXT("/users/%s/restraints/%d/lock"), *Owner, Slot);
	TWeakObjectPtr<UMyBondageLifeAccountSubsystem> WeakThis(this);

	SendFriendApiRequest(TEXT("DELETE"), Path, Body,
		[WeakThis](bool bOk, int32 StatusCode, const TSharedPtr<FJsonObject>& Json)
		{
			UMyBondageLifeAccountSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			UE_LOG(LogMyBondageLife, Log, TEXT("Restraint: unlock %s (%d)."), bOk ? TEXT("ok") : TEXT("failed"), StatusCode);
			Self->RefreshRestraints();
		});
}