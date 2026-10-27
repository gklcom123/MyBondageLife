// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyBondageLifeServerConfig.h"

#include "HAL/FileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "MyBondageLife.h"

namespace
{
	/** File name inside the project's Config directory. */
	const TCHAR* ConfigFileName = TEXT("ServerConfig.ini");

	/** Section that holds the endpoint. */
	const TCHAR* ConfigSection = TEXT("Server");

	/** Used when the file (or a key) is missing. */
	const TCHAR* DefaultHost = TEXT("127.0.0.1");
	constexpr int32 DefaultPort = 3000;
}

FString FMyBondageLifeServerConfig::GetConfigFilePath()
{
	return FPaths::Combine(FPaths::ProjectConfigDir(), ConfigFileName);
}

void FMyBondageLifeServerConfig::ReadConfig(FString& OutHost, int32& OutPort, bool& bOutUseHttps)
{
	OutHost = DefaultHost;
	OutPort = DefaultPort;
	bOutUseHttps = false;

	const FString Path = GetConfigFilePath();
	if (!IFileManager::Get().FileExists(*Path))
	{
		UE_LOG(LogMyBondageLife, Warning,
			TEXT("ServerConfig: %s 不存在，使用默认地址 %s:%d"), *Path, DefaultHost, DefaultPort);
		return;
	}

	FConfigFile Config;
	Config.Read(Path);

	FString Host;
	if (Config.GetString(ConfigSection, TEXT("Host"), Host))
	{
		Host = Host.TrimStartAndEnd();
		if (!Host.IsEmpty())
		{
			OutHost = Host;
		}
	}

	int32 Port = DefaultPort;
	if (Config.GetInt(ConfigSection, TEXT("Port"), Port) && Port > 0 && Port <= 65535)
	{
		OutPort = Port;
	}

	bool bUseHttps = false;
	if (Config.GetBool(ConfigSection, TEXT("bUseHttps"), bUseHttps))
	{
		bOutUseHttps = bUseHttps;
	}
}

FString FMyBondageLifeServerConfig::GetBaseUrl()
{
	FString Host;
	int32 Port = DefaultPort;
	bool bUseHttps = false;
	ReadConfig(Host, Port, bUseHttps);

	// A full URL in Host wins, so "https://api.example.com" works without a port.
	if (Host.StartsWith(TEXT("http://")) || Host.StartsWith(TEXT("https://")))
	{
		return Host.EndsWith(TEXT("/")) ? Host.LeftChop(1) : Host;
	}

	return FString::Printf(TEXT("%s://%s:%d"), bUseHttps ? TEXT("https") : TEXT("http"), *Host, Port);
}

FString FMyBondageLifeServerConfig::GetHost()
{
	FString Host;
	int32 Port = DefaultPort;
	bool bUseHttps = false;
	ReadConfig(Host, Port, bUseHttps);
	return Host;
}

int32 FMyBondageLifeServerConfig::GetPort()
{
	FString Host;
	int32 Port = DefaultPort;
	bool bUseHttps = false;
	ReadConfig(Host, Port, bUseHttps);
	return Port;
}

bool FMyBondageLifeServerConfig::ShouldUseHttps()
{
	FString Host;
	int32 Port = DefaultPort;
	bool bUseHttps = false;
	ReadConfig(Host, Port, bUseHttps);
	return bUseHttps;
}
