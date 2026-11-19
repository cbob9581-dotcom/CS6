# 当前交付：开局武器装配与 C4 输入修正

用户要求自行试玩；不运行自动对局、测试场景或截图巡检。

本次修改：
- Equip 不再全量 DestroyWeaponAndAttached。相同类的现存武器保留；更换槽位时显式销毁 FPCurrentParts/TPCurrentParts，防止独立配件 Actor 遗留。
- 去除 C++ Deferred SpawnGun 手动装配，改为复制默认配件到角色对应数组，再调用套件原始 SpawnWeapon 函数。
- 不再每帧重写 CanShoot，保留套件射击和换弹冷却。菜单与目标动作只门控 ShootingAllowed/CanAim。
- 5 选择 C4；包点内按住左键安包；CT 按 E 拆包；松键、切枪或移动中断；G 掉 C4；T 方靠近自动拾取。保留现有 3.2/5/10/40 秒计时。
- C4 持有时隐藏枪械表现并显示现有炸弹模型；恢复时按组件原隐藏状态还原。没有新增专用持包/安包手部动画，不能声称完整复刻 CS2。

状态：Game Development 编译成功（session94782 Exit0）。仅代码变更，新 exe/PDB 已放入 Build/Desktop/PendingUpdate，由启动游戏.cmd 在用户退出当前游戏后应用。未进行游戏验收，不声明模型表现已验证。

# 最高优先级：用户自行验收 — 2026-11-06 19:38

用户明确要求“不要你验…你改好我验”。停止运行自动验收、自动对局、截图巡检和测试场景，除非用户以后明确重新授权。仍可完成必要编译、资产保存和打包。

本次只恢复原配枪械基线：
- 地图目录WeaponClass改回原始BP_1911/BP_AK74M/BP_AK74U/BP_SKS，保留六个阵营/商品项，显示实际枪名。VisualClass/VisualMesh清空。
- CS6Economy::Equip取消创建任何CS6WeaponPresentation，原枪骨骼网格、配件与动作全部交回FPS Kit。
- CS6Input不再删除Aim Down Sight绑定，也取消每帧强制关闭普通枪瞄准的代码。
- ACS6Character::CalcCamera恢复Super，仅保留GetPawnViewLocation的感知/旁观胶囊计算。
- 启动游戏.cmd带-CS6ManualLoadout，原生初始化仅给本机玩家开局步枪+手枪、8000，方便用户手动切枪，不代表正式经济。
- native-weapon-restore.json记录资产保存成功。package_windows.ps1运行session59234；尚待最终打包返回，不可声称已全修复或通过验收。
- 不要重新启用整枪替代或散件挂骨骼实验。用户已因模型、动作、切枪、瞄准和成本强烈不满。
- 掉枪兼容已增加包内静态展示网格回退，源代码和PendingUpdate已包含；未运行验证。

# 最新：整枪与眼高试玩包 — 2026-11-06 19:21

用户要求先试玩，现暂停进一步修改与长测试，等待反馈。
- 用户截图已复现：旧方案同一枪的不同静态部件按不同骨骼挂载造成散件错位；旧切枪诊断证明 stray=0/legacy=0，不能再说只是两把枪叠加。
- 已改成每枪一个完整 StaticMesh：15商品对应10个合并网格，材质保留，FP/TP各1组件，整体随Kit根骨骼运动；死亡/手动掉枪也改用整个网格。原始包未改，备份Backups/BeforeModelRegression-20261106。
- UCS6WeaponPresentation运行于PostUpdateWork，明确区分本机人类和单机AI，隐藏原Kit表现与非当前枪。
- ACS6Character::GetPawnViewLocation/CalcCamera改用实际胶囊计算眼高，与手臂动画解耦；CameraRules单测站160/蹲105（胶囊底以上）通过。实际站蹲与整体枪模实景尚待试玩，不能宣称全面修复。
- Game/Editor编译及新资产cook/stage/archive已成功，package session16237 Exit0。新版入口Build/Desktop/Windows/UETacticalPack.exe。已可见启动普通对局，日志reports/user-playtest-whole-20261106.log；记录reports/current-user-playtest.json。
- 整体网格报告reports/whole-weapon-assets.json saved=true offers15。完整编辑器合并保存后在退出时遇到TypedElement引用断言；打包进程重新加载资产成功，无此保存/加载错误。
- 新增tests/camera_rules_test.cpp与tools/validate_whole_weapon_models.py；Shop场景新增俯视反复切枪、真实PlayerCameraManager站蹲眼高检查。用户要求先玩，因此尚未运行该新版完整回归。
- 旧18:30及cs2-build-manifest.json的全绿结果、SHA仅适用于旧版，不能套用当前版本。新整枪无独立弹匣/枪机动画，沿用Kit手臂/枪根动作；完整骨骼重新绑定以后再做。

# 18:30 接续检查点（优先于下文历史）

- 枪模重叠修正（用户截图确认）：每把枪都会生成第三人称副本，可见性判断只排除旁观标记、未排除本机玩家，导致副武器手枪的第三人称模型叠在第一人称 AK 上；现第三人称副本仅对非本机操控角色渲染。另修正枪模姿态：保留素材 −90° yaw、按 Kit 骨骼包围盒换算尺度、用扳机锚点对齐 Trigger 骨骼，握把回到手中；掉枪不再额外 roll 90°。reports/cs2-rifle.png、cs2-reload.png。
- 旁观第一人称改为干净视点：Bot 的 1P 手臂在 Kit 内无驱动，硬推相机会把镜头塞进手臂，现隐藏身体/手臂并用相机直视，`CS6 FP DIAG` 断言 armsVisible=0、bodyVisible=0。
- 修复脚本场景被玩家输入取消交互（安包被 `ReleaseInteraction` 打断导致 60s 超时）；修复 sync_runtime_sources.ps1 在 PowerShell 5.1 下崩溃、test_desktop_cs2.ps1 marker 检查误用路径。
- 回归全绿：默认入口逐 Bot 审计（9 Bot 无持续卡路）、ShopScenario、ControlScenario、DefuseScenario、完整 MR12 比赛 PASS score=3:13 maxFirearms=19 零 Accessed None。原生单测 round/economy/navigation 全绿。
- 交付清单 reports/cs2-build-manifest.json。仍待：投掷物玩法、刀/C4 槽、雷达、逐枪弹道、新导航重新 cook 后逐 Bot 回归、Shipping 全量打包。

# 15:10 接续检查点

- 完整自然MR12比赛23回合10:13通过，230个出生检查零穿透，换边后镜头170cm。出生旋转根因和延迟阵容/尸体重置已修正。
- 工作工程接入经济、真实15枪商品、购买/退款/购买区、护甲、战斗HUD、Tab、击杀记录；shop实景通过。
- 本队旁观和E接管实景通过：人数不增、63HP和7弹保留、拒绝敌人；清除旧尸体延迟回调后28秒仍维持控制。reports/cs2-takeover-passed.log。此前延迟失控失败另存cs2-takeover-delayed-callback-failure.log。
- 新枪包静态模型已导入；CS6WeaponPresentation刚写好，用真实模型附加到Kit骨骼，弹匣/枪机动画绑定。初次编译仅TObjectPtr循环auto*不匹配已改auto&，等待再编译。install_cs2_economy.py增加模型坐标偏移尚未重新运行。还未看适配后画面。
- 桌面包仍是阶段1新出生/AI/MR12，尚无购买/接管新cook。必须Game编译并重新cook后交付。
- Jev用户表示会补服务名，尚无官网/API，不得声称已接入模型。本地Bot照常运行。
- 待完成：模型实景/动作验收，掉枪拾枪/投掷物经济，逐Bot活动检查，新打包和整场回归，操作说明及只读审阅。

# 当前构建状态 — 2026-11-06 13:49

## 当前新任务：CS2机制重整（优先于下方历史）

用户反馈换边半身陷地、机制混乱、Bot卡住/不动，要求CS2购买/战斗HUD/旁观接管Bot，新增枪包并可接模型。此前明确自主继续、不操作，因此按既定UE路线直接实施，不重复批准流程。模型问询用户选择“Jev（我补充服务名称）”，**仍无实际服务名称/官网/API，不能猜接口或密钥**；本地功能继续。

- 新设计/计划：docs/superpowers/specs/2026-11-06-cs2-overhaul-design.md、plans/2026-11-06-cs2-overhaul.md。原始归档只读，无Git，备份在Backups/BeforeCS2Overhaul（原Cpp、RoundRules、原桌面exe和地图）。未新增目标工具，也未卸载工具链。
- **已证实陷地根因**：旧configure_military_gameplay.py使用unreal.Rotator(0,yaw,0)，本机位置参数实际把yaw写到pitch。reports/spawn-rotation-audit.json 显示A出生pitch30/yaw0，B出生pitch-30/yaw180/roll180；真实整场换边后玩家cameraHeight由166降至46，AI为负值，胶囊仍无穿透，因此原Z验证漏检。失败证据已备份reports/overhaul-upside-down-failure.log。是此前脚本错误，已向用户明确承认。
- 已新增CS6SpawnPlacement.{h,cpp}：按真实站立胶囊和世界碰撞/导航规划位置，角色强制仅Yaw直立、恢复默认胶囊及Mesh相对姿态。Enroll改为全10人5/5齐备后原子放置；等待时只冻结，不再每帧反复登记/传送部分阵容。每0.25s等待重试。工具validate_spawn_recovery.py旧用户日志正确失败，初始渲染十人测试已通过（玩家camera166,gap10，无穿透，9个Bot均17–39m移动）。**直立修复后换边复测仍在进行，不能提前声称通过**。
- 地图实际保存修复完成：fix_spawn_rotations.py 10点显式pitch0/roll0/yaw30或210，reports/spawn-rotation-fix.json savedtrue。configure脚本也改为关键字参数；地图变动尚未重烹饪进入桌面包，当前原生直立保护可对旧烹饪地图生效。
- 新Bot模块Private/CS6BotObjectives.cpp替代旧主Cpp内UpdateObjectives（旧函数已删）。导航单元NavigationProgress.h和tests/navigation_progress_test.cpp 红→绿：保留进行中路径、不接受partial、目标变更/请求间隔、2秒无进展则附近可达点脱困+跳跃；协同主点/分路、安装/拆除优先、4秒最后所见位置追踪，复用Kit战斗树。当前没有每Bot模式日志/更强整场AI判据，后续需要加。
- RoundRules MR12更新：12回合换边、13胜、24回合12:12平局（本地竞技，无加时）。tests/round_rules_test.cpp先旧规则失败再通过；为避免Windows assert弹框已改测试宏失败return1。runtime/RoundRules.h仍只是include canonical header。
- 经济纯C++核心Public/EconomyRules.h和tests/economy_rules_test.cpp已红→绿，通过800/16000、阵营/买区/时间、退款、护甲/头盔差价、四雷/双闪、输赢奖励/输家等级、保枪/死亡/换边。**尚未接UE库存/购买菜单，也未重写HUD或旁观接管**。absorb护甲伤害方法已写但未单独测/接入；需原生Character父类在TakeDamage前拦截，不能靠事后回血伪实现。
- 新枪包：素材/终极 FPS 武器包 - Ultimate FPS Weapons Pack/【4.26...】...rar 已解到SourceAssets/WeaponsPack，558文件、1.65GB。Content/ChamferZone_Weapons复制到工作工程同名Content（引用保持）。审计0错误reports/weapon-pack-pose-audit.json：228 StaticMesh、24 Blueprint、157Texture2D、41Material；**没有骨架/第一人称动作或动画蓝图，这些BP是静态装配展示，没有Fire/Reload函数**。M4目录其实Benelli M4霰弹枪，不是M4A4；步枪是MK18/M16，AKM、MP5/P90/UMP/P350等。不能仅把这些BP当Kit可射击武器。需用Kit战斗类适配静态分件视觉，复用手部动作，购买目录只开放真实适配的枪。未生成模型。
- Game编译session24530已成功（包含upright、MR12、新AI；经济header未接无影响）。已复制exe至桌面包。
- **当前运行session79441**：tools/test_full_match.ps1 然后validate_spawn_recovery.py，MR12整场+换边站姿。工具FullMatch validator已改13胜或12:12，超时600s；原生测试时限5000世界秒。固定60Hz在正常音频初始化后开，不能用-benchmark/-nosound。待收结果再推进。
- 接下来：先收整场姿态结果/确认native正立；AI每Bot模式与进展验证；清除原尸体latent旧回调（RoundReset目前只销毁controller当前Pawn，旧无主尸体可能跨回合仍回调）和原模板冲刺/侧身/职业/定制入口；经济与真实库存/购买区/B菜单；新枪视觉适配；NativeHUD和本队观战/E接管（Bot1P_Camera默认height251，不能直接当玩家第一人称）。然后Editor编译/重打包/外层入口全回归。不要以当前核心测试代替整体CS2机制完成。

## 以下为旧开发版历史

## 08:50 最新结果（覆盖下方历史接续）

- **严格整场通过** session89954 Exit0：新 reports/bomb-runtime-full-match.log 实际 XAudio2 音频设备初始化，`CS6 FULL clock fixedHz=60 audio=1`；9个自然回合，七回合后换边，1:8结束，含Reason3拆除及Reason4爆炸（未使用受控场景）；maxFirearms11，零Accessed None。新的 validate_full_match.py 强制空引用检查和音频标记。benchmark两个失败日志已分别备份。
- 最终源码 Native Enroll/回合与 HUD 已同步 Editor/Game 编译成功，session80353 Exit0。普通无测试参数游玩关闭周期诊断；已删除不适用于Custom胶囊的历史SweepSingleByProfile探针，保留真实CurrentFloor检查。死亡登记关闭受伤保护仍在。独立审阅对fixedclock与死亡保护无Important/Critical发现。
- **最终启动入口回归通过** session95602 Exit0：最新游戏exe，`smoke_bomb_match.ps1 -Render -UseLauncher` 使用外层 Windows/UETacticalPack.exe（与启动游戏.cmd相同入口）。初始十人无穿透，8Bot自主移动12–31m，出现正常战斗死亡，SMOKE PASS，无空引用。最终截图 reports/military-runtime.png。匹配的当前原生调试符号PDB也已复制，交付文件SHA256见 reports/first-playable-build-manifest.json。
- 首个开发包入口 `D:/CS6/Build/Desktop/启动游戏.cmd`；操作说明同目录。仍标开发版，购买经济、最终菜单/角色外观/竞技枪感/性能调优未完成；不宣称完整CS2复刻。工具链仍保留后续制作，未卸载或移动安装路径。详见 docs/first-playable-validation.md。

## 08:47 严格验收接续（优先于下方）

- 08:43 最终正常渲染和死亡恢复模式都跑通，session16429 Exit0。死亡观战、拾包、复活完整通过，保留并精确报告1次既有 KillerInfo duplicate-add 警告，其他脚本消息/空引用均失败。正常模式8Bot移动17–34m。HUD真实HP100、30/83备弹已截图核对；旧TDM面板和枪上重复弹药已隐藏，观战不画活人准星。
- 曾完整跑完10回合、换边、2:8结束、枪械最多11件（原session33859）。**但进一步检查全场日志找到 Accessed None，严格验收不能算全通过。** 原日志保存在 `bomb-runtime-full-match-no-audio-failure.log`，新 validator 已要求没有 Accessed None。
- 已确认 UE Core/Public/Misc/App.h:434 的 FApp::CanEverRenderAudio 在 IsBenchmarking 时直接 false；模板 ROC_SupressionSound 的 CreateSound2D 返回空音频组件，后续 FadeIn 不检查，产生空引用。不能把所有此类错误归因于死亡尸体；第二次去掉nosound仍有benchmark，因此同样失败，已停止专属测试进程。
- 当前编译 session **82848**：CS6FullMatch 在正常游戏/音频初始化后的 BeginPlay 设置 FApp fixed60Hz；测试CLI移除benchmark、fps、nosound，仍nullrhi。日志须出现 `CS6 FULL clock fixedHz=60 audio=1`，并检查音频设备实际初始化。编译完成后复制游戏exe到桌面包，重新跑 tools/test_full_match.ps1。脚本专属进程超时360秒；待结果，尚不能交付为严格全场通过。
- 同时已在 CS6RoundDirector::ReportDeath 中设置 SetCanBeDamaged(false)，避免登记死亡后继续收到伤害；该保护应保留，但不冒充上面无音频根因的完整修复。此前保护编译成功session35351，编辑器也编译成功；新fixedclock代码编辑器还需同步编译。
- 只读审阅 final_runtime_review 已完成，无确认的Important/Critical缺陷，认可精确报告KillerInfo已添加警告；没有审核后续1行死亡受伤保护及fixedclock测试入口。文档 first-playable-validation.md 的完整比赛条目待严格重跑修订。

## 最新接续点（覆盖下方历史记录）

- 08:26 新地图包 UAT BUILD SUCCESSFUL / ExitCode=0，session 83620 完成。原山体修复只改运行时 collision enabled，重载磁盘后恢复旧属性。已使用 actor/component.modify、NoCollision profile 和 actor collision false 保存；独立重开工程核对十个组件均 NO_COLLISION、profile NoCollision、actor_enabled false。证据 `reports/saved-backdrop-collision.json`。原始环境资产未改，地图备份仍在。
- **正常 Bot 实测通过**（session 59041）：`tools/smoke_bomb_match.ps1 -Render` Exit0，十人初始胶囊无穿透，8名Bot实际自主移动2485–3969cm；age30剩8人，age40剩4人，确认存在真实交战/死亡。证据 `reports/bomb-runtime-render.log`，工具已自动要求至少3Bot移动超过200cm。
- **拆弹及回合重置回归通过**（session 60016）：`-Render -DefuseScenario` Exit0，安装→拆除→比分0:1→下一回合十人存活持枪。`CS6 RESET removed round items=83`；实际枪械11件，重置后仍11。此前107计数包含继承武器父类的配件，已改按Primary/Secondary武器类路径计数。初始持枪验收记忆第一轮十人，避免合法玩家死亡导致错误失败。
- 当前正在编译 session **68174**，尚未读取最终结果：新增开发专用 `-CS6WeaponScenario`，通过 PlayerController.InputKey 实际输入验证开火减弹、R换弹、2手枪/1步枪。测试脚本 `validate_weapon_actions.py` 已在旧日志正确失败。同步的原生 HUD 新增真实生命/弹药/备弹/任务提示和准星，移除旧 GameModeHUD；编译通过后需复制新游戏exe到桌面包，再运行 `smoke_bomb_match.ps1 -Render -WeaponScenario` 并查看截图。不要用尚未跑过的新验收宣称完成。
- Build/Desktop 新增 `启动游戏.cmd`（直接军事基地）及中文 `操作说明.md`，开发版标签明确。尚未做最终菜单、购买、死亡观战/掉包与整场换边集成验收，不是最终交付。工具链清理/移走亦未进行。

## 下方为先前历史记录

## 当前接续点（以本节为准）

- 已成功打包新开发版（11/6 07:21 附近，UAT ExitCode=0，cook 0 errors/4 warnings）。之后通过 `Build.bat UETacticalPack Win64 Development -Project=...` 增量编译并将项目 Binaries/Win64/UETacticalPack.exe 复制到 Build/Desktop/Windows/UETacticalPack/Binaries/Win64，避免 C++ 变化反复烹饪未变内容。新的地图变化仍须重新打包。
- 原生适配器真正进入游戏：十名角色、5/5 队伍、十人持枪；安装→拆除→比分 0:1→下一回合十人复活，渲染实测通过。证据 `reports/bomb-runtime-defuse.log`。当前截图 `reports/military-runtime.png` 是正常模式出生视角，已显示完整 AK 和 TEAM A；不是完成整局的证据。
- 修复了 PlayerState/PC 职业枚举：数字0是 None，数字1才是 Assaulter；用原 `SVR_SetTeam`、HUD `RepresentingTeam` 更新队伍；从 DT_WeaponSelection 读取并调用原 `AddDefaultStartingPartsPrimary/Secondary`，AK6配件、1911两配件，不再只显示枪架。
- 出生改为用已测地面坐标+真实胶囊半高直接设置，并清旧速度；`tools/validate_runtime_positions.py` 已从失败变通过。首次十人出生Z均约6220。
- **Bot 自主对战仍未通过，尚不能称完成游戏。** `validate_bot_activity.py` 在旧日志明确失败（九个Bot XY完全不变）。诊断并非脑暂停或导航代理：请求成功2，NavData存在，PF tick/active均1，完整路径也存在，角色收到非零加速度，速度上限380，Walking，胶囊 query开启，RVO关闭。运行时真实 CurrentFloor 判定 `StaticMeshActor_30` 穿透约66m，扫掠命中相同；这是背景山体巨大简化凸包包住营地。先前编辑器 ECC_Pawn 检查0阻挡不能覆盖模板Custom胶囊情况，不能作为物理无阻挡证明。
- 已在工作地图备份后仅关闭十个 `SM_Mountain*` 远景组件碰撞，保留 Landscape 与营地建筑。重建导航，并重选双方各5个出生点，全部到 A/B 两点路径完整，已经保存。B队移到东侧约(10000,6900)/(10000,6300)一带。`reports/military-collision-fix.json`、`reports/military-gameplay-setup.json` 已更新。**该地图物理修复尚未进入包，下一步必须重新打包再运行正常 Bot 活动验证。** 备份 `Backups/BeforeBackdropCollision`。
- Read-only review agent `/root/runtime_review` 审阅，确认回合重置漏销毁独立枪械/配件。已核对模板原图后在源码调用 `DestroyWeaponAndAttached`，并清理 `/Game/MultiplayerFPS/Game/Weapons/` 运行时遗留物，增加 `CS6 INVENTORY` 数量检查；仍待新地图版本跑拆包/复活回归。审阅未确认正常死亡、观战、掉包和换边实际运行通过。
- 开发自测入口 `tools/smoke_bomb_match.ps1 [-Render] [-DefuseScenario]`。正常自主模式还需手动/脚本运行 `validate_bot_activity.py` 验证至少三Bot移动超过2m；计划加入自动判据。原生调试输出暂多，Shipping编译会裁剪大部分日志。最终未交付，购买经济、独立前端/最终HUD及整局验收未完成。
- 上次运行中的 session 34973 是旧地图 headless 已接近结束；碰撞修复脚本 session 81466 已完成0错误。无编辑器交互窗口由用户控制，避免误杀/覆盖运行包。

## 最新验证

两个旧 MetaSound 的编辑器缓存图已通过资产工厂 `InitEdGraph` 重建并实际保存。最小命令行复现（1911 → AK74M → FPSCharacter）通过，ExitCode=0、0 errors / 3 warnings，日志 `reports/probe-after-metasound-factory.log`。已撤回不相关的七个蓝图迁移，所以当前修复范围集中在两个音频资产。无窗口音频编辑器路径不再使用：曾分别因禁用音频发生 AudioWidgets 除零、因 NullRHI 窗口对象发生 GetRestoredDimensions 断言。

包含新爆破适配器、开发自测场景和这两个音频修复的独立包正在重新生成，session 75650。尚未完成新包的运行测试，不能使用上一版包的成功替代验证。

目标：复用 UE5 FPS Kit 的武器、动画、控制和 AI，在军事基地地图完成独立 Windows 单机 Bot 爆破游戏。用户已明确允许编译爆破模块。

## 已有证据

- 工程：`D:/CS6/Game/Ultimate Multiplayer FPS Kit UE5.4/UltimateMultiplayerFPSKit/UETacticalPack.uproject`，UE5.4.4；两版 Kit 游戏资产经归档 CRC 对照相同；原始归档未改动。
- 军事基地已迁入，地图 `/Game/CS6/Maps/M_MilitaryBase` 已保存，双方各 5 个出生点有地面检测。导航生成后重新加载，两队出生区之间路径有效且非部分路径。证据：`reports/military-gameplay-setup.json`、`reports/military-navigation-validation.json`。
- 菜单数据表已实际保存为军事基地单地图、TDM 单模式，GameInstance 默认 Bot 为 4/5。已核对磁盘文件，不能只依据内存导出报告。
- 原爆破核心模块已编译通过 Editor/Development/Shipping；纯 C++ 边界测试通过。后来新增 `CS6TacticalMatch` 模板适配器（首次自动出生、原模板复活队列、冻结、Bot 目标、F 交互及原生 UMG 回合界面），新增代码 Editor 已编译，初版适配器 Development 也已编译。最新开发自测场景仍待编译进桌面包。**尚未完成实战运行验收，不能称为可玩的完整爆破对局。**
- 两个爆破点已实际放入并保存，A=(6500,7000,6120)、B=(9000,8500,6120)，双队到双点导航均有效；原模板角色、AK74M、1911 和已有箱体模型以地图属性引用。证据 `reports/bomb-match-map-setup.json`。
- 21:21 桌面包成功，UAT ExitCode=0，烹饪 0 errors / 4 warnings。`Build/Desktop/Windows/UETacticalPack.exe`，ucas 时间 21:21:30，大小 4,316,298,464 字节。保留原版菜单外观，含军事基地入口和 Bot 配置。
- 最新桌面日志 `reports/desktop-military-smoke.log` 确认独立程序完成初始化和菜单加载。未完成地图实战验收。

## 排查状态

菜单恢复后曾成功打包，但新增地图硬引用又触发相同崩溃，因此不能断言菜单修改是根因。品牌菜单备份：`Backups/MenuDiagnostic/WB_MainMenu.branded.uasset`。当前磁盘仍保留原版菜单外观；修正后的品牌脚本未运行验收。

已用 `OnHandleSystemEnsure` 和 `FUObjectSerializeContext.SerializedObject` 定位：真正触发坏图钉校验的是枪声 MetaSound 的编辑器缓存图，而非角色蓝图。资产为 `/Game/MultiplayerFPS/Game/Effects/Sounds/Weapons/AK-74/MSS_AK-74Fire` 与 `.../SKS/MSS_SKS_Fire`，节点均为 `MetasoundEditorGraphExternalNode_17`。证据 `reports/trace-blueprint-load.log`。

七个相关蓝图刷新/编译/保存的尝试未解决最小复现，已从 `Backups/BlueprintMigration54` 撤回，不能再把这些蓝图当根因。两个音频简单重存也未解决。当前通过官方 MetaSound 资产编辑器 API，从原音频前端文档重建编辑器缓存图；不修改音频行为或跳过校验。两个原文件备份在 `Backups/MetaSoundMigration54`。第一次实现传入 SetGraph(nullptr) 被 UE CastChecked 拒绝，未保存；已改为新建正确图类并打开官方资产编辑器同步，正在运行 `rebuild-legacy-metasounds-2.log`。

模板图检查结果在 `reports/BP_GMBase-graphs.txt`、`BP_BaseGS-graphs.txt` 等：CreateBots 为首生工厂，SpawnBots 消费 GS.AIInLoadout 控制器队列，SpawnCharacterWithClass 会先销毁旧玩家 Pawn 再调用 SpawnPlayer。TeamA=NewEnumerator0、TeamB=NewEnumerator1。适配器按这些实际接口写入，未另造角色和枪械系统。

21:22 电脑控制连续返回 `foreground window did not report a process id`，未取得截图；停止重复 UI 操作，继续原生接口和日志检查。

## 未完成

- 军事基地玩家出生、选枪、射击/换弹、Bot 移动和战斗验证。
- 爆破与模板对接、A/B 点、Bot 目标、统一复活、观战、HUD、购买及整场验收。
- 菜单外观、军事基地真实缩略图、性能与枪感打磨，最终包和控制说明。
- 工具链在 `D:/CS6/BuildTools`，按用户要求构建结束后处理移走/清理；不可直接移动已安装路径导致注册失效。
