# 堵嘴文本混淆器（Garbler）

把正常聊天文本「按堵嘴档位」转换成听不清的混淆发音。算法源自 GagSpeak 的 MufflerCore
（音素级替换 + 多堵嘴竞争 + 兜底音节），中文部分改用**拼音音素**实现。

```
原文「你好，今天天气不错。」
   │
   ├─ 分词：汉字逐字 / 英文按拼写规则 / 标点与空格原样保留
   │
   ├─ 汉字 → pinyin「ni」「hao」 → 声母 + 韵母 [n][i] [h][ao]
   ├─ 英文 → IPA                「hello」→ [h][ɛ][ɫ][oʊ]
   │
   ├─ 每个音素按当前堵嘴档位查表 → 取 muffle 最大的那一档的替换发音
   │
   └─ 拼回： 「nihaw，shihnnyahnnyahnshihmooswoh。」（口球）
```

---

## 一、音素表在哪

```
Source/MyBondageLife/Core/Garbler/
├── Data/
│   ├── GarblerTables.json     ← 音素表（就是你要看/要调的那张表）
│   └── HanziPinyin.json       ← 汉字 → 无声调拼音（6763 个 GB2312 常用字）
├── Tools/
│   ├── generate_tables.py     ← 音素表生成器（改这里再重新生成，或直接改 JSON）
│   ├── preview_garbler.py     ← 命令行试听，不启动 UE 就能看效果
│   └── gen_embedded.py        ← 把 JSON 内嵌成 C++（打包兜底用）
├── MyBondageLifeGarbler.*             ← 纯 C++ 核心
├── MyBondageLifeGarblerSubsystem.*    ← 蓝图接口（GameInstanceSubsystem）
├── MyBondageLifeGarblerLibrary.*      ← 蓝图静态工具函数
└── MyBondageLifeGarblerEmbedded.*     ← 自动生成，别手改
```

运行时要覆盖表而不改仓库，也可以把同名文件放到：

```
Config/Garbler/GarblerTables.json      ← 优先级最高
Config/Garbler/HanziPinyin.json
```

加载优先级：`Config/Garbler/` → `Source/.../Garbler/Data/` → `Content/GarblerData/` → 内置副本。

---

## 二、`GarblerTables.json` 结构

```jsonc
{
  "zhInitials": ["b","p","m", ...],       // 声母表（最长匹配用）
  "zhFinals":   ["a","o","e","ang", ...], // 韵母表
  "enPhonemes": ["p","b","m", ...],       // 英文 IPA 音素表

  "profiles": [                            // ← 堵嘴档位，一个档位一张表
    {
      "id": "BallGag",                     // 运行时用的 id
      "displayName": "口球",
      "aliases": ["ball", "口球", "球塞"],  // 蓝图里可以直接传中文名
      "silent": false,                     // true = 完全无声（只剩 ……）
      "fallbackSounds": ["m","m","h","mh"],// 表里没收录的音素用哪个
      "zh": {                              // 中文音素表
        "b": { "muffle": 3, "sound": "m" },
        "a": { "muffle": 2, "sound": "ah" }
      },
      "en": {                              // 英文音素表
        "p": { "muffle": 3, "sound": "m" }
      }
    }
  ],

  "graphemesEn": [                         // 英文拼写 → IPA（最长匹配）
    { "g": "tion", "p": ["ʃ","ə","n"] }
  ],
  "digitsZh": { "1": "yi", "2": "er" }     // 数字读法（中文）
}
```

### 两个字段的含义

| 字段 | 含义 |
|---|---|
| `muffle` | **抑制强度**，0=不影响 1=轻 2=重 3=完全抑制。它不是音量，而是**竞争权重**：同时戴两个堵嘴时，同一个音素取 `muffle` 更大的那一档的 `sound` |
| `sound` | 这个音素被替换成的字符串。可以是 `""`（被吞掉），也可以是多字符（`"ah"`、`"mh"`、`"yahn"`） |

`muffle` 打平时，由 `profiles` 数组里的先后顺序裁决（靠前的赢），所以结果是稳定的。

### 内置的 7 个档位

| id | 名称 | 听感 | 「你好世界」 |
|---|---|---|---|
| `RingGag` | 开口环 | 双唇音失效、元音清楚，最轻 | `nihhawshihshyeh` |
| `BallGag` | 口球 | 塞音塌成鼻音、元音被压扁（默认推荐） | `nihawshihshyeh` |
| `BallGagXL` | 大号口球 | 与口球同样的辅音，元音全部顶到鼻腔 | `nihngawngshihngshyehng` |
| `PanelGag` | 面板口枷 | 元音被中和并带上 m 尾巴 | `nihmaumnihmnyehm` |
| `CleaveGag` | 布条封口 | 声母全消失，只剩 m | `nmmmm` |
| `StuffGag` | 填塞 | 比封口更闷、带气声 | `nmmhmmh` |
| `HoodGag` | 密封头套 | `silent: true`，只剩 `……` / `m..` | `m....` |

### 档位继承（做"同款但更狠"的变体）

变体档位**只写和基准档位不同的音素**，其余在加载时从 `base` 补齐：

```jsonc
{
  "id": "BallGagXL",
  "base": "BallGag",              // 继承 BallGag 的全部 99 条
  "displayName": "大号口球",
  "aliases": ["球xl", "大号口球"],
  "silent": false,
  "fallbackSounds": ["m", "m", "mh", "n"],
  "zh": { "a": {"muffle":3,"sound":"ahng"}, "i": {"muffle":3,"sound":"ihng"} },  // 只写差异
  "en": { "æ": {"muffle":3,"sound":"ahng"} }
}
```

所以再加一档（比如「口球 XL 加强版」）只需要复制这 10 行、改几处 `sound`，不用再写 99 条。
支持多层继承（A ← B ← C），也支持 `base` 写在变体后面，加载时会自动排序合并。

> 注意：`base` 只补**缺失**的音素，变体自己写的条目永远优先。

### 同一时间只戴一件时，配置就是一列"档位名"

```
DT_Restraint 行 "Ballgag"    -> BallGag
另一行        "Ballgag_XL"  -> BallGagXL
第三行        "RingGag_01"  -> RingGag
```

`muffle` 完全不用你填：它只用于「两件以上同时戴、争同一个音素」时决定谁更狠。
单件生效时，决定听感的只有该档位的 `sound`。

---

## 三、怎么调

### 1) 不启动 UE 试听（最快）

```powershell
python Source/MyBondageLife/Core/Garbler/Tools/preview_garbler.py
```

它会打印每个档位对若干例句的处理结果。改 `GarblerTables.json` 后重跑即可。

### 2) 在编辑器里试

启动后按 `~` 打开控制台：

```
mbl.garbler.test BallGag 你好世界
mbl.garbler.test BallGag+CleaveGag 我的嘴里被塞了东西     # 多个堵嘴用 + 连接
mbl.garbler.tables                  # 列出档位 + 当前数据目录
mbl.garbler.reload                  # 改完 json 后热重载
```

> 控制台里逗号会被 `ExecCmds` 当分隔符，所以多档位请用 `+`；蓝图接口收的是数组，没有这个限制。

`test` 会同时打印**原文 / 中间音素 / 混淆结果**，例如：

```
[Garbler] 音素: 你[ni=n-i] 好[hao=h-ao] 世[shi=sh-i] 界[jie=j-ie]
[Garbler] 混淆: nihawshihshyeh
```

### 已实测的效果（seed 固定，编辑器中实际跑过）

| 档位 | 原文 | 混淆 |
|---|---|---|
| 口球 | 你好，今天天气不错。 | `nihaw，shihnnyahnnyahnshihmooswoh。` |
| 开口环 | 别说话，我马上就回来。 | `wyehshwohhwah，wohmahshahngshyoohwaylai。` |
| 面板口枷 | 求求你放开我…… | `nyoomnyoomnihmmahngngaimohm……` |
| 布条封口 | 我嘴里被塞了东西 | `mmmmmmmm` |
| 填塞 | Hello world this is a test | `Mhmh mh mh mh mh mh` |
| 密封头套 | 我的QQ是12345 | `h..m..h.` |
| 口球+布条（取更狠） | 你好世界 | `nmmmshm` |
| 口球 | `*挣扎* 放开我 *喘气*` | `*挣扎* mhahngngaiwoh *喘气*` |
| 口球 | `:smile: 你好` | `:smile: nihaw` |

### 3) 重新生成表

改 `Tools/generate_tables.py` 里的映射（推荐，结构清晰），然后：

```powershell
python Source/MyBondageLife/Core/Garbler/Tools/generate_tables.py
python Source/MyBondageLife/Core/Garbler/Tools/gen_embedded.py   # 打包兜底副本
```

> 只改 JSON 也能用，但记得同步跑一次 `gen_embedded.py`，否则打包后的版本还是旧数据。

---

## 四、代码里怎么用

### C++（任意位置，不需要 UObject）

```cpp
#include "Core/Garbler/MyBondageLifeGarbler.h"

FMyBondageLifeGarblerOptions Options;
const FString 混淆 = FMyBondageLifeGarbler::Garble(TEXT("你好，今天天气不错。"),
                                                   { FName("BallGag") }, Options);
```

### 蓝图

1. `Get Game Instance` → `Get Subsystem (MyBondageLifeGarblerSubsystem)`
2. 戴/摘堵嘴时：`Set Gag Active (BallGag, true)` / `Clear Active Gags`
3. 发消息前：`Garble Text`，得到的字符串直接当正文用
4. 只想试算不改状态：`Garble Text Once`（静态节点）

主要接口：

| 函数 | 说明 |
|---|---|
| `GarbleText(Text)` | 按当前佩戴的堵嘴混淆；没戴时原样返回 |
| `GarbleTextWith(Text, Gags, Options)` | 一次性，不改状态 |
| `SetActiveGags` / `SetGagActive` / `ClearActiveGags` | 维护佩戴列表（可传 `"口球"` 这种中文名） |
| `GetActiveGags` / `IsGagged` / `GetAvailableGags` | 查询 |
| `OnActiveGagsChanged` | 状态变化广播，方便 UI / 音效跟随 |
| `ToPhonemes(Text)` | 调试：看中间音素 |
| `ReloadTables()` / `GetDataDirectory()` | 热重载 / 查表路径 |

### 保护规则

- `*挣扎*` 这类 `*...*` 段落默认**不混淆**（`bPreserveRoleplaySegments`）。
- `:emoji:` 形式的表情默认不混淆（`bPreserveEmotes`）。
- 标点、空格、换行原样保留。
- 英文大小写会被保留（全大写仍然全大写）。

---

## 五、打包注意

`Data/*.json` 是普通文件，不会自动进包。两种做法：

1. **什么都不做**：打包版本读不到外置 JSON 时会退回 `MyBondageLifeGarblerEmbedded.cpp` 里内嵌的那份，功能正常，只是改表要重新编译。
2. **想打包后还能改表**：在 `Config/DefaultGame.ini` 里加

```ini
[/Script/UnrealEd.ProjectPackagingSettings]
+DirectoriesToAlwaysStageAsNonUFS=(Path="Garbler")
```

然后把 `GarblerTables.json` / `HanziPinyin.json` 放到 `Config/Garbler/`（`Config` 目录本身会被打进包）。

---

## 六、已经接入游戏的地方

聊天发送链路（服务端权威，所有客户端看到同一份文本）：

```
玩家按回车提交
  └─ UMyBondageLifeChatWidget::OnMessageSubmitted
       └─ AMyBondageLifePlayerController::ServerSendChatMessage(原文)      // Server RPC
            ├─ GetPawn() -> AMyBondageLifeCharacter
            ├─ GetGagActive()          // = bRestraintInGag：任一穿戴件带 InGag? 就为 true
            ├─ GetGagProfileId()       // 扫 RestraintItemIds，找到带 InGag 的那件
            │     └─ FMyBondageLifeGagTypeMap::ToProfileId(GagType)
            │           ├─ 优先用 Enum_GagType 的显示名匹配档位（"口球" -> BallGag）
            │           └─ 显示名对不上时按顺序 0..5 兜底
            ├─ FMyBondageLifeGarbler::Garble(原文, {档位}, Options)
            └─ ClientReceiveChatMessage(混淆后的文本)  -> 所有人（含自己）
```

| 文件 | 作用 |
|---|---|
| `Core/MyBondageLifeRestraintCatalog.*` | 从 `DT_Restraint` 读 `GagType`（旧表没有这一列也能跑） |
| `Core/Garbler/MyBondageLifeGagTypeMap.*` | `Enum_GagType` -> 档位 id |
| `MyBondageLifeCharacter::GetGagProfileId()` | 当前穿戴里哪件算堵嘴、对应哪个档位 |
| `MyBondageLifePlayerController::ServerSendChatMessage_Implementation` | 真正调用 `Garble` 的地方 |

### 几点约定

- **强度只由 `GagType` 决定。** `ST_Restraint` 里没有 `Muffle` 字段，同一档位内的听感是固定的；
  想调强弱请直接换档位，或改该档位的 `sound`。
- **旧数据兜底**：口部道具没配 `GagType`（或表里根本没这列）时按默认 `BallGag` 处理，
  避免「戴着却还能正常说话」。
- **`GagType` 的 0 是合法值**（0 = 开口环），所以不能用 0 表示「未设置」——判断依据是这一列**存不存在**。
- 混淆只发生在**发送时**：输入框里仍是原文，聊天记录里是混淆后的文本。
