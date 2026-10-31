# -*- coding: utf-8 -*-
"""
把 Data/*.json 内嵌成 C++ 源码，供打包后的版本在没有外置文件时兜底。

MSVC 单个字符串字面量有长度上限，所以这里按块切开，运行时再拼起来。

用法（改完音素表后执行一次）：
    python gen_embedded.py
输出：
    ../MyBondageLifeGarblerEmbedded.cpp
"""

import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.normpath(os.path.join(HERE, "..", "Data"))
OUT_CPP = os.path.normpath(os.path.join(HERE, "..", "MyBondageLifeGarblerEmbedded.cpp"))

CHUNK_CHARS = 2000

HEADER = """// Copyright Epic Games, Inc. All Rights Reserved.
//
// 由 Tools/gen_embedded.py 自动生成，请勿手工修改。
// 内容 = Data/GarblerTables.json + Data/HanziPinyin.json 的紧凑版本，
// 只在打包后读取不到外置 JSON 时使用。

#include "Core/Garbler/MyBondageLifeGarblerEmbedded.h"

namespace
{
"""


def emit_chunks(name: str, text: str) -> str:
    out = [f"\tconst TCHAR* const {name}Chunks[] =\n\t{{\n"]
    tag_index = 0
    for start in range(0, len(text), CHUNK_CHARS):
        piece = text[start:start + CHUNK_CHARS]
        tag = f"GS{tag_index}"
        out.append(f"\t\tLR\"{tag}({piece}){tag}\",\n")
        tag_index += 1
    out.append("\t};\n\n")
    out.append(
        f"\tFString Join_{name}()\n\t{{\n"
        f"\t\tFString Result;\n"
        f"\t\tfor (const TCHAR* Chunk : {name}Chunks)\n\t\t{{\n"
        f"\t\t\tResult += Chunk;\n\t\t}}\n"
        f"\t\treturn Result;\n\t}}\n\n"
    )
    return "".join(out)


FOOTER = """}

const TCHAR* FMyBondageLifeGarblerEmbedded::GetTablesJson()
{
	// 首次调用时拼一次，之后复用（避免每帧重复拼接 100KB）。
	static const FString Cached = Join_Tables();
	return *Cached;
}

const TCHAR* FMyBondageLifeGarblerEmbedded::GetHanziJson()
{
	static const FString Cached = Join_Hanzi();
	return *Cached;
}
"""


def main():
    with open(os.path.join(DATA_DIR, "GarblerTables.json"), encoding="utf-8") as f:
        tables = json.load(f)
    with open(os.path.join(DATA_DIR, "HanziPinyin.json"), encoding="utf-8") as f:
        hanzi = json.load(f)

    # 紧凑化，减小内嵌体积
    tables_txt = json.dumps(tables, ensure_ascii=False, separators=(",", ":"))
    hanzi_txt = json.dumps(hanzi, ensure_ascii=False, separators=(",", ":"))

    body = []
    body.append(emit_chunks("Tables", tables_txt))
    body.append(emit_chunks("Hanzi", hanzi_txt))

    with open(OUT_CPP, "w", encoding="utf-8", newline="\n") as f:
        f.write(HEADER)
        f.write("".join(body))
        f.write(FOOTER)

    print("wrote", OUT_CPP, os.path.getsize(OUT_CPP), "bytes")
    print("tables chunks:", (len(tables_txt) + CHUNK_CHARS - 1) // CHUNK_CHARS,
          "hanzi chunks:", (len(hanzi_txt) + CHUNK_CHARS - 1) // CHUNK_CHARS)


if __name__ == "__main__":
    main()
