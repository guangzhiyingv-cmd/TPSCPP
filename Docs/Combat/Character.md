# 装备、瞄准与角色表现

[角色与战斗设计](../CombatDesign.md) / 角色操作

角色把输入转换为移动、姿态和镜头变化。调试时先确定观察者：持枪者能同时观察输入和相机，其他玩家只能通过复制状态观察其装备和动作。

## 输入与装备

本地 PlayerController 安装 Enhanced Input 映射。持枪腰射时，按下右键立即进入肩射，移速、散布和镜头同步响应。0.20 秒前松开，沿当前镜头进度继续进入 ADS；按满 0.20 秒再松开，退出肩射。已在 ADS 时，按下保持当前镜头，短按松开退出。在 ADS 中长按也能直接改用肩射。冲刺仍是按下进入、松开退出。[输入映射资产](../../Content/Input/) · [角色输入绑定](../../Source/TPSCPP/Character/TPSCPPCharacter.cpp)

服务器通过武器重叠区域设置候选武器，`OverlappingWeapon` 只复制给拥有者，主机本地玩家直接更新拾取提示。装备输入在权威端执行，客户端向服务器请求。一次只装备一把武器：更换时先掉落旧武器并取消换弹，再将新武器挂到角色自定义网格的 `hand_rSocket`。装备引用复制给所有端；远端据此挂接网格、切换动画层。Iris 左手对位读取武器的 `LeftHandSocket`，在 Control Rig 内用本帧右腕姿势还原握点。[冲刺握持与双端验证](SprintTurnaround.md) [装备逻辑](../../Source/TPSCPP/Components/CombatComponent.cpp) · [角色网格与动画](../../Source/TPSCPP/Character/TPSCPPCharacter.cpp)

| 状态 | 进入条件与玩家反馈 |
| --- | --- |
| Hipfire | 默认状态。装备后使用持枪朝向；准星随水平速度、滞空和武器参数变化。 |
| Shoulder | 持枪腰射时按下右键即进入，结束冲刺，镜头沿时间线过渡，移动速度设为步行速度的一半（当前角色为 150 cm/s）。步枪层站立时用对应方向的 Rifle Walk Start 起步，直行用 Rifle Walk 循环，持续按住右键折返时用 Rifle Walk Pivot。长按松开回到 Hipfire，短按松开继续进入 ADS。 |
| ADS | 右键短按松开后从 Shoulder 切入，换弹中禁止进入。使用武器的第一人称模型偏移、视场角和灵敏度，隐藏普通准星。狙击枪另用镜片材质、遮罩、后处理和分划线表现放大效果。 |
| 冲刺 | Hipfire 下请求冲刺能力；肩射、ADS、换弹标签阻止激活。进入瞄准或开火会结束冲刺。原生步行、冲刺速度分别为 500、1000 cm/s，角色蓝图可覆盖。 |

瞄准状态复制给其他机器，镜头过渡和后坐力由本地玩家驱动。动画实例把开火、ADS、换弹、冲刺标签映射到动画蓝图变量；冲刺对应变量名 `GameplayTag_IsDashing`。远端姿态异常时可沿“服务器标签 → 动画实例映射 → 武器动画层”定位。[动画映射](../../Source/TPSCPP/Animation/TPSCPPAnimInstance.cpp) · [狙击镜](../../Source/TPSCPP/Weapon/SniperWeapon.cpp)

第三人称动画通过 `bUseThirdPersonAimPose` 读取角色的瞄准状态。肩射和 ADS 共用 `Idle_ADS` 待机、上身覆盖和 Aim Offset 的混合逻辑。站立待机沿用序列切换的惯性混合；进入地面瞄准时，武器层逐帧插值混合权重。步枪肩射的四方向起步和循环继续由 `UseShoulderWalkCycle` 选择。调试远端时，先观察 `ABP_Mannequin_Base` 的共享姿势变量，再看武器层的 `UpdateIdleAnim` 与 `Update Blend Weight Data`。Iris 在重定向后显示结果。[共享武器动画层](../../Content/Characters/Heroes/Mannequin/Animations/LinkedLayers/ABP_ItemAnimLayersBase.uasset)

## 怎样调右键的点按和长按

1. 在内容浏览器打开 `Content/Input/IMC_Default`。展开 `IA_ADS` 和 `IA_ShoulderAim`，两行都选 `Right Mouse Button`。以后若要把这套操作换到同一个新键，两行一起改。[默认输入映射](../../Content/Input/IMC_Default.uasset)
2. 打开 `Content/Input/Actions/IA_ADS`。在 Details → Triggers 保留一个 `Tap`，将 `Tap Release Time Threshold` 设为 **0.20 秒**。在阈值前松手才切换 ADS，已在 ADS 时只在松开后退镜。[ADS 输入动作](../../Content/Input/Actions/IA_ADS.uasset)
3. 打开同目录的 `IA_ShoulderAim`。在 Triggers 保留一个 `Hold`，将 `Hold Time Threshold` 也设为 **0.20 秒**，关闭 `Is One Shot`。从腰射按下就进入肩射；达到阈值后松手回腰射。如果起点是 ADS，长按达到阈值才离开 ADS 进入肩射。[肩射输入动作](../../Content/Input/Actions/IA_ShoulderAim.uasset)
4. 保存两个动作。两处一起增大可放宽 ADS 点按窗口，两处一起减小可更快从 ADS 长按转肩射。腰射时的肩射起手始终在按下当帧。两个值相同，手势在同一处分界。若 Tap 的时间比 Hold 长，中间一段可能同时满足两者；反过来则可能两者都不触发。

肩射的 `Started` 在按下时生效，`Hold` 判定持续按住，`Tap` 在短按松手时提交 ADS。若换弹阻止进入 ADS，短按松手后结束肩射。左右键同帧按下时先处理肩射，第一枪按当前武器的肩射散布采样。[长按的处理代码](../../Source/TPSCPP/Character/TPSCPPCharacterAimInput.cpp)

## 为什么 ADS 分两段过渡

1. `DoADSStart` 标记 `bPendingADS`，提高当前相机时间线的播放速率并从现有位置继续。第一段调整弹簧臂长度、偏移和第三人称 FOV，终点为肩射视角。
2. `CameraTimelineFinished` 在本地角色仍处于待完成 ADS 时切到第一人称相机。它隐藏第三人称持枪模型，把武器的网格和材质实例交给 `ViewModelWeapon`，按武器插槽和相对变换挂接瞄准镜遮罩。同骨架遮罩可跟随武器姿态。
3. `ADSWeaponTimelineUpdate` 用同一个进度抬枪、从肩射 FOV 插值到武器的 `ADSFOV`，并调用武器的 `SetAimBlend`。进度达到 `ScopeReticleShowBlend` 后 HUD 才取分划线纹理。若能放大却看不到分划线，先核对纹理和控件绑定。
4. 松开肩射会倒放肩射过渡。切出 ADS 时则立即恢复普通 FOV、隐藏第一人称模型和分划线，再让弹簧臂倒放。换弹会从拥有者一侧退出 ADS。[相机与模型切换](../../Source/TPSCPP/Character/TPSCPPCharacter.cpp) · [分划线控件](../../Source/TPSCPP/HUD/CharacterOverlay.cpp)

## 两个网格的职责

角色移动的实测速度、肩射和 ADS 的选片差异，以及 Quinn 到 Iris 的足部轨迹对照，见[基础移动品质评估](LocomotionQuality.md)。

装备时通过 `CustomMeshComponentTag` 寻找自定义骨骼网格，它提供右手挂点，也作为左手变换的参照。武器动画层通过角色主网格的 AnimInstance 链接；淘汰时同一主网格进入布娃娃。排查“能开火但手姿错误”时，可先看挂点上的武器，再看主网格的动画实例与当前链接层。[装备与左手对位](../../Source/TPSCPP/Character/TPSCPPCharacter.cpp) · [状态标签映射](../../Source/TPSCPP/Animation/TPSCPPAnimInstance.cpp)

移动时准星如何变宽、开火时相机如何抬起，见[武器参数与射击](Weapons.md)。两端实际要观察的结果见[战斗进度](../CombatProgress.md)。
