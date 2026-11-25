# TPSCPP 项目文档

TPSCPP 是 UE 5.8 第三人称自由死斗项目。玩家经 Steam 会话进入监听服务器；从大厅切到 GameLevel 后，比赛依次经历热身、交战、结算和重开。

## 从你要做的事开始

| 我想了解 | 先读 | 接手已有工作 |
| --- | --- | --- |
| 玩家怎样装备、瞄准、开火、换弹和淘汰对手 | [角色与战斗设计](CombatDesign.md) | [战斗进度与双端回归](CombatProgress.md) |
| 玩家怎样开房、加入、计分和重生 | [联机与对局设计](MultiplayerDesign.md) | [联机进度与双机验证](MultiplayerProgress.md) |
| 关卡如何支撑移动、交火和出生 | [GameLevel 白盒概览](../../TPSMAP/Docs/README.md) | [当前白盒与待走测位置](../../TPSMAP/Docs/GameLevel_Progress.md) |

**第一次运行。** 用 UE 5.8 打开 [TPSCPP.uproject](../TPSCPP.uproject)。编辑器和游戏默认进入 GameLevel。看一局对战，用 2 名玩家、Play As Listen Server 进入 PIE；看建房和加入，从 Lobby 打开菜单。两种入口覆盖的流程不同。[启动地图配置](../Config/DefaultEngine.ini)

**文档怎么读、怎么更新。** 概览说明玩家经历什么；系统设计页定义玩法规则，按问题链接到相应实现专题；专题提供字段、调用顺序和源码入口。进度页可直接进入，集中记录核对结果、证据和下一项验证。玩法规则变更时先更新对应设计，验证有结果时更新该主题的进度。完整工作区包含同级 TPSMAP 和历史故障目录。

仓库协作与提交约定见[工作区指南](../../AGENTS.md)，过往问题的复现和修复过程见[工作区记录](../../Docs/README.md)。Visual Studio 集成工具随插件提供[独立使用说明](../Plugins/VisualStudioTools/README.md)。
