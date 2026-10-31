// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Garbler/MyBondageLifeGarblerTypes.h"

class FJsonObject;

/** 某个音素在某个堵嘴档位下被替换成什么。 */
struct FMyBondageLifeGagRule
{
	/** 0 = 不影响 1 = 轻度 2 = 重度 3 = 完全抑制。多个堵嘴同时佩戴时，取该值最大的那一档。 */
	int32 Muffle = 0;

	/** 替换后的发音（空字符串 = 这个音被吞掉）。 */
	FString Sound;
};

/** 一个堵嘴档位（开口环 / 口球 / 面板口枷 ...）。 */
struct FMyBondageLifeGagProfile
{
	FName Id;
	FString DisplayName;
	FString Description;
	TArray<FString> Aliases;
	TArray<FString> FallbackSounds;

	/** 完全无声（只剩 …… / mnm 这种残响）。 */
	bool bSilent = false;

	/** 中文拼音音素（声母 + 韵母）。 */
	TMap<FString, FMyBondageLifeGagRule> Zh;

	/** 英文 IPA 音素。 */
	TMap<FString, FMyBondageLifeGagRule> En;
};

/**
 *  堵嘴文本混淆器（Garbler）的纯逻辑核心。
 *
 *  流程： 原文 -> 分词 -> 汉字查拼音 / 英文按拼写规则转 IPA -> 拆成音素
 *        -> 每个音素按当前佩戴的堵嘴替换发音 -> 拼回原字符串（保留空格与标点）
 *
 *  数据全部放在 <项目>/Source/MyBondageLife/Core/Garbler/Data/*.json：
 *    GarblerTables.json  音素表（每种堵嘴 × 每个音素 -> muffle + 替换发音）
 *    HanziPinyin.json    汉字 -> 无声调拼音
 *  也可以在 <项目>/Config/Garbler/ 放同名文件覆盖（改完调 Reload() 即可，不用重编译）。
 *
 *  这个类不依赖任何 UObject，可以在任意线程的普通 C++ 里直接调用；
 *  蓝图请用 UMyBondageLifeGarblerSubsystem / UMyBondageLifeGarblerLibrary。
 */
class MYBONDAGELIFE_API FMyBondageLifeGarbler
{
public:

	/** 首次调用时自动加载；已加载则直接返回 true。 */
	static bool EnsureLoaded();

	/** 强制重新读取 JSON（改表后调用，无需重启）。 */
	static bool Reload();

	static bool IsLoaded();

	/** 当前实际使用的数据目录（排查"读的是哪份表"时看这个）。 */
	static FString GetDataDirectory();

	/** 所有已加载的堵嘴档位 id，顺序与表内一致。 */
	static const TArray<FName>& GetProfileIds();

	static bool IsSilentProfile(FName ProfileId);

	static FString GetProfileDisplayName(FName ProfileId);

	/** 把 "口球" / "ball gag" / "BallGag" 这类写法统一解析成档位 id；失败返回 NAME_None。 */
	static FName ResolveProfileId(const FString& IdOrDisplayNameOrAlias);

	/** 调试用：一句话描述某个档位（音素条目数等）。 */
	static FString DescribeProfile(FName ProfileId);

	/**
	 *  把一句话混淆成堵嘴后的发音。
	 *
	 *  @param Text     原文（中英混排均可）
	 *  @param GagIds   当前佩戴的堵嘴档位 id；空数组 = 不做任何混淆，原样返回
	 *  @param Options  可调参数
	 */
	static FString Garble(const FString& Text, const TArray<FName>& GagIds, const FMyBondageLifeGarblerOptions& Options);

	/** 调试用：显示中间结果，例如 "你好" -> "你[n-i] 好[h-ao]"。 */
	static FString ToPhonemes(const FString& Text);

private:

	// ---- 加载 ---------------------------------------------------------
	static bool LoadTables(const FString& Json);
	static bool LoadHanzi(const FString& Json);
	static bool ReadDataFile(const FString& FileName, FString& OutJson, FString& OutPath);
	static void Reset();

	// ---- 分词/音素 ----------------------------------------------------
	static bool IsCjk(TCHAR Ch);
	static bool IsWordChar(TCHAR Ch);
	static bool IsAsciiDigit(TCHAR Ch);

	/** 汉字 -> 拼音音节；查不到返回 nullptr。 */
	static const FString* FindPinyin(TCHAR Hanzi);

	/** 拼音音节 -> 声母/韵母（"zhuang" -> "zh" + "uang"）。 */
	static void SplitPinyin(const FString& Syllable, FString& OutInitial, FString& OutFinal);

	/** 拼音音节 -> 音素序列（声母、韵母各算一个）。 */
	static void SyllableToPhonemes(const FString& Syllable, TArray<FString>& OutPhonemes);

	/** 英文拼写 -> IPA 音素序列。 */
	static void LatinToPhonemes(const FString& Word, TArray<FString>& OutPhonemes);

	/** 音素序列 -> 混淆后的字符串。 */
	static FString GarblePhonemes(const TArray<FString>& Phonemes, const TArray<const FMyBondageLifeGagProfile*>& Active,
		bool bZh, FRandomStream& Rng);

	static FString ApplyCase(const FString& Source, const FString& Garbled);

	/** 完全无声档位的残响：偶尔回一个 "m." / "n.."，多数时候什么都不留。 */
	static FString SilentResidue(FRandomStream& Rng);

	static FString ResolveDataFile(const FString& FileName);

	// ---- 数据 ---------------------------------------------------------
	static bool bLoaded;
	static FString DataDirectory;

	static TArray<FMyBondageLifeGagProfile> Profiles;
	static TArray<FName> ProfileIds;
	static TArray<FString> ZhInitials;			// 按长度倒序，便于最长匹配
	static TSet<FString> ZhFinals;
	static TMap<FString, FName> AliasToId;
	static TMap<FString, int32> ProfileIndex;

	/** 英文拼写规则，按 pattern 长度倒序。 */
	struct FLatinRule
	{
		FString Grapheme;
		TArray<FString> Phonemes;
	};
	static TArray<FLatinRule> LatinRules;

	static TMap<TCHAR, FString> HanziPinyin;
	static TMap<TCHAR, FString> DigitZh;		// "1" -> "yi"
};
