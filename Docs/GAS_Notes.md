# GAS 迁移笔记（TPSCPP）

本文件记录将项目迁移到 Gameplay Ability System（UE 5.8）过程中实际踩过的坑与已验证的做法。
新增 GAS 相关功能前请先读一遍，避免重复踩坑。

参考实现：`E:\Unreal Projects\LyraStarterGame\Source\LyraGame`（Lyra），
引擎源码：`E:\UE5.8\UE_5.8\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities`。

---

## 1. GameplayCue

### 1.1 native C++ cue 不会被自动注册（关键）
`UGameplayCueManager::InitObjectLibrary()` 只通过 AssetRegistry 扫描 `GameplayCueNotifyPaths`
下的**蓝图资产**，**没有任何 native C++ 类的扫描分支**。因此直接继承
`UGameplayCueNotify_Static` / `AGameplayCueNotify_Actor` 的 C++ 类**永远不会**进入
`UGameplayCueSet`，`CueSet->HandleGameplayCue()` 查不到 tag 时会**静默丢弃**（日志无任何输出）。

已验证做法（本项目的 `TPSCPPNativeGameplayCues` + `TPSCPPGameplayCueManager`）：
- 把 native cue 类用 `UGameplayCueSet::AddCues(TArray<FGameplayCueReferencePair>)` 追加进表，
  并立刻把 `GameplayCueData[i].LoadedGameplayCueClass` 指向该类（native 类已在内存，不必走异步加载）。
- 注册时机：模块 `StartupModule()` 里 `InitGlobalData()` 之后注册一次，**并且**绑定
  `FWorldDelegates::OnPreWorldInitialization` 再注册一次——因为
  `ReloadObjectLibrary → RefreshObjectLibraries → InitObjectLibrary` 会 `Empty()` 整张表
  （注意 GAS 自己也绑了这个委托，所以在它之后绑定，顺序才对）。
- `UGameplayCueManager::InitObjectLibrary(FGameplayCueObjectLibrary&)` 是 **virtual**，
  子类可在 `Super` 之后补注册，覆盖"建表/重建表"的所有路径。
- 排查手法：在注册处/cue 处理处/cue 执行处各加一行临时日志；"注册失败"与"执行没到"症状相同（都没特效）。

### 1.2 手动 `ExecuteGameplayCue` 不会复制到客户端
5.8 的 `UAbilitySystemComponent` 已无 cue replication proxy，手动调用的 cue **只在本地执行**。
因此本项目保留原有的 `NetMulticast/Reliable` RPC，在**每台机器上本地执行一次**同一 cue
（每机恰好一次，不会重复）。将来做预测阶段时，可改为由 GE 授予 cue（`UGameplayEffect::GameplayCues`
+ effect context 携带命中点），届时这些多播 RPC 可整体删除。

### 1.3 Cue 参数里的弱指针 + 帧末 flush
`FGameplayCueParameters` 里的 `SourceObject` / `Instigator` / `EffectCauser` 都是弱指针，
而 cue 会被排队到**帧末**才 flush。同帧 spawn+destroy 的子弹（贴脸射击、高速弹）在客户端
此时已销毁 → 弱指针失效 → `SourceObject` 变成 `None`，cue 静默 return（表现为"服务端有特效、客户端没有"）。
已验证做法：传**类默认对象**（`GetClass()->GetDefaultObject()`）代替 Actor 实例——
CDO 随类常驻不会被 GC，而且蓝图里配的特效资产值就在 CDO 上，取值完全一致。
（长命对象如角色可以直接用 `MyTarget`，那来自执行 cue 的 ASC 的 Avatar，是有效的。）

### 1.4 5.8 的配置位置变了
`GlobalGameplayCueManagerClass` / `GameplayCueNotifyPaths` 已迁移到
`UGameplayAbilitiesDeveloperSettings`（`[/Script/GameplayAbilities.GameplayAbilitiesDeveloperSettings]`），
`UAbilitySystemGlobals` 上的是 5.5 起废弃的旧字段（仍有兼容 shim `PerformDeveloperSettingsUpgrade()`）。
本项目实测这份 ini 没有生效，因此最终**不依赖 ini**，改用 1.1 的代码注册（更可靠）。

### 1.5 cue 里读蓝图配置
native cue 若需要"每个蓝图各自配置"的资产（如子弹的 `HitParticles`、角色的 `BloodNiagaraSystem`），
不要把这些资产硬编码进 C++：通过 `Params.SourceObject`（或 `MyTarget`）取回对应对象再读其属性，
这样蓝图配置项名称与位置都不用变，也不需要重新指派资源。

---

## 2. GameplayAbility

### 2.1 `ServerOnly` 能力的"激活前检查"跑在 CDO 上
客户端不会实例化 `ServerOnly` 能力，`CanActivateAbility` / `CheckCost` 会在**类默认对象**上执行。
此时 `GetAvatarActorFromActorInfo()` 会触发 Ensure（"called on the CDO"）并返回空 → 客户端永远激活失败。
- 用 `GetCurrentActorInfo()`（CDO 上为 null，安全），或
- 直接用**传入的 `ActorInfo`** 参数（`ActorInfo->AvatarActor`）解析角色/武器。
本项目：`UTPSCPPGameplayAbility::GetTPSCPPCharacter()`、`GA_Reload::CanActivateAbility()`、
`UTPSCPPAbilityCost_WeaponAmmo` 都按此处理。

### 2.2 本地表现必须由"发起端"执行
相机切换、视角模型显隐等属于本地表现。`ServerOnly` 能力里调用 `DoADSEnd()` 之类的函数
只会作用于服务器，客户端会出现"状态退出了一半"（例：ADS 中换弹后仍是第一人称相机但准星已恢复）。
本项目做法：把退出 ADS 放在 `ATPSCPPCharacter::TryReload()`（发起换弹的那台机器）里，
能力内不再处理；并在本地先做前置检查（武器存在/弹匣未满/有备弹），避免"满匣按换弹也退 ADS"。

### 2.3 服务端冷却 vs 客户端节奏的边界
若客户端自己按 `FireDelay` 计时发请求，服务端冷却也设成 `FireDelay`，则第 N+1 发请求
正好落在冷却到期的同一瞬间（差值只有网络抖动）→ 会**随机丢枪**。
本项目做法：`UGA_FireWeapon::ApplyCooldown` 用 `FireDelay - CooldownTolerance`（默认 0.05s），
让服务端限速永远比客户端节奏宽松一点。

### 2.4 自定义 Cost
`AdditionalCosts`（`UTPSCPPAbilityCost`）在能力构造函数里用 `CreateDefaultSubobject` 建立
（`DefaultToInstanced` 的对象会被序列化）。Cost 的 `CheckCost`/`ApplyCost` 同样必须从传入的
`ActorInfo` 解析角色/武器（见 2.1），扣弹在服务器执行（`CommitAbility`）。

### 2.5 `ServerOnly` 能力无法被客户端取消
`UAbilitySystemComponent::CancelAbilities()` 会跳过 `!Spec.IsActive()` 的 spec，而 `ServerOnly`
能力在客户端**从不实例化** → 客户端调 `CancelAbilities` 静默无效，服务器上的能力会一直跑下去
（表现：客户端本地状态结束了，服务器状态没结束）。

本项目做法：客户端触发"结束"必须走 `Server_` RPC，让服务器去取消它正在运行的能力。
```cpp
void ATPSCPPCharacter::StopSprintAbility()
{
	if (!HasAuthority()) { Server_StopSprint(); return; }   // 客户端无法自行取消
	FGameplayTagContainer Tags; Tags.AddTag(TPSCPPGameplayTags::Ability_Sprint);
	GetAbilitySystemComponent()->CancelAbilities(&Tags);
}
```
取消后由该能力的 `EndAbility` 统一清理状态（tag / 移动参数），**不要在别处重复实现**。

对照：`CancelReloadAbility()` 一直正常，是因为它的调用点（`ServerFire`、`DropEquippedWeapon`）
本来就在服务器上；若将来从客户端路径调用它，同样会静默失效。
通用规律：**由服务器执行的能力，其结束也必须由服务器执行**。

---

## 3. GameplayEffect

### 3.1 授予标签要用 `UTargetTagsGameplayEffectComponent`
`UGameplayEffect::InheritableOwnedTagsContainer` 自 5.3 起废弃（`GetGrantedTags()` 读的是
`CachedGrantedTags`）。C++ 构造 GE 时：
```cpp
UTargetTagsGameplayEffectComponent* TagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("GrantedTags"));
FInheritedTagContainer TagChanges;
TagChanges.Added.AddTag(SomeTag);
TagsComponent->SetAndApplyTargetTagChanges(TagChanges);   // 会同步刷新 CachedGrantedTags
GEComponents.Add(TagsComponent);
```
（`SetAndApplyTargetTagChanges` 是必须的：仅 `GEComponents.Add` 不会刷新缓存标签。）

### 3.2 `ApplyCooldown` 必须重写才能传 SetByCaller
默认实现 `ApplyGameplayEffectToOwner(...CooldownGE...)` 用的是 CDO，无法带运行时数值。
本项目做法：重写后用 `MakeOutgoingGameplayEffectSpec(CooldownEffect->GetClass(), Level)` +
`SetSetByCallerMagnitude(Data.Cooldown, 武器 FireDelay)` + `ApplyGameplayEffectSpecToOwner(...)`；
时长在 GE 里用 `DurationMagnitude = FGameplayEffectModifierMagnitude(FSetByCallerFloat{ Data.Cooldown })`。

### 3.3 头文件
使用 `FGameplayEffectModCallbackData` 需要显式 `#include "GameplayEffectExtension.h"`
（`AttributeSet` 的 `PostGameplayEffectExecute` 常见坑）。

---

## 4. 网络

### 4.1 同帧 spawn + destroy 的对象要用 Reliable 多播
子弹命中即销毁时，**不可靠多播会被丢弃**（客户端什么都收不到，尤其贴脸射击）。
本项目：`AProjectile::MulticastExecuteImpactCue` 等一律 `NetMulticast, Reliable`，
在销毁前把表现数据（命中点/法线）发出去；不能依赖该 Actor 之后还存在于客户端。
配合 1.3，cue 需要的资产引用也要用 CDO 而不是 Actor 实例。

---

## 5. 工程 / 工具链

- **编译前必须关闭 Unreal 编辑器**（否则 DLL 被占用）；写法：
  `& "E:\UE5.8\UE_5.8\Engine\Build\BatchFiles\Build.bat" TPSCPPEditor Win64 Development -Project="E:\Unreal Projects\TPSCPP\TPSCPP\TPSCPP.uproject" -WaitMutex`
- 编辑器运行时锁定 `.uasset`，`git checkout` 会报 `unable to unlink ... Invalid argument`；
  切分支/合并前先关编辑器。
- `git push` 本机 schannel 吊销检查会失败（`CRYPT_E_NO_REVOCATION_CHECK`），需加
  `-c http.schannelCheckRevoke=false -c http.sslVerify=false`。
- 全局 `credential.https://github.com.helper` 指向了一个**不存在**的
  `E:\mod\re9\ai\.tools\bin\gh.exe`（另一项目的残留配置）→ 推送会报
  `failed to execute prompt script` / `could not read Username for 'https://github.com'`。
  本机 Windows 凭据管理器里已有 `git:https://github.com`，用 `git-credential-wincred` 覆盖即可：
  `git -c credential.https://github.com.helper= -c credential.https://github.com.helper=wincred -c http.schannelCheckRevoke=false -c http.sslVerify=false push -u origin <branch>`
  （彻底修复可编辑 `C:\Users\123\.gitconfig` 里那两行 `credential.https://github.com.helper=`）。
- 改动 `DefaultGame.ini` 中 `ConfigRestartRequired` 的项（如 cue manager、标签）后必须重启编辑器。
- 诊断 GAS 问题时，直接读 `Saved/Logs/TPSCPP*.log`（日志时间为 UTC，本地 = UTC+8）。
  `LogAbilitySystem` / `LogGameplayCueManager` 默认不会报告"cue tag 不存在"这类问题。

### 5.1 模拟高延迟 / 丢包（测预测行为必备）
**图形界面**：PIE 工具栏 **Play → Network Emulation → Custom**
（`ULevelEditorPlayNetworkEmulationSettings`）：Min/Max Latency、Packet Loss Percentage、
Emulation Target，可分别设 Outgoing / Incoming traffic。

**控制台**（在某个 PIE 窗口按 `~` 输入，只影响该实例的 NetDriver，可精确区分 server/client）：
```
Net PktLag=100              // 发送方向固定延迟(ms)，往返 ≈ 2×
Net PktLagVariance=25       // 抖动(±ms)，需要 PktLag 打开
Net PktLoss=2               // 发送方向丢包(%)
Net PktIncomingLagMin=100   // 接收方向延迟(ms)
Net PktIncomingLagMax=150
Net PktIncomingLoss=2       // 接收方向丢包(%)
Net PktFrameDelay=2         // 延迟 N 个 tick 再发送
Net GameNetDriverPktLoss=50 // 指定 NetDriver（默认就是 GameNetDriver）
```
互斥关系（`FPacketSimulationSettings`）：`PktOrder` 与 `PktDup`/`PktLag` 互斥；`PktLagMin/Max`
与 `PktLag` 二选一；`PktJitter` 会以**丢包**形式表现出来，不是纯延迟。

验证：`stat net` 看 Ping / 丢包。测预测功能（B-1/B-2）推荐：在客户端窗口设
`Net PktLag=150`（≈300ms RTT），确认拥有者表现**零延迟**、其他机器表现**只出现一次**。

---

## 6. 预测（阶段 B 记录）

### 6.1 本项目采用"手工乐观预测"，而不是 `LocalPredicted`
原因：开火请求携带命中点走的是 `ServerFire(bool, HitTarget)`。若把能力改成 `LocalPredicted`，
客户端的预测激活会**自己再发一条 GAS 激活 RPC**，服务器会因此先激活一次，随后 `ServerFire`
再触发一次（host 端尤其明显 = 一枪打两发）。要正确做必须把命中点改为**随能力激活传的
target data**（Lyra 的 `ServerSetReplicatedTargetData` + 等待目标数据模式），改动面很大。
当前需求（拥有者零延迟 + 服务器权威）用乐观预测就够。

统一模式（B-1 开火 / B-2 换弹 都用它）：
- 拥有者本地**立即**做"看得见的事"：弹药文本、HUD、枪动画、开火反应、换弹蒙太奇、`IsReloading()`
- 服务器仍是唯一权威：伤害、弹药真值、冷却、生成子弹、状态标签
- 预测的**退场依据**有两种，都要接上：
  1. **服务器复制值到达** → `OnRep_Ammo` 把 `PredictedAmmoCost` 归零；
     `RegisterGameplayTagEvent(State_Reloading, NewOrRemoved)` 清掉本地预测标志
  2. **服务器回执**（被拒绝时）→ 见 6.2
- **不要直接改复制值来做预测**：例如直接 `AWeapon::Ammo--`，服务器拒绝时不会回滚，会**长期差 1 发**。
  B-1 的做法是维护 `PredictedAmmoCost` 偏移，只影响显示与本地判定（`GetPredictedAmmo()`）。
- **表现不要双播**：拥有者预测过的表现，服务器多播要**跳过射手机器**
  （角色上 `!HasAuthority() && IsLocallyControlled()`；`UActorComponent` 没有 `HasAuthority()`，
  用 `GetOwner()->HasAuthority()`）。但"停止"类事件（停枪动画/停换弹蒙太奇）**不能**跳过拥有者。

### 6.2 服务器拒绝激活**不会**通知客户端
`UAbilitySystemComponent::ClientActivateAbilityFailed_Implementation` 只标记 prediction key 被拒绝、
结束本地实例，**不广播 `AbilityFailedCallbacks`**（5.8 实测）。因此：
- 想让拥有者撤销预测，必须自己发回执（本项目：`GA_Reload` → `Client_StopReloadPresentation()`）
- 校验必须放在 **`ActivateAbility`** 而不是 `CanActivateAbility`：后者被拒时不会进入我们的代码，
  回执就发不出去（`ActivateAbility` 里做 `bCanReload` 判定 + 立即 `EndAbility(cancelled)`）

### 6.3 `ActivationOwnedTags` 与 `ActivationBlockedTags`
- `ActivationOwnedTags` 在激活时以 `AddLooseGameplayTags(..., EGameplayTagReplicationState::CountToOwner)`
  应用，前提是 `UGameplayAbilitiesDeveloperSettings::ReplicateActivationOwnedTags=True`
  （`[/Script/GameplayAbilities.GameplayAbilitiesDeveloperSettings]`，`ConfigRestartRequired`）。
  `CountToOwner` 语义 = **标签复制给所有人、计数只给拥有者** → 远端也能看到该状态标签 ✓
  （本项目用它替代手工 `SetLooseGameplayTagCount` 管理 `State.Reloading`）。
- `ActivationBlockedTags` 可表达"同类能力不能叠加"：本项目用它防止急速双击 R 激活两个换弹实例。

### 6.4 hitscan 可预测，投射物不可预测（本次结论：暂不做命中特效预测）
- **hitscan**：命中点由客户端准星决定，客户端 trace 与服务器 trace 用同一套参数即可预测。
- **投射物**：落点在服务器模拟（`ProjectileMovementComponent` 服务器权威），客户端拿不到；
  要预测得在客户端预演弹道（`UGameplayStatics::PredictProjectilePath`，注意 `ProjectileGravityScale`、
  目标移动会造成误差）或改成客户端子弹。
- 本项目武器类映射（容易踩坑）：`BP_Pistol` → `AHitScanWeapon`；
  `BP_Assault_Rifle` / `BP_RocketLauncher` → `AProjectileWeapon`（弹丸初速 15000 cm/s，带重力）。
  **主用武器是投射物**，所以针对 hitscan 的预测在实战里几乎没有效果。
- 曾尝试"hitscan 本地 trace 预判命中 + 服务器多播跳过射手机器（`bSkipInstigatorMachine`）"，
  已**回退**（未实战验证）。命中反馈目前由服务器 cue 下发，射手端晚约 1 个 RTT。
  若将来要做：hitscan 走共用 trace + 跳过射手机器；投射物需要弹道预演。

