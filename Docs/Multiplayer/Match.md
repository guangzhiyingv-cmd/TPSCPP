# 比赛时钟、计分与重生

[联机与对局设计](../MultiplayerDesign.md) / 对战地图

## 比赛阶段与可调参数

| 阶段 | 服务端推进条件 | 本地显示 |
| --- | --- | --- |
| 等待与热身 | `WaitingToStart` 下，人数首次达到对战 GameMode 的 `MinPlayersToStart` 后启动热身，到时调用 `StartMatch`。倒计时启动后，当前逻辑不因人数下降而复位。 | `WBP_WarmupOverlay` |
| 进行中 | 按服务器的开赛时间计算剩余时间，到时调用 `EndMatch`。 | `WBP_CharacterOverlay` 显示生命、弹药、得分、死亡数和剩余时间。准星在本阶段启用。 |
| 结算 | `WaitingPostMatch` 开始时销毁仍由控制器持有的 Pawn 及其装备武器，结算结束后调用 `RestartGame`。 | `WBP_PostMatchOverlay` 显示领先玩家、击杀数和重开倒计时。 |

对战 GameMode 的 C++ 默认值是 2 人触发、热身 10 秒、比赛 120 秒、结算 10 秒。大厅切图人数、对战热身人数和会话席位分别配置，调试时要核对对应蓝图的实际值。[对局配置](../../Source/TPSCPP/GameMode/TPSCPPGameMode.h) · [对局推进](../../Source/TPSCPP/GameMode/TPSCPPGameMode.cpp)

服务器把各阶段的起始时间和时长写入每位玩家的 PlayerState；`PostLogin` 为中途加入者补发当前阶段的数据。本地控制器读取 GameState 的比赛阶段，定期用请求往返时间估算服务器时钟，在本地计算剩余秒数。[时间与 HUD](../../Source/TPSCPP/PlayerController/TPSCPPPlayerController.cpp) · [PlayerState 复制](../../Source/TPSCPP/PlayerState/TPSCPPPlayerState.cpp)

## 主机和客户端如何显示同一倒计时

监听主机用所在世界的时间。远程控制器在进入游戏时发起对时，之后按 `TimeSyncFrequency` 再次采样（C++ 默认 5 秒）。服务器回传收到请求的时间，客户端加上半个往返时间估算当前服务器时间，把差值保存在 `ClientServerDelta`。每帧显示时，本地时间加上该差值，再参与阶段剩余时间计算。[对时实现](../../Source/TPSCPP/PlayerController/TPSCPPPlayerController.cpp) · [对时参数](../../Source/TPSCPP/PlayerController/TPSCPPPlayerController.h)

例如，一局时长为 120 秒，比赛从服务器时间 35 秒开始，客户端估算的当前服务器时间是 50 秒，则显示剩余 105 秒，即 01:45。比赛或结算的起始时间尚未到达客户端时，相应剩余时间函数返回 0。若阶段面板正确而秒数持续为零，依次核对 PlayerState 的阶段起点、时长和控制器的时钟差。

`APlayerHUD::BeginPlay` 创建已配置的热身、对战和结算控件，初始都隐藏。本地控制器根据 GameState 的比赛状态切换可见性。比赛计时读取 PlayerState，结算领先者读取 GameState，血条读取角色 ASC，弹量经战斗组件送入角色面板。因此，“整个面板没出现”和“只有一个数值错误”分别从控件类绑定和该数值的生产者开始查。[HUD 创建与显示](../../Source/TPSCPP/HUD/PlayerHUD.cpp) · [玩家状态字段](../../Source/TPSCPP/PlayerState/TPSCPPPlayerState.h)

## 击杀归属与重生

受害者生命耗尽后，GameMode 给受害者加一次死亡。存在有效攻击者且双方 PlayerState 不同时，攻击者加 1 分，GameState 重新扫描玩家得分，复制 `TopKillerName` 和 `TopKills`。只在分数严格更高时更新，因此同分只显示扫描时遇到的第一位最高分玩家；无人取得正分时姓名为空。[计分](../../Source/TPSCPP/GameMode/TPSCPPGameMode.cpp) · [领先玩家](../../Source/TPSCPP/GameState/TPSCPPGameState.cpp)

角色播放淘汰表现后，服务器在 5 秒时请求重生。GameMode 只在 `InProgress` 处理该请求，销毁旧角色并从现有 `APlayerStart` 中随机选点重生。出生点数量与位置由关卡提供，当前选择逻辑不按敌人距离或视线筛选。[淘汰入口与生命值](../CombatDesign.md) · [重生实现](../../Source/TPSCPP/GameMode/TPSCPPGameMode.cpp)

`RequestRespawn` 的顺序是检查比赛阶段、处理旧 Pawn、收集关卡里的 PlayerStart、选择并重启玩家。若复活时场内没有可用的 PlayerStart，函数在收集后返回。关卡里名为 Spawn 的白盒网格属于静态几何；应使用 World Outliner 的类筛选来核对实际出生点。[关卡出生点核对记录](../../../TPSMAP/Docs/GameLevel_Progress.md) · [跨阶段回归](../MultiplayerProgress.md)
