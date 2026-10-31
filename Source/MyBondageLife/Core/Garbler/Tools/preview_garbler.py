# -*- coding: utf-8 -*-
"""
Garbler 音素表的"参考实现 + 试听器"。

用途:
  1. 在改音素表后立刻看到效果, 不用编译 UE。
  2. 作为 C++ 实现(FMyBondageLifeGarbler)的对照基准, 两者行为应保持一致。

用法:
    python preview_garbler.py
"""

import json
import os
import random
import re

HERE = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.normpath(os.path.join(HERE, "..", "Data"))


def load():
    with open(os.path.join(DATA_DIR, "GarblerTables.json"), encoding="utf-8") as f:
        tables = json.load(f)
    with open(os.path.join(DATA_DIR, "HanziPinyin.json"), encoding="utf-8") as f:
        pinyin = json.load(f)
    _resolve_bases(tables)
    return tables, pinyin


def _resolve_bases(tables):
    """变体档位只写了差异部分，这里把 base 档位的音素合并进来（与 C++ 行为一致）。"""
    by_id = {p["id"]: p for p in tables["profiles"]}

    def merge(profile, seen):
        base_id = profile.get("base")
        if not base_id or base_id in seen or base_id not in by_id:
            return
        parent = by_id[base_id]
        merge(parent, seen | {profile["id"]})
        for lang in ("zh", "en"):
            merged = dict(parent.get(lang, {}))
            merged.update(profile.get(lang, {}))
            profile[lang] = merged

    for profile in tables["profiles"]:
        merge(profile, set())


class Garbler:
    def __init__(self, tables, pinyin):
        self.t = tables
        self.pinyin_map = pinyin
        # 档位顺序 = 表里的顺序, 用于多个堵嘴 muffle 相同时的稳定裁决
        self.order = {p["id"]: i for i, p in enumerate(tables["profiles"])}
        self.profiles = {p["id"]: p for p in tables["profiles"]}
        self.initials = sorted(tables["zhInitials"], key=len, reverse=True)
        self.finals = set(tables["zhFinals"])
        self.graphemes = sorted(tables["graphemesEn"], key=lambda r: len(r["g"]), reverse=True)
        self.digits_zh = tables["digitsZh"]
        self.digits_en = tables["digitsEn"]

    # ---- 拼音切分 -------------------------------------------------------
    def split_pinyin(self, syl):
        init = ""
        for cand in self.initials:
            if syl.startswith(cand):
                init = cand
                break
        fin = syl[len(init):]
        if fin in self.finals or not fin:
            return init, fin
        return init, fin  # 罕见韵母: 交给调用方按字母拆

    def zh_phonemes(self, ch):
        py = self.pinyin_map.get(ch)
        if py is None:
            return None
        init, fin = self.split_pinyin(py)
        out = []
        if init:
            out.append(init)
        if fin:
            if fin in self.finals:
                out.append(fin)
            else:
                out.extend(list(fin))
        return out

    # ---- 英文 -> IPA ----------------------------------------------------
    def en_phonemes(self, word):
        w = word.lower()
        out = []
        i = 0
        while i < len(w):
            for rule in self.graphemes:
                g = rule["g"]
                if w.startswith(g, i):
                    out.extend(rule["p"])
                    i += len(g)
                    break
            else:
                i += 1
        return out

    # ---- 音素 -> 堵嘴后发音 --------------------------------------------
    def garble_phonemes(self, phonemes, active, lang, rng):
        res = []
        for ph in phonemes:
            best = None
            for prof in active:
                rule = prof[lang].get(ph)
                if rule is None:
                    continue
                if best is None or rule["muffle"] > best[0]:
                    best = (rule["muffle"], rule["sound"])
            if best is None:
                res.append(rng.choice(active[0]["fallbackSounds"]))
            else:
                res.append(best[1])
        return "".join(res)

    @staticmethod
    def apply_case(src, dst):
        if not dst:
            return dst
        letters = [c for c in src if c.isalpha()]
        if letters and all(c.isupper() for c in letters):
            return dst.upper()
        if letters and src[0].isupper():
            return dst[0].upper() + dst[1:]
        return dst

    def no_sound_syllable(self, rng):
        """HoodGag 之类完全无声的档位: 偶尔留一点残响。"""
        if rng.random() < 0.45:
            return ""
        core = rng.choice(["m", "n", "h", ""])
        return core + "." * rng.randint(1, 2)

    # ---- 入口 -----------------------------------------------------------
    def garble(self, text, gag_ids, seed=0, preserve_rp=True, preserve_emote=True):
        active = [self.profiles[g] for g in gag_ids if g in self.profiles]
        if not active:
            return text
        # 稳定排序: muffle 相同时由表内顺序裁决, 不受调用方传参顺序影响
        active.sort(key=lambda p: self.order[p["id"]])
        rng = random.Random(seed if seed else None)
        silent = all(p["silent"] for p in active)
        no_residue = all(p.get("noResidue", False) for p in active)

        out = []
        rp = False
        i, n = 0, len(text)
        while i < n:
            ch = text[i]

            if ch == "*" and preserve_rp:
                rp = not rp
                out.append(ch)
                i += 1
                continue

            if ch == ":" and preserve_emote:
                m = re.match(r":[A-Za-z0-9_+\-]{1,32}:", text[i:])
                if m:
                    out.append(m.group(0))
                    i += len(m.group(0))
                    continue

            if rp:
                out.append(ch)
                i += 1
                continue

            # --- 连续汉字 = 一个"词"(中文没有空格, 这样无声档位才能按词出残响)
            if "\u4e00" <= ch <= "\u9fff":
                j = i
                while j < n and "\u4e00" <= text[j] <= "\u9fff":
                    j += 1
                run = text[i:j]
                if silent:
                    if no_residue:
                        out.append("")
                    else:
                        out.append("".join(self.no_sound_syllable(rng) for _ in run))
                else:
                    parts = []
                    for c in run:
                        ph = self.zh_phonemes(c)
                        parts.append(self.garble_phonemes(ph, active, "zh", rng) if ph else "")
                    out.append("".join(parts))
                i = j
                continue

            # --- 拉丁字母 / 数字
            if ch.isascii() and (ch.isalnum() or ch in "'-"):
                j = i
                while j < n and text[j].isascii() and (text[j].isalnum() or text[j] in "'-"):
                    j += 1
                word = text[i:j]
                if silent:
                    out.append(self.no_sound_syllable(rng) * max(1, len(word) // 3))
                else:
                    parts = []
                    k = 0
                    while k < len(word):
                        c = word[k]
                        if c.isdigit():
                            m = k
                            while m < len(word) and word[m].isdigit():
                                m += 1
                            ph = []
                            for d in word[k:m]:
                                ini, fin = self.split_pinyin(self.digits_zh[d])
                                if ini:
                                    ph.append(ini)
                                if fin:
                                    if fin in self.finals:
                                        ph.append(fin)
                                    else:
                                        ph.extend(list(fin))
                            parts.append(self.garble_phonemes(ph, active, "zh", rng))
                            k = m
                        else:
                            m = k
                            while m < len(word) and not word[m].isdigit():
                                m += 1
                            ph = self.en_phonemes(word[k:m])
                            parts.append(self.garble_phonemes(ph, active, "en", rng))
                            k = m
                    out.append(self.apply_case(word, "".join(parts)))
                i = j
                continue

            out.append(ch)
            i += 1

        return "".join(out)


def main():
    tables, pinyin = load()
    g = Garbler(tables, pinyin)
    samples = [
        "你好，今天天气不错。",
        "别说话，我马上就回来。",
        "求求你放开我……",
        "我嘴里被塞了东西，说话有点困难",
        "Hello world, this is a test.",
        "我的QQ是12345，记得加我",
        "*挣扎* 放、放开我…… *喘气*",
    ]
    for prof in tables["profiles"]:
        pid = prof["id"]
        print("=" * 78)
        print(f"档位: {pid}  ({prof['displayName']})  —  {prof['description']}")
        print("=" * 78)
        for s in samples:
            print(f"  原文: {s}")
            print(f"  混淆: {g.garble(s, [pid])}")
        print()

    print("=" * 78)
    print("组合: 口球 + 布条封口(同一音素取 muffle 更大的那一档)")
    print("=" * 78)
    for s in samples:
        print(f"  原文: {s}")
        print(f"  混淆: {g.garble(s, ['BallGag', 'CleaveGag'])}")
    print()


if __name__ == "__main__":
    main()
