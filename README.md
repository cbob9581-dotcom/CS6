# CS6

基于 Unreal Engine 5.4 和现有 FPS Kit 的单机 Bot 爆破游戏工程。当前仍在开发中。

## 获取工程

需要 Git LFS。完整素材约 10.5 GB：

```sh
git lfs install
git clone https://github.com/cbob9581-dotcom/CS6.git
cd CS6
git lfs pull
```

在 UE 5.4 中打开 `Game/Ultimate Multiplayer FPS Kit UE5.4/UltimateMultiplayerFPSKit/UETacticalPack.uproject`。工程 EngineAssociation 为原开发机标识，其他电脑可通过“Switch Unreal Engine version”选择本机 UE 5.4。需要 Visual Studio C++ 工具链及 Windows SDK。

## 目录

- `Game/`：UE 工程、地图、蓝图与素材，以及安装到工程的运行插件。
- `Plugins/CS6BombRuntime/`：运行插件的维护源；修改后执行 `tools/sync_runtime_sources.ps1` 同步到 UE 工程。
- `tools/`：资产处理、编译与打包脚本；部分脚本使用原开发机路径 `D:/CS6` 和 UE 安装路径。
- `docs/`：开发记录和已知限制。
- `tests/`：已有测试源；用户当前要求由其手动试玩验收。
- `Build/Desktop/*.cmd`：本地打包后的启动入口；可执行文件和打包素材不纳入仓库。

本仓库不包含下载压缩包、独立原始素材目录、构建缓存、历史备份、编译产物或本地 Build Tools。工程内使用的第三方素材遵循各自授权，本仓库不另行授予其再分发或使用权。
