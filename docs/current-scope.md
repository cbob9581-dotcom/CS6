# 当前执行范围（2026-11-05）

用户最终澄清：复用 FPS Kit 的枪械、动画、操作、角色、Bot 等，加用军事基地地图，制作一个类似 CS2 的独立 Windows 桌面游戏。

不是仅替换原游戏地图，也不从零重写已有系统。首版目标仍为单机 Bot 对战。

执行顺序：先把军事基地场景接到现有游戏模式，跑通角色、枪械、动画、Bot、导航与独立启动打包；再基于现有模式补足爆破与 CS2 风格规则。此前新增规则代码是待集成模块，不代替现有射击系统。

资源：工作工程 `Game/Ultimate Multiplayer FPS Kit UE5.4/UltimateMultiplayerFPSKit/UETacticalPack.uproject`；军事基地已接入其 `Content/FC_MilitaryCamp`。复用清单见 `reports/reuse-manifest.json`。

用户要求完成后移走新增工具；编译工具安装目标为 D:/CS6/BuildTools，安装包为 tools/vs_BuildTools.exe。完成后通过安装器迁移或卸载，不直接搬动已安装目录。
