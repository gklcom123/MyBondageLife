// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/Garbler/MyBondageLifeGarblerSubsystem.h"

#include "Core/Garbler/MyBondageLifeGarbler.h"
#include "HAL/IConsoleManager.h"
#include "MyBondageLife.h"

namespace
{
	/** 把 "BallGag,口球" 这样的列表解析成档位 id；解析不了的会被忽略。 */
	void ResolveGagList(const TArray<FName>& Input, TArray<FName>& Out)
	{
		Out.Reset();
		for (const FName& Entry : Input)
		{
			const FString Raw = Entry.ToString().TrimStartAndEnd();
			if (Raw.IsEmpty())
			{
				continue;
			}

			FName Resolved = FMyBondageLifeGarbler::ResolveProfileId(Raw);
			if (Resolved.IsNone())
			{
				// 允许直接传 id（大小写不完全一致时也认）
				for (const FName& Id : FMyBondageLifeGarbler::GetProfileIds())
				{
					if (Id.ToString().Equals(Raw, ESearchCase::IgnoreCase))
					{
						Resolved = Id;
						break;
					}
				}
			}

			if (!Resolved.IsNone() && !Out.Contains(Resolved))
			{
				Out.Add(Resolved);
			}
		}
	}

	void LogUsage()
	{
		UE_LOG(LogMyBondageLife, Display, TEXT("[Garbler] 用法:"));
		UE_LOG(LogMyBondageLife, Display, TEXT("  mbl.garbler.test <档位,逗号分隔> <文本>   例: mbl.garbler.test BallGag+CleaveGag 你好世界"));
		UE_LOG(LogMyBondageLife, Display, TEXT("  mbl.garbler.tables                       列出所有档位与数据目录"));
		UE_LOG(LogMyBondageLife, Display, TEXT("  mbl.garbler.reload                       重新读取 json"));
	}

	void GarblerTestCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			LogUsage();
			return;
		}

		TArray<FName> Requested;
		TArray<FString> Parts;
		// 控制台里逗号会被 ExecCmds 当成分隔符，所以这里同时接受 "+"。
		FString Spec = Args[0];
		Spec.ReplaceInline(TEXT("+"), TEXT(","));
		Spec.ParseIntoArray(Parts, TEXT(","), /*InCullEmpty*/ true);
		for (const FString& Part : Parts)
		{
			Requested.Add(FName(*Part.TrimStartAndEnd()));
		}

		TArray<FName> Gags;
		ResolveGagList(Requested, Gags);

		TArray<FString> Words;
		for (int32 Index = 1; Index < Args.Num(); ++Index)
		{
			Words.Add(Args[Index]);
		}
		FString Text;
		for (int32 Index = 0; Index < Words.Num(); ++Index)
		{
			if (Index > 0)
			{
				Text += TEXT(" ");
			}
			Text += Words[Index];
		}

		if (Gags.Num() == 0)
		{
			UE_LOG(LogMyBondageLife, Warning, TEXT("[Garbler] 没有识别出堵嘴档位: %s"), *Args[0]);
			return;
		}

		FMyBondageLifeGarblerOptions Options;
		Options.RandomSeed = 12345;	// 固定种子，方便反复对比

		FString GagLabel;
		for (int32 Index = 0; Index < Gags.Num(); ++Index)
		{
			GagLabel += (Index > 0 ? TEXT(", ") : TEXT("")) + Gags[Index].ToString();
		}
		UE_LOG(LogMyBondageLife, Display, TEXT("[Garbler] 档位: %s"), *GagLabel);
		UE_LOG(LogMyBondageLife, Display, TEXT("[Garbler] 原文: %s"), *Text);
		UE_LOG(LogMyBondageLife, Display, TEXT("[Garbler] 音素: %s"), *FMyBondageLifeGarbler::ToPhonemes(Text));
		UE_LOG(LogMyBondageLife, Display, TEXT("[Garbler] 混淆: %s"), *FMyBondageLifeGarbler::Garble(Text, Gags, Options));
	}

	void GarblerTablesCommand(const TArray<FString>& /*Args*/)
	{
		FMyBondageLifeGarbler::EnsureLoaded();
		UE_LOG(LogMyBondageLife, Display, TEXT("[Garbler] 数据目录: %s"), *FMyBondageLifeGarbler::GetDataDirectory());

		for (const FName& Id : FMyBondageLifeGarbler::GetProfileIds())
		{
			UE_LOG(LogMyBondageLife, Display, TEXT("[Garbler]   %s"), *FMyBondageLifeGarbler::DescribeProfile(Id));
		}
	}

	void GarblerReloadCommand(const TArray<FString>& /*Args*/)
	{
		const bool bOk = FMyBondageLifeGarbler::Reload();
		UE_LOG(LogMyBondageLife, Display, TEXT("[Garbler] 重新加载%s"), bOk ? TEXT("成功") : TEXT("失败"));
	}

	FAutoConsoleCommand GGarblerTestCmd(
		TEXT("mbl.garbler.test"),
		TEXT("把一句话按指定堵嘴混淆并打印中间音素。用法: mbl.garbler.test BallGag,CleaveGag 你好世界"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&GarblerTestCommand));

	FAutoConsoleCommand GGarblerTablesCmd(
		TEXT("mbl.garbler.tables"),
		TEXT("列出所有堵嘴档位与当前使用的数据目录。"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&GarblerTablesCommand));

	FAutoConsoleCommand GGarblerReloadCmd(
		TEXT("mbl.garbler.reload"),
		TEXT("重新读取 GarblerTables.json / HanziPinyin.json。"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&GarblerReloadCommand));
}

// ---------------------------------------------------------------------------

void UMyBondageLifeGarblerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 提前加载，确保数据目录/加载结果在启动日志里就能看到。
	FMyBondageLifeGarbler::EnsureLoaded();
}

void UMyBondageLifeGarblerSubsystem::Deinitialize()
{
	ActiveGags.Reset();
	Super::Deinitialize();
}

FString UMyBondageLifeGarblerSubsystem::GarbleText(const FString& Text) const
{
	return FMyBondageLifeGarbler::Garble(Text, ActiveGags, DefaultOptions);
}

FString UMyBondageLifeGarblerSubsystem::GarbleTextWith(const FString& Text, const TArray<FName>& GagProfiles,
	const FMyBondageLifeGarblerOptions& Options) const
{
	TArray<FName> Resolved;
	ResolveGagList(GagProfiles, Resolved);
	return FMyBondageLifeGarbler::Garble(Text, Resolved, Options);
}

FString UMyBondageLifeGarblerSubsystem::ToPhonemes(const FString& Text) const
{
	return FMyBondageLifeGarbler::ToPhonemes(Text);
}

void UMyBondageLifeGarblerSubsystem::SetActiveGags(const TArray<FName>& GagProfiles)
{
	TArray<FName> Resolved;
	ResolveGagList(GagProfiles, Resolved);

	if (Resolved == ActiveGags)
	{
		return;
	}
	ActiveGags = MoveTemp(Resolved);
	OnActiveGagsChanged.Broadcast();
}

void UMyBondageLifeGarblerSubsystem::SetGagActive(FName GagProfile, bool bActive)
{
	TArray<FName> Resolved;
	ResolveGagList({ GagProfile }, Resolved);

	const FName Id = Resolved.Num() > 0 ? Resolved[0] : NAME_None;
	if (Id.IsNone())
	{
		UE_LOG(LogMyBondageLife, Warning, TEXT("[Garbler] 未知堵嘴: %s"), *GagProfile.ToString());
		return;
	}

	const bool bHas = ActiveGags.Contains(Id);
	if (bActive == bHas)
	{
		return;
	}

	if (bActive)
	{
		ActiveGags.Add(Id);
	}
	else
	{
		ActiveGags.Remove(Id);
	}
	OnActiveGagsChanged.Broadcast();
}

void UMyBondageLifeGarblerSubsystem::ClearActiveGags()
{
	if (ActiveGags.Num() == 0)
	{
		return;
	}
	ActiveGags.Reset();
	OnActiveGagsChanged.Broadcast();
}

TArray<FName> UMyBondageLifeGarblerSubsystem::GetAvailableGags() const
{
	return FMyBondageLifeGarbler::GetProfileIds();
}

FString UMyBondageLifeGarblerSubsystem::GetGagDisplayName(FName GagProfile) const
{
	return FMyBondageLifeGarbler::GetProfileDisplayName(GagProfile);
}

bool UMyBondageLifeGarblerSubsystem::IsSilentGag(FName GagProfile) const
{
	return FMyBondageLifeGarbler::IsSilentProfile(GagProfile);
}

bool UMyBondageLifeGarblerSubsystem::ReloadTables()
{
	return FMyBondageLifeGarbler::Reload();
}

FString UMyBondageLifeGarblerSubsystem::GetDataDirectory() const
{
	return FMyBondageLifeGarbler::GetDataDirectory();
}
