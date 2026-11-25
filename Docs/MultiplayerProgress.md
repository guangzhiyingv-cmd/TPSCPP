# 联机与对局进度

截至 2026-11-24，已按[联机与对局设计](MultiplayerDesign.md)核对工作区内的菜单、会话子系统、两个 GameMode、PlayerState、GameState 和控制器。建房到结算重开的代码路径均已存在。本次为文档回填，以下运行场景待实测。

排查接入问题先查[会话回调与地图切换](Multiplayer/Sessions.md)；双方已在对战图时查[比赛计时、HUD 与重生](Multiplayer/Match.md)。

| 从哪里接手 | 当前落点 | 如何判定通过 |
| --- | --- | --- |
| GameLevel 双人 PIE | 以 Play As Listen Server、2 名玩家直接进入对战图。有效人数门槛和时长取自该图使用的 GameMode。 | 两个窗口依次显示热身、比赛、结算。轮流由主机和客户端完成击杀，核对双方得分、死亡数、5 秒后重生、结算领先者及下一局的初始状态。 |
| 中途加入与离开 | 对战 GameMode 的 `PostLogin` 单独处理三个阶段。 | 分别在热身启动后、比赛中、结算时加入；新玩家应看到当前阶段和剩余时间。热身开始后有人退出，检查剩余玩家继续倒计时。 |
| Steam 双机联调 | 从 Lobby 检查 `WBP_Menu`，两台机器用不同 Steam 账号运行相同构建。默认启动图会直达 GameLevel。 | 一端 Host，另一端 Join。记录是否找到目标房间、解析地址、进入同一大厅、达人数切图，并完成一局。PIE 直连只覆盖对战路径。 |

**中途加入的判定示例。** 在主机已开始比赛、房间还有空位时加入一名来宾，记录主机的比赛剩余时间。来宾应进入对战面板；等本地收到比赛起点和时长后，倒计时应跟随同一局继续。若来宾显示整局初始时长，先看 `PostLogin` 补发的阶段数据；若双方都还在大厅，回到大厅人数和地图切换检查。

**失败定位。** 仅显示“Joined session”时，继续查看连接地址解析和客户端切图日志。能进大厅却未切图，检查大厅 GameMode 的人数门槛。Lobby 地图资产引用旧路径 `/Game/Blueprints/BP_LobbyGameMode`，该处的 ObjectRedirector 指向 `GameModes` 目录下的实际蓝图。已在 GameLevel 却一直显示热身，检查对战 GameMode 的人数门槛、PlayerState 阶段时间和控制器读取的比赛状态。[菜单回调](../Plugins/MultiplayerSessions/Source/MultiplayerSessions/Private/Menu.cpp) · [大厅 GameMode 蓝图](../Content/Blueprints/GameModes/BP_LobbyGameMode.uasset) · [对战 GameMode 蓝图](../Content/Blueprints/GameModes/BP_ThirdPersonGameMode.uasset)

**验证记录。** 本次核对了默认地图、Steam 配置，以及 Lobby 和 GameLevel 资产中的菜单或 GameMode 引用。核心模块及自有联机插件源码中未检出项目专用 Automation 测试用例。当前版本的 PIE 双端结果和 Steam 跨机器结果尚待记录。
