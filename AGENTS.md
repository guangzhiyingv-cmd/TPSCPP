# 仓库指南

## 项目结构与模块组织

这是一个 **TPS C++** Unreal Engine 5 项目，包含三种玩法变体。

```
Source/TPSCPP/               — 核心模块（Character, GameMode, PlayerController）
  Variant_Combat/            — 战斗变体（AI, Animation, Gameplay, Interfaces, UI）
  Variant_Platforming/       — 平台跳跃变体（Animation）
  Variant_SideScrolling/     — 横版卷轴变体（AI, Gameplay, Interfaces, UI）
Content/                     — 蓝图、地图、材质、角色、输入资源
Config/                      — 引擎、游戏、输入、编辑器配置（.ini）
Plugins/
  MultiplayerSessions/       — 多人联机插件（会话管理、菜单 UI、Steam OSS 封装）
  VisualStudioTools/          — VS 集成工具
```

每个变体拥有独立的 Character、GameMode 和 PlayerController。共享逻辑应放在核心 `TPSCPP` 模块中。

## 网络与多人游戏
多人联机功能已从核心 `TPSCPP` 模块解耦，移至 `Plugins/MultiplayerSessions` 插件中。

| 层级 | 组件 |
|------|------|
| **网络传输** | `OnlineSubsystemSteam` — `SteamNetDriver` / `SteamNetConnection` |
| **会话管理** | `MultiplayerSessionsSubsystem` — 创建 / 查找 / 加入 / 销毁 Steam 会话 |
| **菜单 UI** | `Menu` — 基于 UMG 的主菜单控件（托管在 `WBP_Menu` 蓝图资产中） |
| **后端服务** | Steam OSS（`DefaultEngine.ini` 中配置 `SteamDevAppId=480`） |
| **Lobby** | 核心模块中的 `ALobbyGameMode`（仅跟踪玩家进出计数） |

`ATPSCPPCharacter` 不再包含任何网络或 `OnlineSubsystem` 代码。所有网络逻辑均通过 `MultiplayerSessions` 插件处理，角色类仅关注输入、移动和摄像机。

详细网络配置位于 `Config/DefaultEngine.ini`：
- `[OnlineSubsystem]` — `DefaultPlatformService=Steam`
- `[OnlineSubsystemSteam]` — `bEnabled=true`, `SteamDevAppId=480`
- `[/Script/OnlineSubsystemSteam.SteamNetDriver]` — `NetConnectionClassName=/Script/OnlineSubsystemSteam.SteamNetConnection`

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

- **提交信息**：使用现在时祈使句（如 "Add crouch mechanic to combat character"）。适当时为受影响的变体或模块添加前缀（`[Combat]`、`[Platforming]`）。
- **拉取请求**必须包含：
  - 简洁的变更说明及原因
  - 关联 issue 链接（如有）
  - 视觉或行为变更的截图/GIF
  - 变更影响游戏性的验证步骤

## AI 代理专用说明

当此仓库由 AI 编码代理读取时：

- 修改或添加任何代码前，先向用户展示拟变更内容，待用户明确确认后方可执行。
- 所有代码与注释必须使用英文书写。
- 网络代码集中在 `Plugins/MultiplayerSessions/` 中；**不要**在 `TPSCPPCharacter` 或其他核心模块中添加网络相关逻辑。
- 优先使用项目现有模式与模块边界，而非引入新的抽象。
- 在充分阅读相关源文件之前，不要对架构或约定做出假设。
