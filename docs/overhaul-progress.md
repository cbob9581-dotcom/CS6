# CS6 机制重整进度 — 2026-11-06 18:30

工作工程和 canonical 插件都在 D:/CS6。原始素材归档保留，备份在 Backups/BeforeCS2Overhaul。

## 本轮（17:00–18:12）已验收

- 枪模重叠修正（用户截图确认的 AK 与手枪同时出现）：每把枪都会生成第三人称副本，可见性判断只排除了旁观标记、没排除本机玩家，于是挂在副武器上的手枪第三人称副本叠在第一人称 AK 上。现第三人称副本仅对非本机操控角色渲染，第一人称只显示当前武器。reports/cs2-rifle.png、cs2-reload.png。
- 枪模姿态修正：保留素材作者标定的 −90° yaw，不再用包围盒猜朝向（曾把 AK 判成倒置）；尺度按 Kit 骨骼包围盒长度自动换算，位置用**扳机锚点**对齐 Kit 的 Trigger 骨骼，握把回到手上。日志 `CS6 PRESENT ALIGN`。
- 掉枪姿态修正：掉落模型原先额外 roll 90° 变成"立着的棍子"，现只保留 yaw。
- 旁观第一人称：Bot 的 1P 手臂在 Kit 里没有驱动，硬推相机曾把镜头塞进手臂（截图已确认穿模）。现改为干净第一人称视点：隐藏身体网格与 1P 手臂，用相机 Actor 直视，`CS6 FP DIAG` 断言 armsVisible=0 / bodyVisible=0。
- 脚本场景被玩家输入打断的缺陷：`UpdateCS2Input` 每帧 `ReleaseInteraction` 会取消已安包场景的交互，导致 CS6DefuseScenario 到 60 秒超时失败。现脚本场景下不跑玩家 E/G 分支。
- 工具修复：`tools/sync_runtime_sources.ps1` 用了 .NET Core 才有的 `Path.GetRelativePath`，在 Windows PowerShell 5.1 下直接崩；改为前缀截取。`test_desktop_cs2.ps1` 的 marker 检查把日志路径当成了搜索词，改用 `-LiteralPath`。

## 本轮回归结果

- `tools/test_desktop_cs2.ps1`（默认入口 + 逐 Bot 审计）：PASS，9 个 Bot 无持续卡路、无未分类活动；reports/cs2-desktop-default-entry.log。
- `-Scenario CS6ShopScenario`：PASS，含真实模型换弹、掉枪拾枪保弹药、已开火拒绝退款。reports/cs2-desktop-CS6ShopScenario.log。
- `-Scenario CS6ControlScenario`：PASS，旁观/接管/拒绝敌方/延迟回调安全/下一回合恢复。reports/cs2-desktop-CS6ControlScenario.log、cs2-spectator-tp.png、cs2-spectator-fp.png。
- `-Scenario CS6DefuseScenario`：PASS，安包 300、拆包 300、比分 0:1。reports/cs2-desktop-CS6DefuseScenario.log。
- `tools/test_full_match.ps1`（MR12 整场）：PASS score=3:13，maxFirearms=19，零 Accessed None；出生检查 gap 6.7/10.0、玩家相机 166.7cm、无穿透。reports/bomb-runtime-full-match.log。
- 原生单元测试全绿：round_rules_test、economy_rules_test、navigation_progress_test。
- 交付清单：reports/cs2-build-manifest.json（exe/pdb SHA256）。

## 仍然未完成（不得声称已实现）
- 投掷物（闪光/烟雾/燃烧/高爆）只有购买与库存逻辑，未接实际投掷玩法，购买界面暂不售卖。
- 刀、C4 独立武器槽、雷达、CS2 逐枪弹道/后坐力、加时与联网均未实现。
- Bot 第一人称手臂动画仍未接入（当前用干净视点替代）；若要做"真第一人称观战"，需要给 AI 角色单独驱动 1P 骨骼。
- 新导航（42/192 站立胶囊 RecNavMesh）已重建，但仍需重新 cook 后再做一次逐 Bot 严格回归；本轮整场用的是旧 cook + 原生站立保护。
- 桌面包仍是 Development；最终交付前需要重新完整打包（BuildCookRun）而不是只换 exe。

Jev：用户表示会补名称，尚无官网/API/服务配置，不能虚构已接入。本地 AI 继续工作。BuildTools 仍用于开发，尚未卸载或移动安装目录。
