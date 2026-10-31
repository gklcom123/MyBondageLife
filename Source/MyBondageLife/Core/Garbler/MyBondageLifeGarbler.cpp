// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/Garbler/MyBondageLifeGarbler.h"

#include "Core/Garbler/MyBondageLifeGarblerEmbedded.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/CriticalSection.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogGarbler, Log, All);

// ---------------------------------------------------------------------------
//  静态数据
// ---------------------------------------------------------------------------

bool FMyBondageLifeGarbler::bLoaded = false;
FString FMyBondageLifeGarbler::DataDirectory;
TArray<FMyBondageLifeGagProfile> FMyBondageLifeGarbler::Profiles;
TArray<FName> FMyBondageLifeGarbler::ProfileIds;
TArray<FString> FMyBondageLifeGarbler::ZhInitials;
TSet<FString> FMyBondageLifeGarbler::ZhFinals;
TMap<FString, FName> FMyBondageLifeGarbler::AliasToId;
TMap<FString, int32> FMyBondageLifeGarbler::ProfileIndex;
TArray<FMyBondageLifeGarbler::FLatinRule> FMyBondageLifeGarbler::LatinRules;
TMap<TCHAR, FString> FMyBondageLifeGarbler::HanziPinyin;
TMap<TCHAR, FString> FMyBondageLifeGarbler::DigitZh;

namespace
{
	/** 配置文件优先，其次源码目录（编辑器里改完就能生效），最后内容目录（打包用）。 */
	const TCHAR* const GarblerTablesFile = TEXT("GarblerTables.json");
	const TCHAR* const GarblerHanziFile = TEXT("HanziPinyin.json");

	FCriticalSection& GarblerLock()
	{
		static FCriticalSection Lock;
		return Lock;
	}

	FString JoinWith(const TArray<FString>& Parts, const TCHAR* Separator)
	{
		FString Result;
		for (int32 Index = 0; Index < Parts.Num(); ++Index)
		{
			if (Index > 0)
			{
				Result += Separator;
			}
			Result += Parts[Index];
		}
		return Result;
	}
}

// ---------------------------------------------------------------------------
//  加载
// ---------------------------------------------------------------------------

FString FMyBondageLifeGarbler::ResolveDataFile(const FString& FileName)
{
	TArray<FString> Candidates;
	Candidates.Add(FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("Garbler"), FileName));
	Candidates.Add(FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/MyBondageLife/Core/Garbler/Data"), FileName));
	Candidates.Add(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("GarblerData"), FileName));

	for (const FString& Candidate : Candidates)
	{
		if (FPaths::FileExists(Candidate))
		{
			return FPaths::ConvertRelativePathToFull(Candidate);
		}
	}
	return FString();
}

bool FMyBondageLifeGarbler::ReadDataFile(const FString& FileName, FString& OutJson, FString& OutPath)
{
	const FString Resolved = ResolveDataFile(FileName);
	if (Resolved.IsEmpty())
	{
		OutPath.Reset();
		return false;
	}
	if (!FFileHelper::LoadFileToString(OutJson, *Resolved))
	{
		UE_LOG(LogGarbler, Warning, TEXT("读取失败: %s"), *Resolved);
		OutPath.Reset();
		return false;
	}
	OutPath = Resolved;
	return true;
}

void FMyBondageLifeGarbler::Reset()
{
	Profiles.Reset();
	ProfileIds.Reset();
	ZhInitials.Reset();
	ZhFinals.Reset();
	AliasToId.Reset();
	ProfileIndex.Reset();
	LatinRules.Reset();
	HanziPinyin.Reset();
	DigitZh.Reset();
	bLoaded = false;
	DataDirectory.Reset();
}

bool FMyBondageLifeGarbler::EnsureLoaded()
{
	if (bLoaded)
	{
		return true;
	}
	FScopeLock Lock(&GarblerLock());
	if (bLoaded)
	{
		return true;
	}
	return Reload();
}

bool FMyBondageLifeGarbler::Reload()
{
	FScopeLock Lock(&GarblerLock());

	Reset();

	FString TablesJson;
	FString TablesPath;
	bool bHaveTables = ReadDataFile(GarblerTablesFile, TablesJson, TablesPath);

	FString HanziJson;
	FString HanziPath;
	bool bHaveHanzi = ReadDataFile(GarblerHanziFile, HanziJson, HanziPath);

	if (!bHaveTables)
	{
		UE_LOG(LogGarbler, Log, TEXT("未找到 %s，改用内置兜底音素表。"), GarblerTablesFile);
		TablesJson = FMyBondageLifeGarblerEmbedded::GetTablesJson();
		TablesPath = TEXT("(内置)");
	}
	if (!bHaveHanzi)
	{
		UE_LOG(LogGarbler, Log, TEXT("未找到 %s，改用内置兜底拼音表。"), GarblerHanziFile);
		HanziJson = FMyBondageLifeGarblerEmbedded::GetHanziJson();
		HanziPath = TEXT("(内置)");
	}

	const bool bTablesOk = LoadTables(TablesJson);
	const bool bHanziOk = LoadHanzi(HanziJson);

	DataDirectory = bHaveTables ? FPaths::GetPath(TablesPath) : FString();
	bLoaded = bTablesOk && bHanziOk;

	UE_LOG(LogGarbler, Log, TEXT("音素表: %s | 拼音表: %s | 档位 %d 个 | 拼音 %d 字"),
		*TablesPath, *HanziPath, ProfileIds.Num(), HanziPinyin.Num());
	return bLoaded;
}

bool FMyBondageLifeGarbler::IsLoaded()
{
	return bLoaded;
}

FString FMyBondageLifeGarbler::GetDataDirectory()
{
	return DataDirectory.IsEmpty()
		? FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/MyBondageLife/Core/Garbler/Data"))
		: DataDirectory;
}

bool FMyBondageLifeGarbler::LoadTables(const FString& Json)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogGarbler, Error, TEXT("音素表 JSON 解析失败。"));
		return false;
	}

	// ---- 音素清单 ----
	ZhFinals.Reset();
	if (const TArray<TSharedPtr<FJsonValue>>* Initials = nullptr; Root->TryGetArrayField(TEXT("zhInitials"), Initials))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Initials)
		{
			ZhInitials.Add(Value->AsString());
		}
		// 最长匹配优先（"zh" 必须排在 "z" 前面）
		ZhInitials.Sort([](const FString& A, const FString& B) { return A.Len() > B.Len(); });
	}
	if (const TArray<TSharedPtr<FJsonValue>>* Finals = nullptr; Root->TryGetArrayField(TEXT("zhFinals"), Finals))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Finals)
		{
			ZhFinals.Add(Value->AsString());
		}
	}
	if (ZhInitials.Num() == 0 || ZhFinals.Num() == 0)
	{
		UE_LOG(LogGarbler, Error, TEXT("音素表缺少 zhInitials / zhFinals。"));
		return false;
	}

	// ---- 数字读法 ----
	DigitZh.Reset();
	if (const TSharedPtr<FJsonObject>* Digits = nullptr; Root->TryGetObjectField(TEXT("digitsZh"), Digits))
	{
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Digits)->Values)
		{
			if (Pair.Key.Len() == 1)
			{
				DigitZh.Add(Pair.Key[0], Pair.Value->AsString());
			}
		}
	}

	// ---- 英文拼写规则 ----
	LatinRules.Reset();
	if (const TArray<TSharedPtr<FJsonValue>>* Graphemes = nullptr; Root->TryGetArrayField(TEXT("graphemesEn"), Graphemes))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Graphemes)
		{
			const TSharedPtr<FJsonObject> Rule = Value->AsObject();
			if (!Rule.IsValid())
			{
				continue;
			}
			FLatinRule Entry;
			Rule->TryGetStringField(TEXT("g"), Entry.Grapheme);
			if (const TArray<TSharedPtr<FJsonValue>>* Sounds = nullptr; Rule->TryGetArrayField(TEXT("p"), Sounds))
			{
				for (const TSharedPtr<FJsonValue>& Sound : *Sounds)
				{
					Entry.Phonemes.Add(Sound->AsString());
				}
			}
			if (!Entry.Grapheme.IsEmpty() && Entry.Phonemes.Num() > 0)
			{
				LatinRules.Add(MoveTemp(Entry));
			}
		}
		LatinRules.Sort([](const FLatinRule& A, const FLatinRule& B) { return A.Grapheme.Len() > B.Grapheme.Len(); });
	}

	// ---- 堵嘴档位 ----
	Profiles.Reset();
	ProfileIds.Reset();
	AliasToId.Reset();
	ProfileIndex.Reset();

	const TArray<TSharedPtr<FJsonValue>>* ProfileArray = nullptr;
	if (!Root->TryGetArrayField(TEXT("profiles"), ProfileArray) || ProfileArray == nullptr)
	{
		UE_LOG(LogGarbler, Error, TEXT("音素表缺少 profiles。"));
		return false;
	}

	for (const TSharedPtr<FJsonValue>& Value : *ProfileArray)
	{
		const TSharedPtr<FJsonObject> Obj = Value->AsObject();
		if (!Obj.IsValid())
		{
			continue;
		}

		FMyBondageLifeGagProfile Profile;
		FString Id;
		if (!Obj->TryGetStringField(TEXT("id"), Id) || Id.IsEmpty())
		{
			continue;
		}
		Profile.Id = FName(*Id);
		Obj->TryGetStringField(TEXT("displayName"), Profile.DisplayName);
		Obj->TryGetStringField(TEXT("description"), Profile.Description);
		Obj->TryGetBoolField(TEXT("silent"), Profile.bSilent);
		Obj->TryGetBoolField(TEXT("noResidue"), Profile.bNoResidue);
		FString BaseId;
		if (Obj->TryGetStringField(TEXT("base"), BaseId) && !BaseId.IsEmpty())
		{
			Profile.Base = FName(*BaseId);
		}

		if (const TArray<TSharedPtr<FJsonValue>>* Aliases = nullptr; Obj->TryGetArrayField(TEXT("aliases"), Aliases))
		{
			for (const TSharedPtr<FJsonValue>& Alias : *Aliases)
			{
				Profile.Aliases.Add(Alias->AsString());
			}
		}
		if (const TArray<TSharedPtr<FJsonValue>>* Sounds = nullptr; Obj->TryGetArrayField(TEXT("fallbackSounds"), Sounds))
		{
			for (const TSharedPtr<FJsonValue>& Sound : *Sounds)
			{
				Profile.FallbackSounds.Add(Sound->AsString());
			}
		}
		if (Profile.FallbackSounds.Num() == 0)
		{
			Profile.FallbackSounds.Add(FString());
		}

		auto ReadRules = [](const TSharedPtr<FJsonObject>& Source, TMap<FString, FMyBondageLifeGagRule>& Out)
		{
			for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Source->Values)
			{
				const TSharedPtr<FJsonObject> RuleObj = Pair.Value->AsObject();
				if (!RuleObj.IsValid())
				{
					continue;
				}
				FMyBondageLifeGagRule Rule;
				RuleObj->TryGetNumberField(TEXT("muffle"), Rule.Muffle);
				RuleObj->TryGetStringField(TEXT("sound"), Rule.Sound);
				Out.Add(Pair.Key, Rule);
			}
		};

		const TSharedPtr<FJsonObject>* ZhObj = nullptr;
		if (Obj->TryGetObjectField(TEXT("zh"), ZhObj) && ZhObj && ZhObj->IsValid())
		{
			ReadRules(*ZhObj, Profile.Zh);
		}
		const TSharedPtr<FJsonObject>* EnObj = nullptr;
		if (Obj->TryGetObjectField(TEXT("en"), EnObj) && EnObj && EnObj->IsValid())
		{
			ReadRules(*EnObj, Profile.En);
		}

		const int32 Index = Profiles.Num();
		ProfileIndex.Add(Profile.Id.ToString(), Index);
		ProfileIds.Add(Profile.Id);

		AliasToId.FindOrAdd(Profile.Id.ToString().ToLower()) = Profile.Id;
		if (!Profile.DisplayName.IsEmpty())
		{
			AliasToId.FindOrAdd(Profile.DisplayName.ToLower()) = Profile.Id;
		}
		for (const FString& Alias : Profile.Aliases)
		{
			const FString Key = Alias.TrimStartAndEnd().ToLower();
			if (!Key.IsEmpty())
			{
				AliasToId.FindOrAdd(Key) = Profile.Id;
			}
		}

		Profiles.Add(MoveTemp(Profile));
	}

	if (Profiles.Num() == 0)
	{
		UE_LOG(LogGarbler, Error, TEXT("音素表里没有任何档位。"));
		return false;
	}

	// ---- 解析档位继承 ----
	// 变体档位（如 BallGagXL）只写与基准档位不同的音素，其余在这里补齐。
	// 多跑几轮以支持 base 定义在变体之后、以及多层继承。
	TSet<FName> Merged;
	for (int32 Pass = 0; Pass < 8; ++Pass)
	{
		bool bProgress = false;
		for (int32 Index = 0; Index < Profiles.Num(); ++Index)
		{
			FMyBondageLifeGagProfile& Profile = Profiles[Index];
			if (Profile.Base.IsNone() || Merged.Contains(Profile.Id))
			{
				continue;
			}

			const int32* BaseIndex = ProfileIndex.Find(Profile.Base.ToString());
			if (!BaseIndex || *BaseIndex == Index)
			{
				Merged.Add(Profile.Id);
				continue;
			}

			const FMyBondageLifeGagProfile& BaseProfile = Profiles[*BaseIndex];
			if (!BaseProfile.Base.IsNone() && !Merged.Contains(BaseProfile.Id))
			{
				continue;	// 等基准档位先合并完
			}

			// 只补缺失的音素，变体自己写的条目优先。
			for (const TPair<FString, FMyBondageLifeGagRule>& Pair : BaseProfile.Zh)
			{
				if (!Profile.Zh.Contains(Pair.Key))
				{
					Profile.Zh.Add(Pair.Key, Pair.Value);
				}
			}
			for (const TPair<FString, FMyBondageLifeGagRule>& Pair : BaseProfile.En)
			{
				if (!Profile.En.Contains(Pair.Key))
				{
					Profile.En.Add(Pair.Key, Pair.Value);
				}
			}
			Merged.Add(Profile.Id);
			bProgress = true;
		}
		if (!bProgress)
		{
			break;
		}
	}

	return true;
}

bool FMyBondageLifeGarbler::LoadHanzi(const FString& Json)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogGarbler, Error, TEXT("拼音表 JSON 解析失败。"));
		return false;
	}

	HanziPinyin.Reset();
	HanziPinyin.Reserve(Root->Values.Num());
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Root->Values)
	{
		if (Pair.Key.Len() != 1 || !IsCjk(Pair.Key[0]))
		{
			continue;
		}
		HanziPinyin.Add(Pair.Key[0], Pair.Value->AsString());
	}

	if (HanziPinyin.Num() == 0)
	{
		UE_LOG(LogGarbler, Error, TEXT("拼音表为空。"));
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
//  查询接口
// ---------------------------------------------------------------------------

const TArray<FName>& FMyBondageLifeGarbler::GetProfileIds()
{
	EnsureLoaded();
	return ProfileIds;
}

bool FMyBondageLifeGarbler::IsSilentProfile(FName ProfileId)
{
	EnsureLoaded();
	if (const int32* Index = ProfileIndex.Find(ProfileId.ToString()))
	{
		return Profiles[*Index].bSilent;
	}
	return false;
}

FString FMyBondageLifeGarbler::GetProfileDisplayName(FName ProfileId)
{
	EnsureLoaded();
	if (const int32* Index = ProfileIndex.Find(ProfileId.ToString()))
	{
		return Profiles[*Index].DisplayName;
	}
	return ProfileId.ToString();
}

FName FMyBondageLifeGarbler::ResolveProfileId(const FString& IdOrDisplayNameOrAlias)
{
	EnsureLoaded();
	const FString Key = IdOrDisplayNameOrAlias.TrimStartAndEnd().ToLower();
	if (Key.IsEmpty())
	{
		return NAME_None;
	}
	if (const FName* Found = AliasToId.Find(Key))
	{
		return *Found;
	}
	return NAME_None;
}

FString FMyBondageLifeGarbler::DescribeProfile(FName ProfileId)
{
	EnsureLoaded();
	const int32* Index = ProfileIndex.Find(ProfileId.ToString());
	if (!Index)
	{
		return FString();
	}
	const FMyBondageLifeGagProfile& Profile = Profiles[*Index];
	return FString::Printf(TEXT("%s(%s) 音素 %d+%d 兜底音 %d%s"),
		*Profile.Id.ToString(), *Profile.DisplayName,
		Profile.Zh.Num(), Profile.En.Num(), Profile.FallbackSounds.Num(),
		Profile.bSilent ? TEXT(" 完全无声") : TEXT(""));
}

// ---------------------------------------------------------------------------
//  词法工具
// ---------------------------------------------------------------------------

bool FMyBondageLifeGarbler::IsCjk(TCHAR Ch)
{
	return Ch >= 0x4E00 && Ch <= 0x9FFF;
}

bool FMyBondageLifeGarbler::IsWordChar(TCHAR Ch)
{
	return FChar::IsAlpha(Ch) || FChar::IsDigit(Ch) || Ch == TEXT('\'') || Ch == TEXT('-');
}

bool FMyBondageLifeGarbler::IsAsciiDigit(TCHAR Ch)
{
	return Ch >= TEXT('0') && Ch <= TEXT('9');
}

const FString* FMyBondageLifeGarbler::FindPinyin(TCHAR Hanzi)
{
	return HanziPinyin.Find(Hanzi);
}

void FMyBondageLifeGarbler::SplitPinyin(const FString& Syllable, FString& OutInitial, FString& OutFinal)
{
	OutInitial.Reset();
	OutFinal = Syllable;

	for (const FString& Candidate : ZhInitials)
	{
		if (Syllable.Len() > Candidate.Len() && Syllable.StartsWith(Candidate, ESearchCase::CaseSensitive))
		{
			OutInitial = Candidate;
			OutFinal = Syllable.Mid(Candidate.Len());
			return;
		}
	}
}

void FMyBondageLifeGarbler::SyllableToPhonemes(const FString& Syllable, TArray<FString>& OutPhonemes)
{
	FString Initial;
	FString Final;
	SplitPinyin(Syllable, Initial, Final);

	if (!Initial.IsEmpty())
	{
		OutPhonemes.Add(Initial);
	}
	if (Final.IsEmpty())
	{
		return;
	}
	if (ZhFinals.Contains(Final))
	{
		OutPhonemes.Add(Final);
	}
	else
	{
		// 罕见韵母：按单字母拆，交给兜底发音处理
		for (int32 Index = 0; Index < Final.Len(); ++Index)
		{
			OutPhonemes.Add(FString::Chr(Final[Index]));
		}
	}
}

void FMyBondageLifeGarbler::LatinToPhonemes(const FString& Word, TArray<FString>& OutPhonemes)
{
	const FString Lower = Word.ToLower();
	int32 Cursor = 0;
	while (Cursor < Lower.Len())
	{
		bool bMatched = false;
		for (const FLatinRule& Rule : LatinRules)
		{
			if (Lower.Len() - Cursor >= Rule.Grapheme.Len() && Lower.Mid(Cursor, Rule.Grapheme.Len()) == Rule.Grapheme)
			{
				OutPhonemes.Append(Rule.Phonemes);
				Cursor += Rule.Grapheme.Len();
				bMatched = true;
				break;
			}
		}
		if (!bMatched)
		{
			++Cursor;	// 不认识的字符直接跳过
		}
	}
}

// ---------------------------------------------------------------------------
//  混淆
// ---------------------------------------------------------------------------

FString FMyBondageLifeGarbler::ApplyCase(const FString& Source, const FString& Garbled)
{
	if (Garbled.IsEmpty())
	{
		return Garbled;
	}

	bool bHasLetter = false;
	bool bAllUpper = true;
	for (int32 Index = 0; Index < Source.Len(); ++Index)
	{
		const TCHAR Ch = Source[Index];
		if (FChar::IsAlpha(Ch))
		{
			bHasLetter = true;
			if (FChar::IsLower(Ch))
			{
				bAllUpper = false;
				break;
			}
		}
	}

	if (bHasLetter && bAllUpper)
	{
		return Garbled.ToUpper();
	}
	if (bHasLetter && FChar::IsUpper(Source[0]))
	{
		return FString::Chr(FChar::ToUpper(Garbled[0])) + Garbled.Mid(1);
	}
	return Garbled;
}

FString FMyBondageLifeGarbler::SilentResidue(FRandomStream& Rng)
{
	// 完全无声的档位偶尔漏出一点气音，多数时候什么都不剩。
	if (Rng.FRand() < 0.45f)
	{
		return FString();
	}

	static const TCHAR* const Cores[] = { TEXT("m"), TEXT("n"), TEXT("h"), TEXT("") };
	const FString Core = Cores[Rng.RandRange(0, UE_ARRAY_COUNT(Cores) - 1)];
	const int32 Dots = Rng.RandRange(1, 2);
	return Core + FString::ChrN(Dots, TEXT('.'));
}

FString FMyBondageLifeGarbler::GarblePhonemes(const TArray<FString>& Phonemes,
	const TArray<const FMyBondageLifeGagProfile*>& Active, bool bZh, FRandomStream& Rng)
{
	if (Active.Num() == 0)
	{
		return FString();
	}

	FString Result;
	for (const FString& Phoneme : Phonemes)
	{
		const FMyBondageLifeGagRule* Best = nullptr;
		for (const FMyBondageLifeGagProfile* Profile : Active)
		{
			const TMap<FString, FMyBondageLifeGagRule>& Table = bZh ? Profile->Zh : Profile->En;
			const FMyBondageLifeGagRule* Rule = Table.Find(Phoneme);
			if (Rule && (!Best || Rule->Muffle > Best->Muffle))
			{
				Best = Rule;
			}
		}

		if (Best)
		{
			Result += Best->Sound;
		}
		else
		{
			const TArray<FString>& Fallbacks = Active[0]->FallbackSounds;
			if (Fallbacks.Num() > 0)
			{
				Result += Fallbacks[Rng.RandRange(0, Fallbacks.Num() - 1)];
			}
		}
	}
	return Result;
}

FString FMyBondageLifeGarbler::Garble(const FString& Text, const TArray<FName>& GagIds,
	const FMyBondageLifeGarblerOptions& Options)
{
	if (Text.IsEmpty() || GagIds.Num() == 0 || !EnsureLoaded())
	{
		return Text;
	}

	// 收集生效档位，并按表内顺序排序：muffle 打平时由表顺序裁决，结果稳定。
	TArray<TPair<int32, const FMyBondageLifeGagProfile*>> Sorted;
	for (const FName& Id : GagIds)
	{
		if (const int32* Index = ProfileIndex.Find(Id.ToString()))
		{
			Sorted.Emplace(*Index, &Profiles[*Index]);
		}
	}
	if (Sorted.Num() == 0)
	{
		return Text;
	}
	Sorted.Sort([](const TPair<int32, const FMyBondageLifeGagProfile*>& A, const TPair<int32, const FMyBondageLifeGagProfile*>& B)
	{
		return A.Key < B.Key;
	});

	TArray<const FMyBondageLifeGagProfile*> Active;
	Active.Reserve(Sorted.Num());
	for (const TPair<int32, const FMyBondageLifeGagProfile*>& Entry : Sorted)
	{
		Active.Add(Entry.Value);
	}

	bool bAllSilent = true;
	bool bAllNoResidue = true;
	for (const FMyBondageLifeGagProfile* Profile : Active)
	{
		bAllSilent &= Profile->bSilent;
		bAllNoResidue &= Profile->bNoResidue;
	}

	const int32 Seed = Options.RandomSeed != 0
		? Options.RandomSeed
		: static_cast<int32>(FPlatformTime::Cycles() & 0x7FFFFFFF);
	FRandomStream Rng(Seed);

	FString Out;
	Out.Reserve(Text.Len() * 2);

	bool bInsideRp = false;
	int32 Cursor = 0;
	while (Cursor < Text.Len())
	{
		const TCHAR Ch = Text[Cursor];

		// *...* RP 段
		if (Ch == TEXT('*') && Options.bPreserveRoleplaySegments)
		{
			bInsideRp = !bInsideRp;
			Out.AppendChar(Ch);
			++Cursor;
			continue;
		}

		// :Emoji:
		if (Ch == TEXT(':') && Options.bPreserveEmotes)
		{
			int32 Close = Cursor + 1;
			bool bValid = false;
			while (Close < Text.Len() && (Close - Cursor) <= 33)
			{
				const TCHAR Inner = Text[Close];
				if (Inner == TEXT(':'))
				{
					bValid = Close > Cursor + 1;
					break;
				}
				if (!(FChar::IsAlnum(Inner) || Inner == TEXT('_') || Inner == TEXT('+') || Inner == TEXT('-')))
				{
					break;
				}
				++Close;
			}
			if (bValid)
			{
				Out += Text.Mid(Cursor, Close - Cursor + 1);
				Cursor = Close + 1;
				continue;
			}
		}

		if (bInsideRp)
		{
			Out.AppendChar(Ch);
			++Cursor;
			continue;
		}

		// 连续汉字
		if (IsCjk(Ch))
		{
			int32 End = Cursor;
			while (End < Text.Len() && IsCjk(Text[End]))
			{
				++End;
			}

			if (bAllSilent)
			{
				// 密封头套XL 这类档位连残响都不给，整句保持空。
				if (!bAllNoResidue)
				{
					for (int32 Index = Cursor; Index < End; ++Index)
					{
						Out += SilentResidue(Rng);
					}
				}
			}
			else
			{
				TArray<FString> Phonemes;
				for (int32 Index = Cursor; Index < End; ++Index)
				{
					if (const FString* Pinyin = FindPinyin(Text[Index]))
					{
						Phonemes.Reset();
						SyllableToPhonemes(*Pinyin, Phonemes);
						Out += GarblePhonemes(Phonemes, Active, true, Rng);
					}
					else if (Options.bGarbleUnknownWords)
					{
						Out += TEXT("m");	// 生僻字：查不到拼音，给个闷响
					}
					else
					{
						Out.AppendChar(Text[Index]);
					}
				}
			}

			Cursor = End;
			continue;
		}

		// 拉丁字母 / 数字
		if (Ch < 0x80 && IsWordChar(Ch))
		{
			int32 End = Cursor;
			while (End < Text.Len() && Text[End] < 0x80 && IsWordChar(Text[End]))
			{
				++End;
			}
			const FString Word = Text.Mid(Cursor, End - Cursor);

			if (bAllSilent)
			{
				if (!bAllNoResidue)
				{
					const int32 Pieces = FMath::Max(1, Word.Len() / 3);
					for (int32 Index = 0; Index < Pieces; ++Index)
					{
						Out += SilentResidue(Rng);
					}
				}
			}
			else
			{
				FString Built;
				TArray<FString> Phonemes;
				int32 Scan = 0;
				while (Scan < Word.Len())
				{
					if (IsAsciiDigit(Word[Scan]))
					{
						int32 DigitsEnd = Scan;
						while (DigitsEnd < Word.Len() && IsAsciiDigit(Word[DigitsEnd]))
						{
							++DigitsEnd;
						}
						Phonemes.Reset();
						for (int32 Index = Scan; Index < DigitsEnd; ++Index)
						{
							if (const FString* Pinyin = DigitZh.Find(Word[Index]))
							{
								SyllableToPhonemes(*Pinyin, Phonemes);
							}
						}
						Built += GarblePhonemes(Phonemes, Active, true, Rng);
						Scan = DigitsEnd;
					}
					else
					{
						int32 LettersEnd = Scan;
						while (LettersEnd < Word.Len() && !IsAsciiDigit(Word[LettersEnd]))
						{
							++LettersEnd;
						}
						Phonemes.Reset();
						LatinToPhonemes(Word.Mid(Scan, LettersEnd - Scan), Phonemes);
						Built += GarblePhonemes(Phonemes, Active, false, Rng);
						Scan = LettersEnd;
					}
				}
				Out += ApplyCase(Word, Built);
			}

			Cursor = End;
			continue;
		}

		Out.AppendChar(Ch);
		++Cursor;
	}

	return Out;
}

FString FMyBondageLifeGarbler::ToPhonemes(const FString& Text)
{
	if (!EnsureLoaded())
	{
		return Text;
	}

	FString Out;
	int32 Cursor = 0;
	while (Cursor < Text.Len())
	{
		const TCHAR Ch = Text[Cursor];

		if (IsCjk(Ch))
		{
			int32 End = Cursor;
			while (End < Text.Len() && IsCjk(Text[End]))
			{
				++End;
			}
			for (int32 Index = Cursor; Index < End; ++Index)
			{
				const FString* Pinyin = FindPinyin(Text[Index]);
				if (!Pinyin)
				{
					Out.AppendChar(Text[Index]);
					continue;
				}
				TArray<FString> Phonemes;
				SyllableToPhonemes(*Pinyin, Phonemes);
				Out += FString::Chr(Text[Index]);
				Out += FString::Printf(TEXT("[%s=%s] "), **Pinyin, *JoinWith(Phonemes, TEXT("-")));
			}
			Cursor = End;
			continue;
		}

		if (Ch < 0x80 && IsWordChar(Ch))
		{
			int32 End = Cursor;
			while (End < Text.Len() && Text[End] < 0x80 && IsWordChar(Text[End]))
			{
				++End;
			}
			TArray<FString> Phonemes;
			LatinToPhonemes(Text.Mid(Cursor, End - Cursor), Phonemes);
			Out += FString::Printf(TEXT("%s[%s] "), *Text.Mid(Cursor, End - Cursor), *JoinWith(Phonemes, TEXT("-")));
			Cursor = End;
			continue;
		}

		Out.AppendChar(Ch);
		++Cursor;
	}
	return Out.TrimEnd();
}
