# 武器数据、开火与换弹

[角色与战斗设计](../CombatDesign.md) / 武器

## 找到正在使用的武器数据

武器优先读 `WeaponDataRow` 指定的表，缺省表为 `/Game/Data/DT_Weapons`；未指定行名时，以武器类名去掉末尾 `_C` 查行。缺表或缺行会输出错误。表行管理弹匣容量、换弹时长、伤害、射击间隔、散布、后坐力、ADS、准星、投射物、动画及特效等配置。弹匣当前余量属于武器实例，备弹属于角色。核对某把枪的实际参数应读取它使用的表行。[武器数据表](../../Content/Data/DT_Weapons.uasset) · [行结构](../../Source/TPSCPP/Weapon/WeaponData.h) · [加载规则](../../Source/TPSCPP/Weapon/Weapon.cpp)

| 现有武器蓝图 | 原生射击路径 | 关键配置或判定 |
| --- | --- | --- |
| `BP_Pistol` | `AHitScanWeapon` | 从枪口做即时射线检测，比较 Visibility 与 SkeletalMesh 通道，取较近的命中并施加点伤害。 |
| `BP_Shotgun` | `AShotgunWeapon` | 准星中心不叠加普通随机偏移；服务器按弹丸数和当前锥角逐颗检测并结算，一次开火只播放一次枪械动画和抛壳。 |
| `BP_Assault_Rifle`、`BP_SniperRifle` | `AProjectileWeapon`；狙击枪经 `ASniperWeapon` 继承 | 服务器从 `MuzzleFlash` 生成配置的投射物，子弹碰撞时施加点伤害。狙击镜的本地表现沿用同一投射物路径。 |
| `BP_RocketLauncher` | `AProjectileWeapon` 搭配 `AProjectileRocket` | 火箭在碰撞点调用范围伤害，使用武器表的 `Damage` 和 `ExplosionRadius`。 |

蓝图和投射物资产位于[武器资产目录](../../Content/Blueprints/Weapon/)，各类的发射与命中逻辑在[武器源码](../../Source/TPSCPP/Weapon/)。

## 从按下开火到权威结算

1. 本地战斗组件按屏幕准星求目标点。普通武器可按当前准星散布偏移采样，连发由本地 `FireDelay` 定时器驱动，弹匣打空后尝试换弹。
2. 拥有者先播放开火表现和后坐力。非权威拥有者在 HUD 上预扣弹量，并发送 `ServerFire`。服务器取消进行中的换弹，尝试激活 `UGA_FireWeapon`；能力处理弹药消耗与射击冷却，时长通过 `Data.Cooldown` 传入。
3. 服务器在激活成功的分支多播开火。监听主机在多播路径执行，已经预测过表现的拥有者跳过重复播放。服务器执行射线或生成投射物；复制回来的弹匣数用于清除本地预扣并更新 HUD。[战斗组件](../../Source/TPSCPP/Components/CombatComponent.cpp) · [开火能力](../../Source/TPSCPP/AbilitySystem/Abilities/GA_FireWeapon.cpp)

换弹可由输入或空匣触发。满匣、已在换弹或有限弹药且无备弹时不启动；在 ADS 下先退出瞄准。拥有者可预播换弹蒙太奇。服务器能力持有 `State.Reloading`，按 `ReloadTime` 等待，完成时从备弹转入“缺口与剩余备弹的较小值”。无限弹药配置免除开火消耗，换弹完成时可直接补满。开火或掉落武器会取消进行中的换弹。[换弹能力](../../Source/TPSCPP/AbilitySystem/Abilities/GA_Reload.cpp) · [弹药消耗](../../Source/TPSCPP/AbilitySystem/Abilities/TPSCPPAbilityCost_WeaponAmmo.cpp)

## 按玩家看到的变化调参

| 现象 | 读取哪些字段 | 在哪里生效 |
| --- | --- | --- |
| 走动、跳跃时准星变大 | `VelocitySpreadMultiplier`、`AirborneSpreadBonus`、`SpreadInterpSpeed` | 取水平速度乘系数，再加插值后的滞空散布，落地后逐渐收回。 |
| 肩射或 ADS 仍然很散 | `ShoulderAimSpreadReduction`、`ADSAimSpreadReduction`、`HipfireMinSpread`、`ShoulderMinSpread` | 先减当前瞄准状态的收缩量，再施加该状态下限。ADS 的最终散布可降至零。HUD 和普通武器的屏幕目标采样使用同一散布值。 |
| 霰弹落点范围 | `PelletCount`、`PelletSpreadMaxAngleDegrees`、运行时当前锥角 | 每颗弹丸在枪口至准星中心的方向附近单独采样；表中的上限约束运行时锥角。 |
| 首发、持续连发表现 | `SingleShotHorizontalRecoil`、`SingleShotVerticalRecoil`、两条 `AutoRecoil…Curve` | 本地相机先取单发幅度，连续开火再按持续时间取曲线，之后叠加扰动。松开开火后按延迟、恢复速度和垂直恢复比例回正，基准是最后一发施加的俯仰。 |
| 射速与弹量 | `FireDelay`、`bAutomatic`、`MagCapacity`、`ReloadTime` | 本地定时器控制下一次请求；服务端 GE 的持续时间取 `max(FireDelay − CooldownTolerance, 0.01)` 秒。换弹能力用弹匣缺口和角色备弹计算装入量。 |

武器在 `PostInitializeComponents` 初次应用行数据，按容量初始化弹匣；`BeginPlay` 再读取表，使已加载的关卡实例取到当前配置，并保留已初始化的弹匣余量。网格、蒙太奇、曲线等可选资源仅在相应引用有效时覆盖已有设置。[数据应用](../../Source/TPSCPP/Weapon/Weapon.cpp) · [准星计算](../../Source/TPSCPP/Components/CombatComponent.cpp) · [后坐力](../../Source/TPSCPP/Character/TPSCPPCharacter.cpp) · [射击冷却](../../Source/TPSCPP/AbilitySystem/Abilities/GA_FireWeapon.cpp)

若武器表现正常但目标生命未变化，继续沿[伤害、复制与命中特效](Damage.md)检查。
