# 装备、瞄准与角色表现

[角色与战斗设计](../CombatDesign.md) / 角色操作

角色把输入转换为移动、姿态和镜头变化。调试时先确定观察者：持枪者能同时观察输入和相机，其他玩家只能通过复制状态观察其装备和动作。

## 输入与装备

本地 PlayerController 安装 Enhanced Input 映射，角色绑定移动、视角、跳跃、冲刺、装备、肩射、ADS、开火与换弹动作。肩射和冲刺按下进入、松开退出；ADS 按下切换。具体键位由 [输入映射资产](../../Content/Input/) 配置，动作入口见 [TPSCPPCharacter.cpp](../../Source/TPSCPP/Character/TPSCPPCharacter.cpp)。

服务器通过武器重叠区域设置候选武器，`OverlappingWeapon` 只复制给拥有者，主机本地玩家直接更新拾取提示。装备输入在权威端执行，客户端向服务器请求。一次只装备一把武器：更换时先掉落旧武器并取消换弹，再将新武器挂到角色自定义网格的 `hand_rSocket`。装备引用复制给所有端；远端据此挂接网格、切换动画层。左手对位读取武器的 `LeftHandSocket`。[装备逻辑](../../Source/TPSCPP/Components/CombatComponent.cpp) · [角色网格与动画](../../Source/TPSCPP/Character/TPSCPPCharacter.cpp)

| 状态 | 进入条件与玩家反馈 |
| --- | --- |
| Hipfire | 默认状态。装备后使用持枪朝向；准星随水平速度、滞空和武器参数变化。 |
| Shoulder | 从持枪 Hipfire 按住肩射进入，结束冲刺，镜头沿时间线过渡，移动速度设为步行速度的一半。松开回到 Hipfire。 |
| ADS | 从持枪 Hipfire 切入，换弹中禁止进入。使用武器的第一人称模型偏移、视场角和灵敏度，隐藏普通准星。狙击枪另用镜片材质、遮罩、后处理和分划线表现放大效果。 |
| 冲刺 | Hipfire 下请求冲刺能力；肩射、ADS、换弹标签阻止激活。进入瞄准或开火会结束冲刺。原生步行、冲刺速度分别为 500、1000 cm/s，角色蓝图可覆盖。 |

瞄准状态复制给其他机器，镜头过渡和后坐力由本地玩家驱动。动画实例把开火、ADS、换弹、冲刺标签映射到动画蓝图变量；冲刺对应变量名 `GameplayTag_IsDashing`。远端姿态异常时可沿“服务器标签 → 动画实例映射 → 武器动画层”定位。[动画映射](../../Source/TPSCPP/Animation/TPSCPPAnimInstance.cpp) · [狙击镜](../../Source/TPSCPP/Weapon/SniperWeapon.cpp)

## 为什么 ADS 分两段过渡

1. `DoADSStart` 从 Hipfire 进入 ADS，标记 `bPendingADS` 并启动相机时间线。第一段调整弹簧臂长度、偏移和第三人称 FOV，终点为肩射视角。
2. `CameraTimelineFinished` 在本地角色仍处于待完成 ADS 时切到第一人称相机。它隐藏第三人称持枪模型，把武器的网格和材质实例交给 `ViewModelWeapon`，按武器插槽和相对变换挂接瞄准镜遮罩。同骨架遮罩可跟随武器姿态。
3. `ADSWeaponTimelineUpdate` 用同一个进度抬枪、从肩射 FOV 插值到武器的 `ADSFOV`，并调用武器的 `SetAimBlend`。进度达到 `ScopeReticleShowBlend` 后 HUD 才取分划线纹理。若能放大却看不到分划线，先核对纹理和控件绑定。
4. 松开肩射会倒放肩射过渡。切出 ADS 时则立即恢复普通 FOV、隐藏第一人称模型和分划线，再让弹簧臂倒放。换弹会从拥有者一侧退出 ADS。[相机与模型切换](../../Source/TPSCPP/Character/TPSCPPCharacter.cpp) · [分划线控件](../../Source/TPSCPP/HUD/CharacterOverlay.cpp)

## 两个网格的职责

装备时通过 `CustomMeshComponentTag` 寻找自定义骨骼网格，它提供右手挂点，也作为左手变换的参照。武器动画层通过角色主网格的 AnimInstance 链接；淘汰时同一主网格进入布娃娃。排查“能开火但手姿错误”时，可先看挂点上的武器，再看主网格的动画实例与当前链接层。[装备与左手对位](../../Source/TPSCPP/Character/TPSCPPCharacter.cpp) · [状态标签映射](../../Source/TPSCPP/Animation/TPSCPPAnimInstance.cpp)

移动时准星如何变宽、开火时相机如何抬起，见[武器参数与射击](Weapons.md)。两端实际要观察的结果见[战斗进度](../CombatProgress.md)。
