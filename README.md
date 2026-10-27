# MyBondageLife

Unreal Engine **5.8** 项目的 C++ 源码仓库。

> 本仓库**只保存代码**（`Source/` 目录）。`Content/`、`Intermediate/`、`Binaries/`、
> `Saved/`、`Packaged/` 等资源与构建产物不上传，详见 `.gitignore`。

## 目录结构

```
Source/
├── MyBondageLife.Target.cs              # 游戏运行时 Target
├── MyBondageLifeEditor.Target.cs        # 编辑器 Target
└── MyBondageLife/
    ├── MyBondageLife.Build.cs           # 模块构建规则
    ├── MyBondageLife*.h / .cpp          # 模块入口、GameMode、PlayerController 等
    ├── Core/                            # 账号 / 会话 / EOS / 服务器配置子系统
    ├── Network/                         # 房间、会话等网络子系统
    ├── UI/                              # UMG 界面（登录、主场景、好友、玩家信息等）
    ├── Variant_Combat/                  # 战斗玩法模块（AI、动画、Gameplay、UI）
    ├── Variant_Platforming/             # 平台跳跃玩法模块
    └── Variant_SideScrolling/           # 横版卷轴玩法模块
```

## 构建

1. 安装 Unreal Engine 5.8 与 Visual Studio 2022（含「使用 C++ 的游戏开发」工作负载）。
2. 右键 `MyBondageLife.uproject` → **Generate Visual Studio project files**。
3. 打开 `MyBondageLife.sln`，以 `Development Editor` 配置编译 `MyBondageLife` 目标。
4. 双击 `MyBondageLife.uproject` 启动编辑器。

命令行编译示例（请替换为自己机器上的引擎路径）：

```bat
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" ^
  MyBondageLifeEditor Win64 Development ^
  -Project="F:\UE_Project\MyBondageLife\MyBondageLife.uproject" -WaitMutex
```

## 仓库约定

- 只提交 `Source/` 下的代码；资源与中间产物不纳入版本控制。
- 提交信息使用中文简述改动内容。
