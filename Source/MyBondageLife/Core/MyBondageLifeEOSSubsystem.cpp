// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeEOSSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "MyBondageLife.h"
#include "SocketSubsystemEOS.h"
#include "SocketSubsystemEOSUtils.h"
#include "eos_sdk.h"

namespace
{
	/** Section name in DefaultEngine.ini: [EOSSDK.Platform.MyBondageLife]. */
	const TCHAR* EOSPlatformConfigName = TEXT("MyBondageLife");

	/** Socket utils handed to the EOS socket subsystem. */
	class FMyBondageLifeSocketUtils final : public ISocketSubsystemEOSUtils
	{
	public:

		FMyBondageLifeSocketUtils(EOS_ProductUserId InPuid, FName InInstanceName)
			: Puid(InPuid)
			, InstanceName(InInstanceName)
		{
		}

		virtual EOS_ProductUserId GetLocalUserId() override { return Puid; }
		virtual FString GetSessionId() override { return FString(); }
		virtual FName GetSubsystemInstanceName() override { return InstanceName; }
		virtual bool IsLoggedIn() override { return Puid != nullptr; }

	private:

		EOS_ProductUserId Puid;
		FName InstanceName;
	};

	void EOS_CALL OnCreateDeviceIdCallback(const EOS_Connect_CreateDeviceIdCallbackInfo* Data)
	{
		if (UMyBondageLifeEOSSubsystem* Self = static_cast<UMyBondageLifeEOSSubsystem*>(Data->ClientData))
		{
			Self->HandleCreateDeviceIdResult(static_cast<int32>(Data->ResultCode));
		}
	}

	void EOS_CALL OnLoginCallback(const EOS_Connect_LoginCallbackInfo* Data)
	{
		if (UMyBondageLifeEOSSubsystem* Self = static_cast<UMyBondageLifeEOSSubsystem*>(Data->ClientData))
		{
			Self->HandleLoginResult(static_cast<int32>(Data->ResultCode));
		}
	}
}

void UMyBondageLifeEOSSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IEOSSDKManager* Manager = IEOSSDKManager::Get();
	if (!Manager)
	{
		Fail(FText::FromString(TEXT("\u0045\u004F\u0053 \u63D2\u4EF6\u672A\u542F\u7528")));
		return;
	}

	PlatformHandle = Manager->CreatePlatform(EOSPlatformConfigName);
	if (!PlatformHandle.IsValid())
	{
		Fail(FText::FromString(TEXT("\u0045\u004F\u0053 \u5E73\u53F0\u521D\u59CB\u5316\u5931\u8D25\uFF0C\u8BF7\u68C0\u67E5 EOSSDK.Platform.MyBondageLife \u914D\u7F6E")));
		return;
	}

	StartDeviceAuth();
}

void UMyBondageLifeEOSSubsystem::Deinitialize()
{
	if (SocketSubsystem)
	{
		SocketSubsystem->Shutdown();
		delete SocketSubsystem;
		SocketSubsystem = nullptr;
	}

	SocketUtils.Reset();
	PlatformHandle.Reset();

	Super::Deinitialize();
}

void UMyBondageLifeEOSSubsystem::StartDeviceAuth()
{
	if (!PlatformHandle.IsValid() || bLoginInFlight)
	{
		return;
	}

	EOS_HConnect Connect = EOS_Platform_GetConnectInterface(*PlatformHandle);
	if (!Connect)
	{
		Fail(FText::FromString(TEXT("\u0045\u004F\u0053 Connect \u63A5\u53E3\u4E0D\u53EF\u7528")));
		return;
	}

	EOS_Connect_CreateDeviceIdOptions Options = {};
	Options.ApiVersion = EOS_CONNECT_CREATEDEVICEID_API_LATEST;
	Options.DeviceModel = "PC";

	bLoginInFlight = true;
	EOS_Connect_CreateDeviceId(Connect, &Options, this, &OnCreateDeviceIdCallback);
}

void UMyBondageLifeEOSSubsystem::HandleCreateDeviceIdResult(int32 ResultCode)
{
	if (ResultCode == static_cast<int32>(EOS_EResult::EOS_Success) ||
		ResultCode == static_cast<int32>(EOS_EResult::EOS_DuplicateNotAllowed))
	{
		// EOS_DuplicateNotAllowed means this machine already has a device ID; reuse it.
		LoginWithDeviceId();
		return;
	}

	bLoginInFlight = false;
	Fail(FText::FromString(FString::Printf(
		TEXT("\u0045\u004F\u0053 \u8BBE\u5907\u8EAB\u4EFD\u521B\u5EFA\u5931\u8D25\uFF08%d\uFF09"), ResultCode)));
}

void UMyBondageLifeEOSSubsystem::LoginWithDeviceId()
{
	if (!PlatformHandle.IsValid())
	{
		return;
	}

	EOS_HConnect Connect = EOS_Platform_GetConnectInterface(*PlatformHandle);
	if (!Connect)
	{
		bLoginInFlight = false;
		Fail(FText::FromString(TEXT("\u0045\u004F\u0053 Connect \u63A5\u53E3\u4E0D\u53EF\u7528")));
		return;
	}

	EOS_Connect_UserLoginInfo LoginInfo = {};
	LoginInfo.ApiVersion = EOS_CONNECT_USERLOGININFO_API_LATEST;
	LoginInfo.DisplayName = "Player";

	EOS_Connect_Credentials Credentials = {};
	Credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
	Credentials.Token = "";
	Credentials.Type = EOS_EExternalCredentialType::EOS_ECT_DEVICEID_ACCESS_TOKEN;

	EOS_Connect_LoginOptions Options = {};
	Options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
	Options.Credentials = &Credentials;
	Options.UserLoginInfo = &LoginInfo;

	EOS_Connect_Login(Connect, &Options, this, &OnLoginCallback);
}

void UMyBondageLifeEOSSubsystem::HandleLoginResult(int32 ResultCode)
{
	bLoginInFlight = false;

	if (ResultCode != static_cast<int32>(EOS_EResult::EOS_Success))
	{
		Fail(FText::FromString(FString::Printf(
			TEXT("\u0045\u004F\u0053 \u767B\u5F55\u5931\u8D25\uFF08%d\uFF09"), ResultCode)));
		return;
	}

	CompleteLogin();
}

void UMyBondageLifeEOSSubsystem::CompleteLogin()
{
	bLoggedIn = true;

	EOS_HConnect Connect = EOS_Platform_GetConnectInterface(*PlatformHandle);
	if (Connect)
	{
		const int32 Count = EOS_Connect_GetLoggedInUsersCount(Connect);
		if (Count > 0)
		{
			const EOS_ProductUserId Puid = EOS_Connect_GetLoggedInUserByIndex(Connect, 0);
			char Buffer[EOS_PRODUCTUSERID_MAX_LENGTH + 1] = { 0 };
			int32 Length = static_cast<int32>(sizeof(Buffer));
			if (EOS_ProductUserId_ToString(Puid, Buffer, &Length) == EOS_EResult::EOS_Success)
			{
				ProductUserId = UTF8_TO_TCHAR(Buffer);
			}
		}
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("EOS: 登录成功，PUID=%s"), *ProductUserId);

	SetupSocketSubsystem();
	OnEOSReady.Broadcast();
}

void UMyBondageLifeEOSSubsystem::SetupSocketSubsystem()
{
	if (SocketSubsystem || !PlatformHandle.IsValid())
	{
		return;
	}

	// In PIE, GetWorldForOnline needs the world context handle to find the right
	// world; in a packaged game NAME_None resolves to the game world.
	FName InstanceName = NAME_None;
	if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		if (GEngine)
		{
			if (const FWorldContext* Context = GEngine->GetWorldContextFromWorld(World))
			{
				InstanceName = Context->ContextHandle;
			}
		}
	}

	const EOS_ProductUserId Puid = EOS_ProductUserId_FromString(TCHAR_TO_UTF8(*ProductUserId));
	SocketUtils = MakeShared<FMyBondageLifeSocketUtils, ESPMode::ThreadSafe>(Puid, InstanceName);
	SocketSubsystem = new FSocketSubsystemEOS(PlatformHandle, SocketUtils);

	FString Error;
	if (!SocketSubsystem->Init(Error))
	{
		UE_LOG(LogMyBondageLife, Error, TEXT("EOS: P2P socket subsystem init failed: %s"), *Error);
		SocketSubsystem->Shutdown();
		delete SocketSubsystem;
		SocketSubsystem = nullptr;
		SocketUtils.Reset();
		return;
	}

	UE_LOG(LogMyBondageLife, Log, TEXT("EOS: P2P socket subsystem ready (world context '%s')."), *InstanceName.ToString());
}

void UMyBondageLifeEOSSubsystem::Fail(const FText& Message)
{
	UE_LOG(LogMyBondageLife, Error, TEXT("EOS: %s"), *Message.ToString());
	OnEOSFailed.Broadcast(Message);
}
