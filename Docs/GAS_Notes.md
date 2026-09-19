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
- 改动 `DefaultGame.ini` 中 `ConfigRestartRequired` 的项（如 cue manager、标签）后必须重启编辑器。
- 诊断 GAS 问题时，直接读 `Saved/Logs/TPSCPP*.log`（日志时间为 UTC，本地 = UTC+8）。
  `LogAbilitySystem` / `LogGameplayCueManager` 默认不会报告"cue tag 不存在"这类问题。
