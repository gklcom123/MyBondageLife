// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 *  Account service endpoint, read from Config/ServerConfig.ini.
 *
 *  The address lives in a data file rather than in code so moving from the local
 *  development service (127.0.0.1:3000) to a deployed server is a config edit, not
 *  a rebuild.
 */
struct FMyBondageLifeServerConfig
{
	/** Absolute path of the config file. */
	static FString GetConfigFilePath();

	/** Base URL such as "http://127.0.0.1:3000". */
	static FString GetBaseUrl();

	static FString GetHost();

	static int32 GetPort();

	/** True when the config asks for https. */
	static bool ShouldUseHttps();

private:

	/** Reads the file, falling back to the local defaults when it is missing. */
	static void ReadConfig(FString& OutHost, int32& OutPort, bool& bOutUseHttps);
};
