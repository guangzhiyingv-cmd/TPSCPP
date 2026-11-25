# GAS、伤害复制与命中特效

[角色与战斗设计](../CombatDesign.md) / 伤害与表现

## 一次命中改变哪些状态

| 状态由谁持有 | 复制与消费方式 |
| --- | --- |
| PlayerState 的 `UTPSCPPAbilitySystemComponent` 和 `UTPSCPPHealthSet` | ASC 使用 Mixed 复制，PlayerState 原生网络更新频率为 100 Hz。占有 Pawn 和客户端收到 PlayerState 时重新绑定 Avatar；服务器初始化生命并授予开火、换弹、冲刺能力。三项能力沿用 `ServerOnly` 策略。 |
| 武器与角色 | 弹匣 `Ammo` 和角色 `ReserveAmmo` 各自仅复制给拥有者。开火、肩射、ADS 和冲刺用 loose tag 同步相关表现，换弹用能力持有标签。 |
| HealthSet 的 `Damage`、`Health`、`MaxHealth` | `Damage` 是一次结算用的中间属性。生命和上限复制给各端；本地生命变化委托驱动 HUD。 |

服务器收到点伤害先按命中骨骼名查角色的伤害分区表，缺行用角色默认倍率。随后通用伤害回调将伤害乘以倍率并交给 ASC；普通范围伤害没有点命中缓存，倍率取 1。`UTPSCPPDamageEffect` 通过 `Data.Damage` 向中间属性写入伤害，HealthSet 消费后清零并把生命限制在 0 到上限之间。[分区表](../../Content/Data/DT_DamageZones.uasset) · [伤害入口](../../Source/TPSCPP/Character/TPSCPPCharacter.cpp) · [属性结算](../../Source/TPSCPP/AbilitySystem/TPSCPPHealthSet.cpp)

环境命中执行 `Cue.Weapon.Impact`，直接命中角色执行 `Cue.Hit.Blood`。即时命中的环境落点按一发合并，血液落点按受害者分组；多播抵达后在各端本地执行 Cue。投射物的环境碰撞消息携带命中特效资源，客户端无需在投射物销毁后再读取其数据。[即时命中](../../Source/TPSCPP/Weapon/HitScanWeapon.cpp) · [投射物命中](../../Source/TPSCPP/Weapon/Projectile.cpp) · [原生 Cue 注册与复制约定](../GAS_Notes.md)

生命归零后，服务器交给 GameMode 判定击杀。淘汰多播停止移动、输入与开火，关闭胶囊和自定义网格的查询碰撞，掉落武器，驱动主网格布娃娃、相机跟随及溶解表现。5 秒后由 GameMode 根据比赛阶段处理重生。[计分、重生和比赛阶段](../MultiplayerDesign.md)

## 一枪在三种网络角色上的表现

| 观察端 | 开火与状态反馈 | 应检查的数据 |
| --- | --- | --- |
| 远程玩家操作自己的角色 | 本机立即预扣弹量并播放开火；收到开火多播时跳过自己，收到弹匣复制后清除预扣。 | 对比预测 HUD 与 `OnRep_Ammo` 后的值。生命条由权威属性更新。 |
| 监听主机操作自己的角色 | 本机同时具有本地控制与权威身份；走服务端激活和多播路径，权威弹量可直接刷新自己的 HUD。 | 主机开火一次时，枪械动画和抛壳应各触发一次；还须从另一窗口查看主机动作。 |
| 观察其他玩家 | 装备引用、状态标签及多播驱动装备、动作和命中特效。 | 先看是否有命中，再查对应 Cue 的目标、点位和资源。 |

`ServerFire` 和 `MulticastFire` 是可靠 RPC。即时命中的环境 Cue 与角色血液 Cue 使用不可靠多播，所以丢包时单次表现可能缺失，即使伤害已经结算。投射物的环境命中使用可靠多播，携带位置、方向与特效资源，处理投射物同帧命中、销毁的情况。[开火 RPC](../../Source/TPSCPP/Components/CombatComponent.h) · [环境即时命中](../../Source/TPSCPP/Weapon/HitScanWeapon.h) · [投射物命中](../../Source/TPSCPP/Weapon/Projectile.h) · [角色血液 Cue](../../Source/TPSCPP/Character/TPSCPPCharacter.h)

## 如何定位延迟出现在哪一段

血量条分两步更新。角色属性委托把新值送给控制器，控件再把 `Health / MaxHealth` 转为目标百分比。没有 `HealthBarCurve` 时直接设置百分比；有曲线时，`NativeTick` 根据 `HealthBarInterpSpeed` 从当前显示值过渡到目标。判断复制延迟应看属性抵达时刻，判断视觉延迟应看控件到达目标的时刻。[控件实现](../../Source/TPSCPP/HUD/CharacterOverlay.cpp) · [历史延迟排查](../../../Docs/Bugs&Resolutions/Bugs&Resolutions.md)

若命中位置正确但两端都无特效，检查启动模块是否把原生 Cue 加入运行时集合，以及对象库重建后是否重新注册。具体引擎行为和当时的排查方法见 [GAS 迁移笔记](../GAS_Notes.md)。[模块初始化](../../Source/TPSCPP/TPSCPP.cpp) · [Cue Manager](../../Source/TPSCPP/AbilitySystem/TPSCPPGameplayCueManager.cpp)
