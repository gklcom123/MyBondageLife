# -*- coding: utf-8 -*-
"""
生成堵嘴混淆器(Garbler)的音素表。

输出:
    ../Data/GarblerTables.json     音素表(中文拼音音素 + 英文 IPA 音素 + 英文拼写规则)
    ../Data/HanziPinyin.json       汉字 -> 无声调拼音(由 build_hanzi_pinyin.py 产生)

设计说明
--------
每个"堵嘴档位"(profile) 都保存了 [音素 -> (抑制强度 muffle, 替换后发音 sound)] 的完整映射,
运行时按 GagSpeak 的规则做竞争: 多个堵嘴同时佩戴时, 同一个音素取 muffle 最大的那一档的 sound。

muffle 语义:
    0 = 该档位不影响这个音素(原样保留)
    1 = 轻度抑制
    2 = 重度抑制
    3 = 完全抑制

所以音素表既是"发音表"也是"抑制权重表", 调整某一档的听感只需要改 sound, 调整它与其它档的
竞争关系只需要改 muffle。
"""

import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.normpath(os.path.join(HERE, "..", "Data"))

# ---------------------------------------------------------------------------
#  中文音素表
#  声母 23 个(含零声母 y / w) + 韵母 37 个
# ---------------------------------------------------------------------------

ZH_INITIALS = [
    "b", "p", "m", "f", "d", "t", "n", "l",
    "g", "k", "h", "j", "q", "x",
    "zh", "ch", "sh", "r", "z", "c", "s",
    "y", "w",
]

ZH_FINALS = [
    "a", "o", "e", "i", "u", "v", "er",
    "ai", "ei", "ao", "ou",
    "an", "en", "ang", "eng", "ong",
    "ia", "ie", "iao", "iu", "ian", "in", "iang", "ing", "iong",
    "ua", "uo", "uai", "ui", "uan", "un", "uang", "ueng",
    "ve", "van", "vn",
]

# 声母在各档位下的发音。键 = 档位 id, 值 = {音素: (muffle, sound)}
# "" 表示这个音素被吞掉。
ZH_INITIAL_SOUNDS = {
    # 开口环: 嘴被撑开, 双唇音全部失效(变成 w/h), 舌尖音基本保留, 元音完全清楚。
    "RingGag": {
        "b": (2, "w"), "p": (2, "w"), "m": (1, "m"), "f": (2, "h"),
        "d": (1, "d"), "t": (1, "t"), "n": (1, "n"), "l": (1, "l"),
        "g": (1, "g"), "k": (1, "k"), "h": (0, "h"),
        "j": (2, "sh"), "q": (2, "sh"), "x": (1, "sh"),
        "zh": (1, "sh"), "ch": (1, "sh"), "sh": (1, "sh"), "r": (1, "r"),
        "z": (1, "s"), "c": (1, "s"), "s": (1, "s"),
        "y": (0, "y"), "w": (1, "w"),
    },
    # 口球: 舌头被压住, 所有塞音/擦音都塌成鼻音或 sh, 只有元音还能听出调子。
    "BallGag": {
        "b": (3, "m"), "p": (3, "m"), "m": (2, "m"), "f": (3, "mh"),
        "d": (3, "n"), "t": (3, "n"), "n": (2, "n"), "l": (3, "n"),
        "g": (3, "ng"), "k": (3, "ng"), "h": (2, ""),
        "j": (3, "sh"), "q": (3, "sh"), "x": (3, "sh"),
        "zh": (3, "sh"), "ch": (3, "sh"), "sh": (2, "sh"), "r": (3, "w"),
        "z": (3, "s"), "c": (3, "s"), "s": (2, "s"),
        "y": (1, "y"), "w": (1, "w"),
    },
    # 面板口枷: 整张嘴被面板压住, 只剩沉闷的鼻音。
    "PanelGag": {
        "b": (3, "m"), "p": (3, "m"), "m": (3, "m"), "f": (3, "m"),
        "d": (3, "n"), "t": (3, "n"), "n": (3, "n"), "l": (3, "n"),
        "g": (3, "ng"), "k": (3, "ng"), "h": (3, ""),
        "j": (3, "n"), "q": (3, "n"), "x": (3, "n"),
        "zh": (3, "n"), "ch": (3, "n"), "sh": (3, "n"), "r": (3, "n"),
        "z": (3, "n"), "c": (3, "n"), "s": (3, "n"),
        "y": (3, ""), "w": (3, ""),
    },
    # 布条/胶带封口: 嘴唇完全封死, 声母全部消失。
    "CleaveGag": {
        "b": (3, ""), "p": (3, ""), "m": (3, "m"), "f": (3, ""),
        "d": (3, ""), "t": (3, ""), "n": (3, "n"), "l": (3, ""),
        "g": (3, ""), "k": (3, ""), "h": (3, ""),
        "j": (3, ""), "q": (3, ""), "x": (3, ""),
        "zh": (3, ""), "ch": (3, ""), "sh": (3, ""), "r": (3, ""),
        "z": (3, ""), "c": (3, ""), "s": (3, ""),
        "y": (3, ""), "w": (3, ""),
    },
    # 填塞(袜子/内裤): 比封口更闷, 只剩一点点漏气声。
    "StuffGag": {
        "b": (3, ""), "p": (3, ""), "m": (3, "mh"), "f": (3, ""),
        "d": (3, ""), "t": (3, ""), "n": (3, "n"), "l": (3, ""),
        "g": (3, ""), "k": (3, ""), "h": (3, ""),
        "j": (3, ""), "q": (3, ""), "x": (3, ""),
        "zh": (3, ""), "ch": (3, ""), "sh": (3, ""), "r": (3, ""),
        "z": (3, ""), "c": (3, ""), "s": (3, ""),
        "y": (3, ""), "w": (3, ""),
    },
    # 密封头套: 完全无声, 走"无声词"逻辑(只剩 …… / mnm)。
    "HoodGag": {p: (3, "") for p in ZH_INITIALS},
}

# 韵母发音。开口环/口球保留元音(口球把元音压扁), 面板之后全部变成鼻音。
ZH_FINAL_SOUNDS = {
    # 开口环: 元音仍然清楚, 只是被撑开的嘴型稍微改变音色(muffle 1)。
    "RingGag": {
        "a": (1, "ah"), "o": (1, "oh"), "e": (1, "eh"), "i": (1, "ih"),
        "u": (1, "oo"), "v": (1, "yu"), "er": (1, "err"),
        "ai": (1, "ai"), "ei": (1, "ei"), "ao": (1, "aw"), "ou": (1, "oh"),
        "an": (1, "ahn"), "en": (1, "ehn"), "ang": (1, "ahng"), "eng": (1, "ehng"), "ong": (1, "ohng"),
        "ia": (1, "yah"), "ie": (1, "yeh"), "iao": (1, "yaw"), "iu": (1, "yoo"),
        "ian": (1, "yahn"), "in": (1, "ihn"), "iang": (1, "yahng"), "ing": (1, "ihng"), "iong": (1, "yohng"),
        "ua": (1, "wah"), "uo": (1, "woh"), "uai": (1, "wai"), "ui": (1, "way"),
        "uan": (1, "wahn"), "un": (1, "wuhn"), "uang": (1, "wahng"), "ueng": (1, "wuhng"),
        "ve": (1, "yweh"), "van": (1, "ywahn"), "vn": (1, "yuhn"),
    },
    "BallGag": {
        "a": (2, "ah"), "o": (2, "oh"), "e": (2, "eh"), "i": (2, "ih"),
        "u": (2, "oo"), "v": (2, "yu"), "er": (2, "err"),
        "ai": (2, "ai"), "ei": (2, "ei"), "ao": (2, "aw"), "ou": (2, "oh"),
        "an": (2, "ahn"), "en": (2, "ehn"), "ang": (2, "ahng"), "eng": (2, "ehng"), "ong": (2, "ohng"),
        "ia": (2, "yah"), "ie": (2, "yeh"), "iao": (2, "yaw"), "iu": (2, "yoo"),
        "ian": (2, "yahn"), "in": (2, "ihn"), "iang": (2, "yahng"), "ing": (2, "ihng"), "iong": (2, "yohng"),
        "ua": (2, "wah"), "uo": (2, "woh"), "uai": (2, "wai"), "ui": (2, "way"),
        "uan": (2, "wahn"), "un": (2, "wuhn"), "uang": (2, "wahng"), "ueng": (2, "wuhng"),
        "ve": (2, "yweh"), "van": (2, "ywahn"), "vn": (2, "yuhn"),
    },
    # 面板口枷: 整张嘴被面板压住 —— 元音还在, 但全部带上鼻音尾巴。
    "PanelGag": {
        "a": (3, "uhm"), "o": (3, "ohm"), "e": (3, "ehm"), "i": (3, "ihm"),
        "u": (3, "uhm"), "v": (3, "yuhm"), "er": (3, "uhm"),
        "ai": (3, "aim"), "ei": (3, "eim"), "ao": (3, "aum"), "ou": (3, "ohm"),
        "an": (3, "ahn"), "en": (3, "ehn"), "ang": (3, "ahng"), "eng": (3, "ehng"), "ong": (3, "ohng"),
        "ia": (3, "yahm"), "ie": (3, "yehm"), "iao": (3, "yawm"), "iu": (3, "yoom"),
        "ian": (3, "yahn"), "in": (3, "ihn"), "iang": (3, "yahng"), "ing": (3, "ihng"), "iong": (3, "yohng"),
        "ua": (3, "wahm"), "uo": (3, "wohm"), "uai": (3, "waim"), "ui": (3, "waym"),
        "uan": (3, "wahn"), "un": (3, "wuhn"), "uang": (3, "wahng"), "ueng": (3, "wuhng"),
        "ve": (3, "ywehm"), "van": (3, "ywahn"), "vn": (3, "yuhn"),
    },
    "CleaveGag": {
        "a": (3, "m"), "o": (3, "m"), "e": (3, "m"), "i": (3, "m"),
        "u": (3, "m"), "v": (3, "m"), "er": (3, "m"),
        "ai": (3, "m"), "ei": (3, "m"), "ao": (3, "m"), "ou": (3, "m"),
        "an": (3, "m"), "en": (3, "m"), "ang": (3, "m"), "eng": (3, "m"), "ong": (3, "m"),
        "ia": (3, "m"), "ie": (3, "m"), "iao": (3, "m"), "iu": (3, "m"),
        "ian": (3, "m"), "in": (3, "m"), "iang": (3, "m"), "ing": (3, "m"), "iong": (3, "m"),
        "ua": (3, "m"), "uo": (3, "m"), "uai": (3, "m"), "ui": (3, "m"),
        "uan": (3, "m"), "un": (3, "m"), "uang": (3, "m"), "ueng": (3, "m"),
        "ve": (3, "m"), "van": (3, "m"), "vn": (3, "m"),
    },
    "StuffGag": {
        "a": (3, "mh"), "o": (3, "mh"), "e": (3, "mh"), "i": (3, "m"),
        "u": (3, "mh"), "v": (3, "mh"), "er": (3, "mh"),
        "ai": (3, "mh"), "ei": (3, "mh"), "ao": (3, "mh"), "ou": (3, "mh"),
        "an": (3, "mh"), "en": (3, "mh"), "ang": (3, "mh"), "eng": (3, "mh"), "ong": (3, "mh"),
        "ia": (3, "mh"), "ie": (3, "mh"), "iao": (3, "mh"), "iu": (3, "mh"),
        "ian": (3, "mh"), "in": (3, "mh"), "iang": (3, "mh"), "ing": (3, "mh"), "iong": (3, "mh"),
        "ua": (3, "mh"), "uo": (3, "mh"), "uai": (3, "mh"), "ui": (3, "mh"),
        "uan": (3, "mh"), "un": (3, "mh"), "uang": (3, "mh"), "ueng": (3, "mh"),
        "ve": (3, "mh"), "van": (3, "mh"), "vn": (3, "mh"),
    },
    "HoodGag": {p: (3, "") for p in ZH_FINALS},
}

# ---------------------------------------------------------------------------
#  英文 IPA 音素表(取自 GagSpeak 使用的 en-US 音素集合)
# ---------------------------------------------------------------------------

EN_PHONEMES = [
    "p", "b", "m", "w", "f", "v", "θ", "ð", "t", "d", "s", "z", "n", "ɫ", "ɹ",
    "ʃ", "ʒ", "dʒ", "tʃ", "j", "k", "ɡ", "ŋ", "h",
    "eɪ", "ə", "ɔ", "æ", "i", "ɛ", "ɝ", "ɪə", "aɪ", "ɪ", "oʊ", "u", "ɑ", "ʊ",
    "aʊ", "ɔɪ",
]

EN_DEFAULT_SOUNDS = {
    "RingGag": {
        "p": (2, "w"), "b": (2, "w"), "m": (1, "m"), "w": (1, "w"), "f": (2, "h"), "v": (2, "w"),
        "θ": (2, "h"), "ð": (2, "h"), "t": (1, "t"), "d": (1, "d"), "s": (1, "s"), "z": (1, "z"),
        "n": (1, "n"), "ɫ": (1, "l"), "ɹ": (1, "r"), "ʃ": (2, "sh"), "ʒ": (2, "sh"),
        "dʒ": (2, "j"), "tʃ": (2, "ch"), "j": (0, "y"), "k": (1, "k"), "ɡ": (1, "g"),
        "ŋ": (1, "ng"), "h": (0, "h"),
        "eɪ": (1, "ay"), "ə": (0, "a"), "ɔ": (0, "aw"), "æ": (0, "a"), "i": (0, "ee"),
        "ɛ": (0, "eh"), "ɝ": (0, "er"), "ɪə": (0, "err"), "aɪ": (0, "y"), "ɪ": (0, "ih"),
        "oʊ": (0, "oh"), "u": (0, "oo"), "ɑ": (0, "ah"), "ʊ": (0, "oo"),
        "aʊ": (0, "ow"), "ɔɪ": (0, "oy"),
    },
    "BallGag": {
        "p": (3, "m"), "b": (3, "m"), "m": (2, "m"), "w": (2, "w"), "f": (3, "mh"), "v": (3, "m"),
        "θ": (3, "h"), "ð": (3, "h"), "t": (3, "n"), "d": (3, "n"), "s": (2, "ss"), "z": (3, "ss"),
        "n": (2, "n"), "ɫ": (3, "n"), "ɹ": (2, "r"), "ʃ": (3, "sh"), "ʒ": (3, "sh"),
        "dʒ": (3, "jh"), "tʃ": (3, "sh"), "j": (2, "y"), "k": (3, "ng"), "ɡ": (3, "ng"),
        "ŋ": (2, "ng"), "h": (2, ""),
        "eɪ": (2, "ay"), "ə": (2, "ah"), "ɔ": (2, "aw"), "æ": (2, "ah"), "i": (2, "ee"),
        "ɛ": (2, "eh"), "ɝ": (2, "er"), "ɪə": (2, "err"), "aɪ": (2, "y"), "ɪ": (2, "ih"),
        "oʊ": (2, "oh"), "u": (2, "oo"), "ɑ": (2, "ah"), "ʊ": (2, "oo"),
        "aʊ": (2, "ow"), "ɔɪ": (2, "oy"),
    },
    "PanelGag": {
        "p": (3, "m"), "b": (3, "m"), "m": (3, "m"), "w": (3, ""), "f": (3, "m"), "v": (3, "m"),
        "θ": (3, ""), "ð": (3, ""), "t": (3, "n"), "d": (3, "n"), "s": (3, "n"), "z": (3, "n"),
        "n": (3, "n"), "ɫ": (3, "n"), "ɹ": (3, "n"), "ʃ": (3, "n"), "ʒ": (3, "n"),
        "dʒ": (3, "n"), "tʃ": (3, "n"), "j": (3, ""), "k": (3, "ng"), "ɡ": (3, "ng"),
        "ŋ": (3, "ng"), "h": (3, ""),
        "eɪ": (3, "aym"), "ə": (3, "ahm"), "ɔ": (3, "awm"), "æ": (3, "ahm"), "i": (3, "eem"),
        "ɛ": (3, "ehm"), "ɝ": (3, "erm"), "ɪə": (3, "errm"), "aɪ": (3, "ym"), "ɪ": (3, "ihm"),
        "oʊ": (3, "ohm"), "u": (3, "oom"), "ɑ": (3, "ahm"), "ʊ": (3, "oom"),
        "aʊ": (3, "owm"), "ɔɪ": (3, "oym"),
    },
    "CleaveGag": {
        **{p: (3, "") for p in EN_PHONEMES},
        "m": (3, "m"), "n": (3, "n"), "ŋ": (3, "ng"),
    },
    "StuffGag": {
        **{p: (3, "") for p in EN_PHONEMES},
        "m": (3, "mh"), "n": (3, "n"), "ŋ": (3, "ng"),
    },
    "HoodGag": {p: (3, "") for p in EN_PHONEMES},
}

# 元音在 Cleave/Stuff 下需要一个可听见的"闷声", 否则整句全是空格。
for prof, vowel in (("CleaveGag", "m"), ("StuffGag", "mh")):
    for v in ("eɪ", "ə", "ɔ", "æ", "i", "ɛ", "ɝ", "ɪə", "aɪ", "ɪ", "oʊ", "u", "ɑ", "ʊ", "aʊ", "ɔɪ"):
        EN_DEFAULT_SOUNDS[prof][v] = (3, vowel)

# 未在表中出现的音素(生僻 IPA) 使用的兜底发音, 按档位区分。
FALLBACK_SOUNDS = {
    "RingGag":   ["h", "a", "e", "h", "w"],
    "BallGag":   ["m", "m", "h", "mh", "n"],
    "PanelGag":  ["m", "m", "n", "mh"],
    "CleaveGag": ["m", "m", "mh"],
    "StuffGag":  ["mh", "m", "n"],
    "HoodGag":   [""],
}

PROFILES = [
    {
        "id": "RingGag",
        "displayName": "开口环",
        "aliases": ["ring", "ring gag", "o ring", "开口器", "开口环", "张口"],
        "silent": False,
        "description": "嘴被撑开: 双唇音失效, 元音清楚。",
    },
    {
        "id": "BallGag",
        "displayName": "口球",
        "aliases": ["ball", "ballgag", "ball gag", "口球", "球塞"],
        "silent": False,
        "description": "口球顶住舌头: 塞音塌成鼻音, 元音被压扁。",
    },
    # 同一风格的"更狠"变体: 只写和基准档位不同的音素, 其余继承 BallGag。
    # 想再加一档 (比如 口球 XL 的加强版) 就再复制这段、改几处 sound 即可。
    {
        "id": "BallGagXL",
        "displayName": "大号口球",
        "aliases": ["ballgagxl", "ball gag xl", "ballxl", "球xl", "口球xl", "大号口球"],
        "silent": False,
        "base": "BallGag",
        "description": "更大号的口球: 比标准口球再闷一档, 元音全部糊住。",
        "fallbackSounds": ["m", "m", "mh", "n"],
        # 大号口球只改"共鸣位置": 声母/口型动作和标准口球一样, 但所有元音都顶到鼻腔 (ng)。
        "overrideZh": {
            "a": (3, "ahng"), "o": (3, "ohng"), "e": (3, "ehng"), "i": (3, "ihng"),
            "u": (3, "oong"), "v": (3, "yung"), "er": (3, "erng"),
            "ai": (3, "aing"), "ei": (3, "eing"), "ao": (3, "awng"), "ou": (3, "ohng"),
            "an": (3, "ahng"), "en": (3, "ehng"), "ang": (3, "ahng"), "eng": (3, "ehng"), "ong": (3, "ohng"),
            "ia": (3, "yahng"), "ie": (3, "yehng"), "iao": (3, "yawng"), "iu": (3, "yoong"),
            "ian": (3, "yahn"), "in": (3, "ihn"), "iang": (3, "yahng"), "ing": (3, "ihng"), "iong": (3, "yohng"),
            "ua": (3, "wahng"), "uo": (3, "wohng"), "uai": (3, "waing"), "ui": (3, "wayng"),
            "uan": (3, "wahn"), "un": (3, "wuhn"), "uang": (3, "wahng"), "ueng": (3, "wuhng"),
            "ve": (3, "ywehng"), "van": (3, "ywahn"), "vn": (3, "yuhn"),
        },
        "overrideEn": {
            "eɪ": (3, "ayng"), "ə": (3, "ahng"), "ɔ": (3, "awng"), "æ": (3, "ahng"),
            "i": (3, "eeng"), "ɛ": (3, "ehng"), "ɝ": (3, "erng"), "ɪə": (3, "errng"),
            "aɪ": (3, "yng"), "ɪ": (3, "ihng"), "oʊ": (3, "ohng"), "u": (3, "oong"),
            "ɑ": (3, "ahng"), "ʊ": (3, "oong"), "aʊ": (3, "owng"), "ɔɪ": (3, "oyng"),
        },
    },
    {
        "id": "PanelGag",
        "displayName": "面板口枷",
        "aliases": ["panel", "panel gag", "muzzle", "面板", "口枷", "口罩"],
        "silent": False,
        "description": "面板压住整张嘴: 只剩沉闷鼻音。",
    },
    {
        "id": "CleaveGag",
        "displayName": "布条封口",
        "aliases": ["cleave", "cleave gag", "tape", "布条", "胶带", "封口"],
        "silent": False,
        "description": "布条/胶带绕嘴: 声母全消失, 只剩 m 音。",
    },
    {
        "id": "StuffGag",
        "displayName": "填塞",
        "aliases": ["stuff", "stuffing", "sock", "panty", "填塞", "袜子", "内裤"],
        "silent": False,
        "description": "嘴里被塞满: 比封口更闷。",
    },
    {
        "id": "HoodGag",
        "displayName": "密封头套",
        "aliases": ["hood", "sealed", "silent", "头套", "密封", "无声"],
        "silent": True,
        "description": "完全听不见: 只会留下 …… / mnm 这类残响。",
    },
]

# ---------------------------------------------------------------------------
#  英文拼写 -> IPA 音素(按最长匹配, 顺序敏感)
# ---------------------------------------------------------------------------

EN_GRAPHEMES = [
    # 三字母以上
    ("tion", ["ʃ", "ə", "n"]), ("sion", ["ʒ", "ə", "n"]), ("ough", ["ɔ"]),
    ("augh", ["ɔ"]), ("eigh", ["eɪ"]), ("igh", ["aɪ"]),
    # 双字母
    ("tch", ["tʃ"]), ("dge", ["dʒ"]), ("sch", ["s", "k"]),
    ("ch", ["tʃ"]), ("sh", ["ʃ"]), ("th", ["θ"]), ("ph", ["f"]), ("wh", ["w"]),
    ("ck", ["k"]), ("ng", ["ŋ"]), ("qu", ["k", "w"]),
    ("ee", ["i"]), ("ea", ["i"]), ("oo", ["u"]), ("ou", ["aʊ"]), ("ow", ["aʊ"]),
    ("oa", ["oʊ"]), ("oi", ["ɔɪ"]), ("oy", ["ɔɪ"]), ("ai", ["eɪ"]), ("ay", ["eɪ"]),
    ("au", ["ɔ"]), ("aw", ["ɔ"]), ("ie", ["i"]), ("ei", ["eɪ"]), ("ey", ["eɪ"]),
    ("ar", ["ɑ", "ɹ"]), ("er", ["ɝ"]), ("ir", ["ɝ"]), ("ur", ["ɝ"]), ("or", ["ɔ", "ɹ"]),
    # 单字母
    ("a", ["æ"]), ("b", ["b"]), ("c", ["k"]), ("d", ["d"]), ("e", ["ɛ"]), ("f", ["f"]),
    ("g", ["ɡ"]), ("h", ["h"]), ("i", ["ɪ"]), ("j", ["dʒ"]), ("k", ["k"]), ("l", ["ɫ"]),
    ("m", ["m"]), ("n", ["n"]), ("o", ["ɔ"]), ("p", ["p"]), ("q", ["k"]), ("r", ["ɹ"]),
    ("s", ["s"]), ("t", ["t"]), ("u", ["ʊ"]), ("v", ["v"]), ("w", ["w"]), ("x", ["ks"]),
    ("y", ["j"]), ("z", ["z"]),
]

# 数字读法(中文用拼音音节, 英文用单词)
DIGIT_ZH = {"0": "ling", "1": "yi", "2": "er", "3": "san", "4": "si",
            "5": "wu", "6": "liu", "7": "qi", "8": "ba", "9": "jiu"}
DIGIT_EN = {"0": ["z", "ɪə", "ɹ", "oʊ"], "1": ["w", "ʊ", "n"], "2": ["t", "u"],
            "3": ["θ", "ɹ", "i"], "4": ["f", "ɔ", "ɹ"], "5": ["f", "aɪ", "v"],
            "6": ["s", "ɪ", "k", "s"], "7": ["s", "ɛ", "v", "ə", "n"],
            "8": ["eɪ", "t"], "9": ["n", "aɪ", "n"]}


def build_table():
    profiles_out = []
    for meta in PROFILES:
        pid = meta["id"]

        # 变体档位: 只写"和基准档位不一样"的音素, 其余从 base 继承(运行时合并)。
        if meta.get("base"):
            profiles_out.append({
                "id": pid,
                "base": meta["base"],
                "displayName": meta["displayName"],
                "aliases": meta["aliases"],
                "description": meta["description"],
                "silent": meta["silent"],
                "fallbackSounds": meta["fallbackSounds"],
                "zh": {k: {"muffle": m, "sound": s} for k, (m, s) in meta["overrideZh"].items()},
                "en": {k: {"muffle": m, "sound": s} for k, (m, s) in meta["overrideEn"].items()},
            })
            continue

        phonemes = {}
        for p, (muf, snd) in ZH_INITIAL_SOUNDS[pid].items():
            phonemes[p] = {"muffle": muf, "sound": snd}
        for p, (muf, snd) in ZH_FINAL_SOUNDS[pid].items():
            phonemes[p] = {"muffle": muf, "sound": snd}

        en = {}
        for p, (muf, snd) in EN_DEFAULT_SOUNDS[pid].items():
            en[p] = {"muffle": muf, "sound": snd}

        profiles_out.append({
            "id": pid,
            "displayName": meta["displayName"],
            "aliases": meta["aliases"],
            "description": meta["description"],
            "silent": meta["silent"],
            "fallbackSounds": FALLBACK_SOUNDS[pid],
            "zh": phonemes,
            "en": en,
        })

    return {
        "version": 1,
        "generator": "Source/MyBondageLife/Core/Garbler/Tools/generate_tables.py",
        "notes": [
            "zh: 中文拼音音素(声母 + 韵母)",
            "en: 英文 IPA 音素",
            "muffle: 0=不影响 1=轻 2=重 3=完全抑制; 多堵嘴同时佩戴时取 muffle 最大的 sound",
        ],
        "zhInitials": ZH_INITIALS,
        "zhFinals": ZH_FINALS,
        "enPhonemes": EN_PHONEMES,
        "profiles": profiles_out,
        "graphemesEn": [{"g": g, "p": p} for g, p in EN_GRAPHEMES],
        "digitsZh": DIGIT_ZH,
        "digitsEn": DIGIT_EN,
    }


def main():
    os.makedirs(DATA_DIR, exist_ok=True)
    table = build_table()
    path = os.path.join(DATA_DIR, "GarblerTables.json")
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(table, f, ensure_ascii=False, indent=1)
        f.write("\n")
    print("wrote", path, os.path.getsize(path), "bytes")
    print("profiles:", [p["id"] for p in table["profiles"]])
    print("zh phonemes:", len(table["profiles"][1]["zh"]), "en phonemes:", len(table["profiles"][1]["en"]))


if __name__ == "__main__":
    main()
