# 联机与对局设计

[项目文档](README.md) / 联机与对局

一局游戏经过 Steam 会话、大厅和对战地图。会话决定谁可以加入，大厅决定何时切图，对战 GameMode 决定比赛何时开始和结束。三处拥有各自的配置。

## 玩家经历的一局

| 阶段 | 玩家可见的结果 | 规则归属 |
| --- | --- | --- |
| 创建或加入 | 主机创建公开会话并进入监听大厅；来宾搜索会话、加入并连接主机。 | 插件负责会话操作和菜单回调。[开房与加入流程](Multiplayer/Sessions.md) |
| 大厅与热身 | 大厅人数达标后全员切到 GameLevel。对战图独立判断人数并开启热身；已经启动的倒计时会继续。 | 大厅和对战图各自的 GameMode。[阶段与调参位置](Multiplayer/Match.md) |
| 对战与重生 | 淘汰其他玩家加 1 分，受害者死亡数加 1。比赛进行中，淘汰 5 秒后服务器从现有 PlayerStart 随机选点重生。 | GameMode 裁决，PlayerState 保存个人数据。[计分与重生](Multiplayer/Match.md#击杀归属与重生) |
| 结算与重开 | 比赛到时结束，客户端切换结算面板，显示领先者及重开倒计时。结算结束后服务器重启游戏。 | GameState 提供领先者和比赛状态，控制器计算本地显示时间。[时钟与 HUD](Multiplayer/Match.md#主机和客户端如何显示同一倒计时) |

改房间席位看菜单参数，改切图人数看大厅 GameMode，改热身、比赛和结算时长看对战 GameMode。进入了 GameLevel 后，直接从比赛阶段定位即可。[会话配置](Multiplayer/Sessions.md) · [对战默认值](Multiplayer/Match.md) · [联机进度与验证](MultiplayerProgress.md)
