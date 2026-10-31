// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 *  内嵌的音素表 / 拼音表。
 *
 *  正常情况下 Garbler 优先读 Data/*.json；只有读不到文件（例如打包后没有一起拷出去）时
 *  才退回这里的数据，保证功能永远不会失效。
 *
 *  内容由 Tools/gen_embedded.py 从 Data/*.json 生成，不要手工修改。
 */
namespace FMyBondageLifeGarblerEmbedded
{
	MYBONDAGELIFE_API const TCHAR* GetTablesJson();
	MYBONDAGELIFE_API const TCHAR* GetHanziJson();
}
