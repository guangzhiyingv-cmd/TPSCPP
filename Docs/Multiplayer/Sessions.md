# 从 Steam 会话到 GameLevel

[联机与对局设计](../MultiplayerDesign.md) / 菜单与会话

## 从点击按钮到地图切换

| 环节 | 负责对象 | 行为 |
| --- | --- | --- |
| 打开菜单 | 插件的 `WBP_Menu`、`UMenu::MenuSetup` | 设置 UI 输入并绑定 Host / Join。原生默认参数为 4 个公开席位、`FreeForAll` 和 `/Game/Maps/Lobby`。 |
| 主持 | `UMultiplayerSessionsSubsystem` | 通过 OSS 创建 `NAME_GameSession`，发布 `MatchType`，允许进行中加入。已有同名会话时，先请求销毁，在回调中重新发起创建。成功后菜单执行 `ServerTravel` 到带 `?listen` 的大厅地址。 |
| 加入 | 子系统搜索，菜单选择结果 | 按 Lobby 条件搜索（上限 10000），按返回顺序尝试加入，失败则试下一个。当前搜索未按 `MatchType` 过滤。加入成功后解析连接地址，再由本地控制器执行 `ClientTravel`。 |
| 进入对战 | `ALobbyGameMode::PostLogin` | 总人数含主机，达到大厅的 `MinPlayersToStart`（原生默认 2）时，对 `/Game/Maps/GameLevel` 发起无缝切图。 |

会话子系统随 GameInstance 跨关卡存续。项目启用 OnlineSubsystemSteam 和 SteamSockets，开发 App ID 为 480；SteamSockets 的连接类配置在引擎配置中。[会话子系统](../../Plugins/MultiplayerSessions/Source/MultiplayerSessions/Private/MultiplayerSessionsSubsystem.cpp) · [菜单流程](../../Plugins/MultiplayerSessions/Source/MultiplayerSessions/Private/Menu.cpp) · [大厅规则](../../Source/TPSCPP/GameMode/LobbyGameMode.cpp) · [网络配置](../../Config/DefaultEngine.ini)

## 根据最后一个成功步骤定位问题

| 已经看到什么 | 接下来依赖什么 | 检查位置 |
| --- | --- | --- |
| 点击 Host 后没有进入大厅 | 有效的会话接口、本地玩家的 UniqueNetId、创建成功回调 | 子系统获取接口与 ID；已存在同名会话时检查销毁后的延迟创建分支。菜单在创建成功后才调用 `ServerTravel`。 |
| Join 搜索成功，结果数为零 | 主持方会话已经发布，来宾搜索到同一 Lobby 服务中的房间 | 对照创建端的 `bShouldAdvertise`、`bUseLobbiesIfAvailable` 和搜索端的 `LOBBYSEARCH`。开发 App ID 共用，若搜到多个结果，还要核对实际加入的主机。 |
| 能找到房间，但加入失败 | 每个候选会话返回各自的加入结果 | 菜单缓存搜索结果，从第一项开始。一次加入失败后尝试下一项，耗尽后恢复按钮。 |
| 已显示加入成功，仍停在原地图 | 先解析 `NAME_GameSession` 的连接地址，再由本地控制器执行 `ClientTravel` | 解析失败会记录 `Failed to resolve connect string`；这一分支已收到成功回调，与遍历下一个候选结果的分支不同。 |
| 双方已进入同一大厅，尚未进对战图 | 大厅 GameMode 在 `PostLogin` 中读取人数 | 核对包含主机的实际人数和大厅蓝图配置；进入 GameLevel 后才由对战 GameMode 管理热身。 |

插件的 `StartSession()` 是空实现。比赛阶段由核心模块的 `StartMatch()` 推进。菜单 API、委托签名及两个按钮的绑定名称见[插件接口说明](../../Plugins/MultiplayerSessions/README.md)。运行记录见[联机进度](../MultiplayerProgress.md)。
