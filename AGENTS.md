# 仓库指南

## 项目结构与模块组织

这是一个 **TPS C++** Unreal Engine 5 项目。

```
Source/
  TPSCPP/                     — 核心模块
    TPSCPP.Build.cs / .cpp / .h
    AbilitySystem/            — GAS 层（见下）
      TPSCPPAbilitySystemComponent.cpp / .h
      TPSCPPHealthSet.cpp / .h
      TPSCPPDamageEffect.cpp / .h
      TPSCPPFireCooldownEffect.cpp / .h
      TPSCPPGameplayTags.cpp / .h
      TPSCPPGameplayCueManager.cpp / .h
      TPSCPPGameplayCueTypes.h
      TPSCPPNativeGameplayCues.cpp / .h
      Abilities/
        TPSCPPGameplayAbility.cpp / .h
        TPSCPPAbilityCost.cpp / .h
        TPSCPPAbilityCost_WeaponAmmo.cpp / .h
        GA_Reload.cpp / .h
        GA_FireWeapon.cpp / .h
        GA_Sprint.cpp / .h
      GameplayCues/
        TPSCPPCueNotify_Blood.cpp / .h
        TPSCPPCueNotify_Impact.cpp / .h
    Animation/
      TPSCPPAnimInstance.cpp / .h
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
    PlayerState/
      TPSCPPPlayerState.cpp / .h
    Weapon/
      Casing.cpp / .h
      HitScanWeapon.cpp / .h
      Projectile.cpp / .h
      ProjectileBullet.cpp / .h
      ProjectileRocket.cpp / .h
      ProjectileWeapon.cpp / .h
      ShotgunWeapon.cpp / .h
      Weapon.cpp / .h
      WeaponData.h

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
  Data/                       — 数据表（DT_Weapons：武器数值，见「武器数值来自数据表」）
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
Docs/                         — 项目文档（GAS_Notes.md：GAS 迁移的坑位与验证过的做法）
Plugins/
  MultiplayerSessions/        — 多人联机插件（会话管理、菜单 UI、Steam Sockets 封装）
  VisualStudioTools/          — VS 集成工具
```

共享逻辑放在核心 `TPSCPP` 模块中。

## 项目目标
实现可通过steam远程联机的多人第三人称PVP射击游戏

## 当前游戏性功能

- 角色移动 / 跳跃 / 冲刺，以及 Hipfire / Shoulder / ADS 三种瞄准状态与平滑镜头过渡
- 武器拾取 / 装备 / 丢弃（`UCombatComponent`）；武器数值统一来自 `DT_Weapons`（`FWeaponData`），不再逐蓝图调参
- 射击：`AProjectileWeapon` 生成 `AProjectileBullet`，`AHitScanWeapon` 走瞬时命中，`AShotgunWeapon` 一次发射多颗弹丸（锥形散布，`PelletCount` / `PelletSpreadMaxAngleDegrees`）；命中角色后应用伤害；弹壳物理掉落
- 生命值：`Health`/`MaxHealth` 属性在 `UTPSCPPHealthSet`，伤害经 `UTPSCPPDamageEffect`（GE）应用；HUD 由属性变更委托驱动
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

## 游戏框架：Gameplay Ability System（GAS）

**本项目以 GAS 作为游戏逻辑框架**（UE 5.8，`GameplayAbilities` / `GameplayTags` / `GameplayTasks` 已启用）。
新增战斗、状态、表现类功能时，**优先用 Ability / GameplayEffect / GameplayCue / GameplayTag 表达**，
不要再新写手工状态机、手工计时器或直接 `SpawnEmitter`/`PlaySound` 的表现广播。

### 已迁移的内容

| 内容 | 承载 |
|---|---|
| 换弹 / 开火 / 冲刺 | `UGA_Reload` / `UGA_FireWeapon` / `UGA_Sprint` |
| 弹药消耗 | `UTPSCPPAbilityCost_WeaponAmmo`（自定义 Cost，经 `CommitAbility`） |
| 射速限制 | `UTPSCPPFireCooldownEffect`（SetByCaller 时长的冷却 GE） |
| 生命值 / 伤害 | `UTPSCPPHealthSet` + `UTPSCPPDamageEffect` |
| 命中 / 落点特效 | 角色命中只播放 `Cue.Hit.Blood`；环境命中播放 `Cue.Weapon.Impact`。命中点与运行时特效引用经多播批量下发 |
| 武器数值 | `FWeaponData` 数据表（`/Game/Data/DT_Weapons`）→ `AWeapon::ApplyWeaponData()` |
| 角色状态（换弹 / 瞄准 / 冲刺 / 开火） | `State.*` GameplayTag，驱动动画与能力门禁 |
| 动画状态 | `UTPSCPPAnimInstance` + `FGameplayTagBlueprintPropertyMap`（tag → `GameplayTag_Is*` 变量） |

### 架构约定

- **ASC 归属**：`ATPSCPPPlayerState` 持有 ASC（Owner），`ATPSCPPCharacter` 实现 `IAbilitySystemInterface` 并在 `PossessedBy` / `OnRep_PlayerState` 绑定 Avatar。
- **能力**：继承 `UTPSCPPGameplayAbility`（默认 `ServerOnly` + `InstancedPerActor`）。
  客户端不做 `LocalPredicted`，而是**手工乐观预测**：本地立即播表现/HUD，服务器仍是唯一权威，
  由复制值或服务器回执纠正（原因与做法见 `Docs/GAS_Notes.md` §6.1）。
  能力的激活前检查在客户端跑在 **CDO** 上——解析角色/武器必须用传入的 `ActorInfo`。
- **状态标签**：跨机器可见的状态标签必须显式选择复制方式：
  `ActivationOwnedTags`（配合 `ReplicateActivationOwnedTags=True`）或
  `ASC->SetLooseGameplayTagCount(Tag, Count, EGameplayTagReplicationState::TagOnly)`；
  **默认参数 `None` 完全不复制**，只会改本机计数。
- **表现**：统一走 GameplayCue。native C++ cue **不会**被自动注册，必须经
  `TPSCPPNativeGameplayCues` 加入 cue set（并在每次世界初始化后重注册）。
  命中点在**自己写的多播**里传给每台机器，实现内用
  `UGameplayCueManager::ExecuteGameplayCue_NonReplicated` 本地执行（细节见下）；
  多发弹药（霰弹枪）必须**一发一条 RPC 携带命中点数组和 `FTPSCPPCueImpactFX` 特效引用**，
  远程端用 `UTPSCPPCueImpactFXSource` 作为 cue 的 `SourceObject`。
- **武器数值来自数据表**：`FWeaponData`（`Weapon/WeaponData.h`）+ `AWeapon::ApplyWeaponData()`，
  默认表 `/Game/Data/DT_Weapons`，行名默认 = 类名（`BP_Shotgun_C` → `BP_Shotgun`）。
  运行时以**行值**为准：标量无条件覆盖，对象/类指针仅在行里配了才覆盖（否则保留蓝图值）；
  所以改数值请改表，蓝图里的同名字段只是兜底/参考（缺行时 `ApplyWeaponData` 会 `Error` 并全部走字段默认值）。
  `FireDelay` / `Damage` / `MagCapacity` / `ReloadTime` / `bAutomatic` 因此也是能力调参的唯一来源。
- **GE 用 C++ 类**，不新建蓝图 GE 资产；参数用 SetByCaller（`Data.Damage` / `Data.Cooldown`）。
- **动画**：不要在 Tick 里反射推属性；新增动画状态 = 加一个 tag + 在
  `UTPSCPPAnimInstance::AddDefaultMappings()` 里加一行映射（属性名对应 ABP 里的 `GameplayTag_Is*`）。

### 详细坑位

实际踩过的坑与验证过的写法记录在 **[`Docs/GAS_Notes.md`](Docs/GAS_Notes.md)**（含 cue 注册、tag 复制、
预测边界、`ServerOnly` 能力取消、hitscan vs 投射物等），新增 GAS 功能前**先读一遍**。要点：

- native C++ GameplayCue 类**不会被自动注册**，漏注册会**静默丢弃**（日志无提示）。
- **ability system 的 cue 多播是 Unreliable，且每个 net update 限流 `net.MaxRPCPerNetUpdate`（默认 2）条**；
  自己写多播时**不要**在实现里再调 `ASC->ExecuteGameplayCue`（会重复播放 + 被限流丢弃），
  改用 `UGameplayCueManager::ExecuteGameplayCue_NonReplicated(ASC->GetOwner(), Tag, Params)` 本地执行。
- **多发弹药（霰弹枪）必须批量**：一发一条 RPC 携带全部命中点（`FTPSCPPCueImpact` 数组），
  血液 cue 按受害者分组；逐弹丸发 cue 会导致客户端只看到前 2 个弹着点。
- 本地表现（相机/ADS）必须由发起端执行，不能在 `ServerOnly` 能力里做。
- 同帧 spawn+destroy 的对象只能用 **Reliable** 多播，且 cue 参数里不要依赖该 Actor 的弱指针；把武器表解析出的
  粒子/音效放进 `FTPSCPPCueImpactFX`，由 `UTPSCPPCueImpactFXSource` 传给 cue。
- `ServerOnly` 能力**无法被客户端取消**（`CancelAbilities` 跳过非激活 spec），需要 `Server_` RPC 转达。

## AI 代理专用说明

当此仓库由 AI 编码代理读取时：

- 修改或添加任何代码前，先向用户展示完整拟变更内容，待用户确认后方可执行（注意：仅修改或添加任何代码前需要确认，其它行为不用等待用户确认但是要告知用户正在做什么）。
- 所有代码与注释必须使用英文书写。
- 当用户要求编译时，如需修复错误则读取代码中的逻辑进行修复而不是按照之前对话中的逻辑，因为用户可能已经自己修改过代码。
- 优先使用项目现有模式与模块边界，而非引入新的抽象。
- 在充分阅读相关源文件之前，不要对架构或约定做出假设。
