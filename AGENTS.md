# 仓库指南

## 项目结构与模块组织

这是一个 **TPS C++** Unreal Engine 5 项目。

```
Source/
  TPSCPP/                     — 核心模块
    TPSCPP.Build.cs / .cpp / .h
    Character/
      TPSCPPCharacter.cpp / .h
    Components/
      CombatComponent.cpp / .h
    GameMode/
      TPSCPPGameMode.cpp / .h
      LobbyGameMode.cpp / .h
    HUD/
      CharacterOverlay.cpp / .h
      PlayerHUD.cpp / .h
      WeaponHeaderWidget.cpp / .h
    PlayerController/
      TPSCPPPlayerController.cpp / .h
    Weapon/
      Casing.cpp / .h
      Projectile.cpp / .h
      ProjectileBullet.cpp / .h
      ProjectileWeapon.cpp / .h
      Weapon.cpp / .h

Content/                      — 蓝图、地图、材质、角色、输入资源、特效、武器、声音
  Assets/                     — 统一素材目录
    LevelPrototyping/         — 关卡原型物体（门/Door、弹跳板/JumpPad、标靶/Target）
    Materials/                — 全局材质（M_Eye, M_NPR, M_Outline）
    Sound/                    — 枪械/受击/弹壳音频（Cue + Wav）
    Textures/                 — 准星等纹理（Crosshairs/Primary）
  Blueprints/                 — 蓝图资产
    GameModes/                — 游戏模式蓝图（BP_LobbyGameMode 等）
    HUD/                      — WBP_CharacterOverlay、BP_PlayerHUD
    Weapon/                   — 武器/子弹/弹壳蓝图（Projectile/ 子目录）
  Characters/                 — BP_TPSCharacter, Iris 角色模型, Mannequins（动画/材质/网格/绑定/纹理）
  Collections/                — UE 辅助目录
  Developers/                 — UE 辅助目录
  Input/                      — 输入映射上下文 IMC_Default / IMC_MouseLook、IA_* 输入动作、触摸控件
  LevelPrototyping/           — 关卡原型物体（门/Door、弹跳板/JumpPad、标靶/Target）
  Maps/                       — GameLevel.umap（对战地图）、Lobby.umap（大厅）
  MilitaryWeapSilver/         — MilitaryWeapSilver 武器素材包
  ThirdPerson/                — Lvl_ThirdPerson.umap + BP_ThirdPersonCharacter/GameMode/PlayerController
  __ExternalActors__/         — UE 自动生成
  __ExternalObjects__/        — UE 自动生成

Config/                       — 引擎、游戏、输入、编辑器配置（.ini）
Plugins/
  MultiplayerSessions/        — 多人联机插件（会话管理、菜单 UI、Steam Sockets 封装）
  VisualStudioTools/          — VS 集成工具
```

共享逻辑放在核心 `TPSCPP` 模块中。

## 项目目标
实现可通过steam远程联机的多人第三人称PVP射击游戏

## 当前游戏性功能

- 角色移动 / 跳跃 / 冲刺，以及 Hipfire / Shoulder / ADS 三种瞄准状态与平滑镜头过渡
- 武器拾取 / 装备 / 丢弃（`UCombatComponent`），射速、全自动、伤害、ADS 等参数在蓝图可配置
- 射击：`AProjectileWeapon` 生成 `AProjectileBullet`，命中角色后应用伤害；弹壳物理掉落
- 生命值：`Health` 复制到所有客户端，`OnRep_Health` 更新 HUD 血量条（`UCharacterOverlay`，支持曲线插值）
- 淘汰：血量归零后 `ATPSCPPGameMode::PlayerEliminated` 触发多播 `Elim()`：关闭碰撞、掉落武器、mesh 布娃娃，5 秒后销毁 Actor

## 网络与多人游戏

游戏使用listen模式，需要着重注意网络同步时：逻辑既要能在客户端跑通又要能在同时作为客户端的服务器端跑通!

多人联机功能已从核心 `TPSCPP` 模块解耦，移至 `Plugins/MultiplayerSessions` 插件中。

| 层级 | 组件 |
|------|------|
| **网络传输** | `SteamSocketsNetDriver` / `SteamSocketsNetConnection`（Steam Sockets 协议） |
| **会话管理** | `MultiplayerSessionsSubsystem` — 创建 / 查找 / 加入 / 销毁 Steam 会话 |
| **菜单 UI** | `Menu` — 基于 UMG 的主菜单控件（托管在 `WBP_Menu` 蓝图资产中） |
| **后端服务** | Steam OSS（`DefaultEngine.ini` 中配置 `SteamDevAppId=480`） |
| **Lobby** | 核心模块中的 `ALobbyGameMode`（跟踪玩家进出计数，人数达到 `MinPlayersToStart` 后 SeamlessTravel 到 GameLevel） |

详细网络配置位于 `Config/DefaultEngine.ini`：
- `[OnlineSubsystem]` — `DefaultPlatformService=Steam`
- `[OnlineSubsystemSteam]` — `bEnabled=true`, `SteamDevAppId=480`
- `[/Script/SteamSockets.SteamSocketsNetDriver]` — `NetConnectionClassName=/Script/SteamSockets.SteamSocketsNetConnection`

## 关卡地图

| 地图 | 说明 |
|------|------|
| `/Game/ThirdPerson/Lvl_ThirdPerson` | 基础第三人称关卡（单人测试） |
| `/Game/Maps/Lobby` | 多人大厅地图（人数达标后自动跳转 GameLevel） |
| `/Game/Maps/GameLevel` | 实际对战地图（当前 PIE 测试主地图） |


## 构建、测试与开发命令

| 命令 | 说明 |
|------|------|
| 右键 `.uproject` → **Generate Visual Studio project files** | 增删模块或源文件后重新生成解决方案文件 |
| 打开 `TPSCPP.sln`，按 **Ctrl+Shift+B** | 从源码编译整个项目 |
| 编辑器中按 **Ctrl+F5** | 在当前地图启动 PIE（Play In Editor） |
| 运行 `Automation_TPSCPP.sln` 测试 | 执行 Unreal Automation Framework 测试（如有配置） |

这是一个纯 C++ 项目，主要运行时未使用脚本语言（Python, JS 等）。

## 编码风格与命名规范

- **缩进**：遵循 `.editorconfig` 规则。默认使用 Unreal Engine 风格（代码用制表符，对齐用空格）。
- **命名**：通过 `.editorconfig` 强制执行 Unreal Engine 命名规范：
  - 类前缀 `A`（Actor）、`U`（UObject）、`S`（SWidget）
  - 布尔变量前缀 `b`（如 `bIsDead`）
  - 结构体与枚举遵循 UE 命名风格
- **所有代码与注释必须使用英文书写**。
- 未配置外部 linter 或格式化工具，请依赖 EditorConfig 与 UE 编码标准。

## 测试指南

- 测试使用 **Unreal Automation Framework**（`FAutomationTestBase`）进行（如有配置）。
- 测试类应命名为 `T<ModuleName><Feature>Test`。
- 通过 Session Frontend（Tools → Session Frontend → Automation 标签页）或 `Automation_TPSCPP.sln` 运行测试。
- 通过 **Play In Editor (PIE)** 进行手动游戏性测试。

## 提交与拉取请求规范

- **提交信息**：使用现在时祈使句（如 "Add crouch mechanic to combat character"）。适当时为受影响的模块添加前缀（如 `[MultiplayerSessions]`）。
- **拉取请求**必须包含：
  - 简洁的变更说明及原因
  - 关联 issue 链接（如有）
  - 视觉或行为变更的截图/GIF
  - 变更影响游戏性的验证步骤

## AI 代理专用说明

当此仓库由 AI 编码代理读取时：

- 修改或添加任何代码前，先向用户展示完整拟变更内容，待用户确认后方可执行（注意：仅修改或添加任何代码前需要确认，其它行为不用等待用户确认但是要告知用户正在做什么）。
- 所有代码与注释必须使用英文书写。
- 当用户要求编译时，如需修复错误则读取代码中的逻辑进行修复而不是按照之前对话中的逻辑，因为用户可能已经自己修改过代码。
- 优先使用项目现有模式与模块边界，而非引入新的抽象。
- 在充分阅读相关源文件之前，不要对架构或约定做出假设。
