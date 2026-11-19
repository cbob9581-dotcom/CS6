# 早期实施记录 — 2026-11-05（已过时）

最新进展见 [current-build-status.md](current-build-status.md)。以下保留早期诊断历史，不代表当前状态。

用户已指示开始，按 UE5 + Blender 单机 Bot 爆破首版设计实施。

## 已完成

- 确认 UE5.4.4 编辑器与 7-Zip 可执行文件。
- FPS Kit UE5.4 归档解压成功，7-Zip 返回 0。
- 工作工程：`D:/CS6/Game/Ultimate Multiplayer FPS Kit UE5.4/UltimateMultiplayerFPSKit/UETacticalPack.uproject`。
- 工作工程 EngineAssociation 从 5.3 调整为 5.4，启用 PythonScriptPlugin 和 EditorScriptingUtilities 用于自动化。
- 军事基地包解压至 `D:/CS6/SourceAssets/MilitaryCamp`，返回 0，669 个文件，共 7,231,706,124 字节；含日夜基地、预制件及草地地图。尚未迁入工作工程。
- 只读注册表审计成功：1398 个模板资产，15 张地图；结果 `reports/template-audit.json`，日志 `reports/ue-inventory.log`，UE 汇总 0 errors、1 warning（Steam 子系统未加载）。
- 日志确认部分动画引用 `/Game/MultiplayerFPS/Effects/`，实际音效位于 `/Game/MultiplayerFPS/Game/Effects/`。已在 DefaultEngine.ini 增加 PackageRedirects；后续加载未再输出原有两条音效路径缺失警告，但还不能宣称全部引用修复。

## 当前失败和下一步

最初受限启动没有输出日志并崩溃，沙箱外启动可完成初始化和只读审计；原因尚未完全定位。

实际加载蓝图仍然失败：CompileAllBlueprints 在成功编译 BP_DroppedMag 后，加载 BP_1911 时发生 EXCEPTION_ACCESS_VIOLATION。NullRHI 与 AllowCommandletRendering 两条路径均复现，分别见 `reports/ue-blueprints.log`、`reports/ue-blueprints-render.log`。因此不能归因于 NullRHI，也不能将资产清单成功当成模板可玩。

下一步优先在独立 UE 编辑器进程对 BP_1911 的依赖进行分组加载定位，并检查原始包中硬编码的旧目录与循环蓝图引用；必要时用 UE5.3 包作为隔离对照（不覆盖当前工程）。未确定根因前不重复完整编译，不修改原始素材，不基于失效父类创建爆破玩法。

尚未实现爆破规则、Bot 目标扩展、新地图或打包程序；尚未进入真实对局验收。
