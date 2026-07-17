# Changelog

## [2026-07-17] 濒死/酒/判定区规则 + 武圣选择 + 奸雄伤害牌

除保留双人简化规则“吕布【无双】不作用于【决斗】”和“【过河拆桥】不选择判定区牌”外，修复本轮确认的其余规则问题。

**濒死与【酒】**
- `Player::setHp` 不再把负体力截为 0；例如 1 体力受到 3 点伤害后保留为 -2，必须回复到 1 才脱离濒死。
- 救援队列先询问濒死者本人，再询问其他存活玩家；濒死者可以用【桃】或【酒】自救，其他角色只能用【桃】。
- 新增独立的出牌阶段用酒标记；造成伤害、被【闪】抵消或被防具抵消后，下一张【杀】都会消费酒杀强化，但同一出牌阶段仍不能再次饮酒。

**判定区与武将技能**
- 判定区禁止同名延时锦囊共存，按后置入先判定；未命中的【闪电】会跳过已有【闪电】的角色。
- 关羽可通过“发动技能”明确选择一张红牌当【杀】，不再要求原牌本身不可使用；孙权【制衡】的一回合一次限制保持不变。
- 曹操【奸雄】改为可发动或跳过，发动后从弃牌堆取得实际造成伤害的牌；杀、AOE、决斗、借刀响应和闪电均传递伤害牌。濒死获救及 AOE 后续响应会恢复未完成的奸雄选择/目标链。

**网络与测试**
- `PendingActionData` 新增 `isSkillChoice`，协议升至 v3；本地与联网模式共用“发动奸雄/跳过”待定动作，服务端继续按连接身份校验响应者。
- 更新并新增负体力多桃、自救、他人酒救援拒绝、酒杀消费、同名延时锦囊、后置先判、武圣主动转化、奸雄选择和协议 round-trip 用例。五个测试目标全部通过。

## [2026-07-17] 联网胜负提示 + 主动断开清理 + ClientApp 棋盘生命周期

修复联网对局结束与连接失败路径中的三项 View/App 生命周期问题。

**View：按本地玩家身份显示正确胜负**
- 问题：`GameBoardWidget::onGameOver` 只判断 `winnerId >= 0`，因此联网胜方和败方都会显示「游戏结束！获胜！」。
- 修复：联网模式下比较 `winnerId` 与 `m_localPlayerId`，分别显示「你获胜」或「你失败了」；平局保持原提示。本地同屏模式改为显示具体的「玩家 N 获胜」，避免含糊的第一人称文案。

**App：正常 GameOver 主动断开不再误报连接错误**
- 问题：收到 GameOver 后主动调用 `disconnectFromServer()`，仍会触发已连接到 `SGSApp::onConnectionError` 的 `disconnected`/`errorOccurred`，可能在正常结局后再次弹出「与服务器断开连接」。异常连接路径也可能因 error + disconnected 连续到达而重复处理。
- 修复：`releaseClient()` 在主动断开前先移除 `GameClient → SGSApp` 的生命周期信号连接，再统一断开 socket 和延迟销毁 `ClientApp`；`onNetworkGameOver()` 只走这一条清理路径，避免重复断开与错误弹窗。`GameClient ↔ GameBoardWidget` 的内部连接不受影响。

**ClientApp：释放未被主窗口接管的 GameBoardWidget**
- 问题：`ClientApp` 创建的 `GameBoardWidget` 初始没有 QWidget 父对象；连接失败、版本拒绝或房间已满时不会进入 `MainWindow::showGamePage()`，删除 `ClientApp` 也不会自动删除 board，反复重试会泄漏整棵控件树及其定时器。
- 修复：新增 `ClientApp` 析构函数。board 仍无父对象时由 `ClientApp` 删除；若已被 `QStackedWidget` 接管，则继续由主窗口负责。监听 board 的 `destroyed` 信号同步清空内部指针，避免外部提前销毁后的悬空引用。

**测试**
- `ViewTest` 新增 `gameBoardGameOverResult`：覆盖联网胜/负/平局和本地同屏获胜文案。
- `ViewTest` 新增 `clientAppBoardOwnership`：覆盖未接管 board 随 `ClientApp` 释放，以及已接管 board 不被重复删除。
- `SanguoshaQt`、`ViewTest`、`NetworkTest` 重新编译通过；完整 `ViewTest` 与 `NetworkTest` 直接运行均返回 0。

## [2026-07-17] log 顺序播放 + 借刀路由 + 判定定时器兜底

三项修复，全套件 5/5 通过（新增 borrowCardResponse / logQueueSequentialPlayback / logQueueClearedOnPhaseChange / judgeTimerMaxRetriesProtection 用例）。

**View：结算 log 队列顺序播放**
- 问题：一次出牌结算在同一调用栈内连续 `emit logMessage`（如「A 使用【杀】」→「B 打出【闪】」→伤害提示），`GameBoardWidget::onLogMessage` 每次直接 `setText` 覆盖 `m_logLabel`，玩家只看到最后一条。
- 改造：新增 `struct LogEntry { QString text; QString style; }` + `QQueue<LogEntry> m_logQueue` + `m_logPlayTimer`（single-shot 1200ms）。`onLogMessage` / `onJudgmentPerformed` 改为入队；`onPlayNextLog` 从队首取一条显示、停留约 1.2s 再播下一条，队列播完启动 `m_logRevertTimer`（2s）回退到当前阶段提示。判定 log 携带 `Theme::judgmentBar(effective)` 样式随队保留颜色。
- `onPhaseChanged` / `onPendingActionCreated` / `onPendingActionCleared` / `onGameOver` 统一走 `clearLogQueue()`（清空队列 + 停止播放/回退定时器），阶段切换与新 pending 时立即显示对应文案，不残留旧结算 log。
- 顺带去掉 `GameViewModel::executePhaseJudge` 中紧随 `emit judgmentPerformed` 的重复 `emitLog(resultText)`（文本相同），改队列后不再显示两次，并修好判定色被普通 log 覆盖的旧问题。
- View 仍零依赖 VM；`m_logLabel` 加 `objectName("logLabel")` 供测试 `findChild` 定位。

**Model / ViewModel：借刀杀人响应路由（确认 bug）**
- 问题：`GameRule::executeBorrow` 建 `requiredCardType=Kill, isDuel=false` 的 pending，`ActionViewModel::respondCard`/`skipResponse` 的 Kill 分支非决斗即走 `handleAoeKillResponse`（南蛮语义）。后果：被借刀者出杀被当「响应南蛮」弃掉不伤人；不出杀反而 `dealDamage(responder,...)` 自己掉血，与借刀规则完全相反。
- 修复：`PendingActionInfo` 增 `bool isBorrow`；`executeBorrow` 置位；新增 `GameRule::handleBorrowResponse` —— 出杀→伤害使用者（`info.source`）；不出杀→被借刀者武器移交使用者。`respondCard`/`skipResponse` Kill 分支增加 `isBorrow` 路由（两人局简化诠释：无第三方，目标对使用者结算）。

**ViewModel：判定定时器无限重挂兜底**
- `GameViewModel::onJudgeTimerFired` 原在 `hasPendingAction()` 时无上限地 `start(100)` 重挂。加 `JUDGE_TIMER_MAX_RETRIES=10` 上限：超限则记诊断 log 并强制 `clearPendingAction()` 后继续推进，正常路径重置计数器。实测濒死流程 pending 必被清除、不会死锁，此为防御性兜底。

## [2026-07-17] tobefixed 修复：装备/武圣/大乔/结算 log + NetworkTest 竞态

处理 `tobefixed.md` 剩余项（同伴 `dd9433d` 已解决扣血❌心、移除五谷/八卦/麒麟弓/丈八、删本机 IP），并修掉网络 e2e flaky。

**规则 / Model**
- `GameRule::executeKill` 增加可选 `Card* killCard`：出杀前做防具判定。仁王盾对**黑色杀**直接无效（不进入出闪）；攻击方装备青釭剑（`ignoreArmor()`）时跳过防具。`KillCard::execute` / 武圣转化出杀均传入原牌判花色。
- 牌堆总数 **101→95**（同伴已从牌堆移除五谷×2、丈八、麒麟弓、八卦×2；类与逻辑保留）。
- 装备 hover 文案已在构造函数 `setDescription` 中齐全（仁王盾/青釭剑等），`CardWidget` tooltip 直通，无需额外 UI 改动。

**武将**
- 关羽武圣：回退同伴「发动技能按钮→当杀」实现（该路径有 `skillTransformCard(nullptr)` 误判、只能打第一活人等硬伤）。恢复出牌转化路径——出牌阶段直接点红牌当杀，走 `playsAsKill`/`playCard`，受出杀次数限制（非张飞/连弩一回合一次）。主动技能按钮仅孙权制衡使用。
- 大乔流离：两人局无第三人可转移，选将页 `CHAR_LIST` 加 `hidden` 字段暂时隐藏大乔（**不删** charId=7 / `DaQiao` 类 / `executeKill` 流离分支，索引与 `createCharacterById` 对齐）。

**View：结算 log 停留后回退**
- `GameBoardWidget` 新增 `m_logRevertTimer`（2s single-shot）：`onLogMessage` / 判定展示后停留约 2 秒，再切回当前阶段文案（出牌阶段/弃牌阶段等）；`onPhaseChanged` / 新 pending 到来时取消回退。View 零依赖 VM，阶段文案由 `phaseLogText(m_currentPhase)` 本地生成。
- `onPhaseChanged` **先 stop 再按需 start** `m_autoAdvanceTimer`，避免 Draw 的 300ms 定时器在进入 Play 后仍触发，把 Play 误推进到 Discard。

**网络 / 竞态**
- `ServerApp`：`AdvanceRequested` 在 **Discard** 阶段拒绝（弃牌须走确认）；Play 仍允许作为 endPlay 等价物（与 `advancePhase` 的 pending 保护一致）。
- `GameViewModel::advancePhase`：`m_judgePending` 时拒绝外部推进，消除与判定定时器竞态。
- `NetworkTest::clientAppWiresBoardToGameClientWithZeroBoardChanges` 改为轮询推进阶段 + 监听 `playCardRequested` + 服务端兜底，降低 loopback 时序敏感；全套件连跑稳定 5/5。

**测试**
- ModelSmoke/ModelTest：牌堆 95、仁王盾挡黑杀 / 红杀正常 / 青釭剑无视防具用例。
- NetworkTest 函数数随既有用例约 65；CTest 5/5 连续多次通过。

## [2026-07-17] UI 美化 Step U4：手牌扇形排列 + 绘制层动画

- `CardWidget` 新增**扇形绘制模式**（`setFanMode(enabled, rotationDeg, baseLift)`）：开启后控件外扩 `2*FAN_MARGIN`（10px），牌面居中绘制在中央 80×112 区域并**绕牌面中心旋转**，四周留白容纳旋转四角与上浮溢出。关键：旋转/上浮全为**绘制层**（`QPainter` 旋转+平移），控件几何仍是正立矩形，Qt 事件命中只认几何 → 固定坐标点击测试完全不受影响。裸控件（非扇形）保持 80×112。
- `CardWidget` 上浮改**动画驱动**：`m_liftAnimation`（`QVariantAnimation`，120ms OutCubic）平滑过渡悬停/选中/扇形基线的上浮量，替换 U3 的静态 `yOffset`；新增 `playEntranceFade(delayMs)`（200ms OutQuad 透明度动画）供入场淡入，几何立即为最终值、仅绘制层淡入。singleShot 以 `this` 为 context，控件销毁自动取消，无悬空。
- `HandCardAreaWidget::arrangeCards()`：≤6 张走**弧形扇形**（相邻 ±3° 旋转、封顶 ±9°，两端抛物线下沉 `FAN_DIP` 控制在 margin 内），几何按外扩尺寸摆放但牌面中心对齐期望位置；>6 张退回原水平重叠（非扇形、恢复 80×112）。`rebuildWidgets()` 对每张牌错峰 40ms 触发入场淡入。
- ViewTest 新增 `handCardFanArrangement`：断言 ≤6 张为扇形模式（控件 100×132）、外扩后固定坐标 `QPoint(15,15)` 点击仍命中正确 cardId、>6 张退回水平重叠（控件恢复 80×112、非扇形）、两种模式点击均正确路由。
- 尺寸常量、鼠标事件、`clicked/doubleClicked` 信号、`cardId<0` 逻辑不变；信号/槽签名零变化，`connection.md` 无需更新；全套件 5/5 通过（ViewTest 含新用例，NetworkTest 真实点击手牌 e2e 连跑 3 次稳定）。

## [2026-07-17] UI 美化 Step U3：CardWidget 牌面重绘

- `Theme.h` 新增卡牌自绘配色常量段（`QColor`，供 `CardWidget` 的 `QPainter` 直接使用，非 QSS）：纸质暖白渐变 `CardPaper*`、双线描边 `CardBorderOuter/Inner`、装备牌青绿边 `CardEquipBorder`、红/黑花色与卡名色、牌背暗红与金纹 `CardBack*`、底部类型徽章底色（基本暗棕/锦囊暗紫/装备暗青绿）与文字色、状态覆盖层绿/蓝/暖光。原先散落在 `CardWidget.cpp` 的硬编码色值全部迁入。
- `CardWidget::drawCardFront`：纸质暖白竖向渐变底 + 双线描边（外暗棕、内浅金；装备牌外线换青绿加粗）；左上角标改**竖排**（数字在上、花色在下）；居中**大花色水印**（40px 半透明，压在卡名下层）；卡名居中按字数自适应字号；底部类型徽章改**圆角胶囊**（按类型分色 + 金描边 + 居中），保留装备槽位图标（⚔/🛡）与武器攻击范围 `+N`。空占位牌（`cardId<0`）分支保留，改用纸底 + 浅金内描边。
- `CardWidget::drawCardBack`：暗红纸底渐变 + 裁剪在牌面内的**金色菱形网格纹理** + 双层内嵌金框 + 中央双层金菱徽记，替换旧的四线交叉简纹。
- `CardWidget::drawStateOverlay`：可打出绿描边加**外扩柔光晕**；选中蓝加白色内高光双描边；悬停暖光罩，均取 Theme 颜色。
- 尺寸（80×112）、`paintEvent` 的 hover/select 偏移与阴影、鼠标事件与 `clicked/doubleClicked` 信号、`cardId<0` 不发信号逻辑全部不变；ViewTest 基于坐标的点击断言不受影响。信号/槽签名零变化，`connection.md` 无需更新；全套件 5/5 通过。

## [2026-07-17] UI 美化 Step U2：选将页与对话框换肤

- `Theme.h` 扩充选将页/对话框用 QSS 工具：`pageBackground`（objectName 限定的深色页面底，避免级联）、`pageTitle`/`pageSubtitle`、`accentText`/`mutedText`、`charCard`（武将卡片暗木渐变 + 悬停金框，悬停只换 border-color 不换宽度避免几何抖动）、`infoBar`（联网状态/等待提示）、`separator`、`inputField`（深色 QLineEdit/QSpinBox）、`dialogBase`（QDialog 深底 + QLabel 级联）；新增 `P1Accent`（青蓝）/`P2Accent`（赤橙）玩家强调色。
- `MainWindow`：本地选将页与联网选将页全部深色古风化——页面深色渐变底（`setObjectName` + `WA_StyledBackground`）、金色标题/VS、武将卡片走 `Theme::charCard()`（单选/说明文字由卡片 QSS 级联配色，不再逐控件设置）、开始对战按钮改暗朱红渐变（与对局界面「结束出牌」同色系）、确认选择改暗松绿、创建/加入房间改暗青蓝/暗黛青、联网状态与等待对手提示条走 `Theme::infoBar`。
- `NetworkConfigDialog`：深色对话框底（`Theme::dialogBase`）、金色标题、深色输入框（`Theme::inputField`，含焦点金框）、取消/连接按钮复用 `Theme::flatButton`/暗松绿 `gradientButton`。
- 按钮文案（「开始对战」等 ViewTest 依赖的文本）、控件几何、信号/槽签名零变化，`connection.md` 无需更新；全套件 5/5 通过。

## [2026-07-16] UI 美化 Step U1：深色古风主题基础 + 对局界面换肤

用户指定 UI 美化走深色古风牌桌方向（plan2.0.md §3，分步计划见 CLAUDE.md「UI 美化分步计划」）。本步为主题基础：

- 新增 `src/View/Theme.h`：深色古风色板（暗木面板/金色点缀/牌桌深绿）+ QSS 片段工具函数（玩家面板四态、渐变按钮、扁平按钮、徽章、提示条、判定横幅）。仅 View 层使用，无逻辑无状态，跨控件配色统一从这里取值。
- `GameBoardWidget`：新增 `paintEvent` 自绘牌桌背景（深绿绒面渐变 + 四周 vignette 暗角 + 内侧细金线描边）；日志条与判定横幅样式迁移到 `Theme::hintBar`/`Theme::judgmentBar`（判定生效红/未生效绿的深色变体）；濒死面板样式改走 `Theme::panelDying()`。
- `PlayerInfoWidget`：面板常态/当前回合/可选目标/濒死四态样式全部迁移到 Theme（金框当前回合、绿光目标态、红光濒死）；头像改暗朱红渐变 + 金描边；文字/徽章（装备/判定/攻击范围/回合指示）换深色变体。**顺带修复**：QWidget 直接子类的 QSS `background`/`border` 需要 `WA_StyledBackground` 属性才会真正绘制，此前的面板边框/底色样式实际未渲染，本次在 `setupUi()` 开启。空心体力由 🖤 改为 🤍（深色面板上黑心不可见）。
- `ActionPanelWidget`：提示条与 4 个按钮（结束出牌暗朱红/发动技能暗紫/跳过深色扁平/确认弃牌暗松绿）迁移到 Theme 渐变按钮。
- 按钮文案、控件几何、信号/槽签名零变化，`connection.md` 无需更新；全套件 5/5 通过。

## [2026-07-17] 合并同事修复并适配本地决斗实现

在本地决斗修复提交上合并 `main` 的无懈可击、吕布无双、主动技能、判定区和延时锦囊改动，解决双方实现之间的路由与分层不一致。

**规则与状态**：
- 保留四参数 `executeDuel(..., duelCard)` 和统一 `handleDuelResponse`，通过 `sourceCard`/`isDuel` 保持 A、B 交替出【杀】；无懈目标放弃响应后会恢复同一条决斗链。
- 无效的决斗/无懈响应不会消耗手牌；吕布【无双】按 `requiredDodgeTotal`/`dodgeProgress` 连续要求两张【闪】。
- 延时锦囊使用后只进入判定区，被无懈抵消或判定结束后才进入弃牌堆；闪电命中时造成 3 点伤害，未命中时移至下一名存活玩家。
- 孙权【制衡】移入 `ActionViewModel`：选择任意张手牌、每回合限一次，并校验阶段、当前玩家、牌权、重复 ID 和待定动作。

**View 与网络**：
- `GameBoardWidget` 使用两阶段技能选择（进入多选/确认），`PlayerData::canUseActiveSkill` 控制按钮可见性；`SGSApp` 恢复为只负责连接和生命周期。
- 网络协议升至 v2，序列化 `canUseActiveSkill`/`judgmentCards`，新增 `SkillRequested` 和 `JudgmentPerformed`；服务端对技能命令使用连接层可信身份，不信任消息体中的 `playerId`。
- `ClientApp`/`GameClient` 接通技能命令与判定结果，联网客户端和本地模式复用同一套 View/VM 行为。

**测试与文档**：
- 新增决斗无懈恢复、无效响应、吕布双闪、延时锦囊区域、制衡权限/次数/多选、协议 v2 DTO、技能身份防伪和判定广播测试。
- `NetworkTest -functions` 当前为 63 个；按 README 方案二使用 Qt 6.11.1 + MinGW 13.1 在 `D:\tmp` 构建，CTest 5/5 通过。
- 同步 README、connection.md、interface.md、plan2.0.md；`docs/` 加入 `.gitignore`，已跟踪文件仅从 Git 索引移除，磁盘内容不改。

## [2026-07-17] 武将头像图片显示（#8）

- 新增 `images.qrc` Qt 资源文件，注册 `images/` 下所有武将头像图片
- `CMakeLists.txt` `SanguoshaQt` 目标加入 `images.qrc`，编译嵌入 exe
- `MainWindow` 选将页：每个武将卡片增加 60×60 圆形头像（从资源加载，裁剪为圆形），无图片武将回退首字符显示
- `PlayerInfoWidget` 信息面板：48×48 圆形头像替换纯文字首字符，渐变背景圆框不变

## Bug Fixes

### [2026-07-16] 无懈可击（Nullify）无法响应锦囊牌

**现象**：对手使用锦囊牌（过河拆桥、顺手牵羊、决斗、乐不思蜀等）时，目标玩家无法使用无懈可击抵消。

**根因**：
- `checkNullifyChain` 和 `executeNullify` 均为空实现（stub），整个无懈可击机制未接入游戏逻辑
- `NullifyCard::getValidTargets` 返回空列表，无懈可击永远无法主动使用
- 各锦囊牌的 `execute` 方法中没有任何无懈可击检查

**修复**：
- `PendingActionInfo` 新增 `onNullifySkipped` 回调字段（`std::function<void()>`），用于在用户放弃无懈时恢复执行被延迟的锦囊效果
- `GameRule` 新增 `hasNullifyToRespond()`、`checkNullifyBeforeEffect()`、`handleNullifyResponse()` 三个函数
- 修改过河拆桥、顺手牵羊、决斗、借刀杀人、乐不思蜀、兵粮寸断六个单目标锦囊的 `execute()`，执行前先调用 `checkNullifyBeforeEffect`
- `ActionViewModel::respondCard` 和 `skipResponse` 分别添加 `CardType::Nullify` 分支处理无懈响应/跳过
- `NullifyCard::execute` 改为空实现（无懈不会主动使用，只通过 pending action 系统响应）

**流程**：
```
锦囊 → execute() → checkNullifyBeforeEffect → 目标有用无懈？
  ├─ 否 → 锦囊直接结算
  └─ 是 → 创建 pending action (Nullify) → 展示给目标玩家
           ├─ 使用无懈 → handleNullifyResponse(used) → 无懈进弃牌堆，锦囊被抵消
           └─ 跳过   → handleNullifyResponse(skipped) → 执行 onNullifySkipped 回调 → 锦囊结算
```

编译通过，4/4 核心测试全部通过。

### [2026-07-16] 决斗（Duel）结算逻辑修复

**现象**：使用决斗后，对方打出一张杀，决斗直接结束，没有交替出杀过程。

**根因**：`executeDuel` 创建了一个 `canSkip=false` 的 `CardType::Kill` 待定动作，但在 `ActionViewModel::respondCard` 和 `skipResponse` 中，`CardType::Kill` 的路由统一走 `handleAoeKillResponse`/`handleAoeSkipResponse`（南蛮入侵的 AOE 逻辑），没有为决斗做区分——对方打出一张杀后 AOE 逻辑判定"已响应，下一位"，但因为只有两名玩家，下一位不存在，流程终止。

**修复**：
- `PendingActionInfo` 新增 `bool isDuel` 标记
- `executeDuel` 设置 `isDuel = true`，`canSkip = true`（可放弃出杀承担伤害）
- 使用统一 `handleDuelResponse`：有效【杀】会被消耗并交换 source/target，空响应则由上一位出杀者造成 1 点伤害并结束决斗
- `ActionViewModel` 中 Kill 响应/跳过分支按 `isDuel` 或 `sourceCard == Duel` 路由到决斗处理函数

**正确流程**：
```
A 使用决斗 → B 需出杀 → B 出杀 → A 需出杀 → A 出杀 → B 需出杀 → B 放弃 → B 受 1 点伤害
                                                                             或 A 放弃 → A 受 1 点伤害
```

编译通过，4/4 核心测试全部通过。

### [2026-07-16] 吕布无双（requireExtraDodge）修复 + 主动技能按钮

**现象 1**：吕布的【杀】仅需一张【闪】即可抵消，不符合"需两张闪"的技能描述。

**根因**：`LvBu::requireExtraDodge()` 返回 `true`，但没有任何代码检查这个返回值——`executeKill` 和 `handleKillResponse` 完全无视了多闪需求。

**修复**：
- `PendingActionInfo` 新增 `requiredDodgeTotal` 和 `dodgeProgress`（默认 1/0）
- `executeKill` 中检测 `user->character()->requireExtraDodge()` 时设 `requiredDodgeTotal = 2`
- `handleKillResponse` 中闪计数 < requiredDodgeTotal 时，不结束待定动作，更新描述继续要求出闪
- 全闪抵消则杀被挡；任一环节放弃则受伤害

**现象 2**：孙权等武将的主动技能（制衡）无法使用，缺少触发按钮。

**修复**：
- `ActionPanelWidget` 新增"发动技能"按钮，由 `PlayerData::canUseActiveSkill` 控制显示
- `HandCardAreaWidget` 新增多选模式（`setMultiSelectMode(true)` + `selectedCardIds()`），支持制衡等需要选择多张牌的技能
- `GameBoardWidget` 新增 `skillRequested(cardIds, playerId)` 信号和选择/确认两阶段交互
- `ActionViewModel` 执行【制衡】：选择任意张手牌等量置换，每回合限一次；`SGSApp` 只建立信号连接

编译通过，4/4 核心测试全部通过。

### [2026-07-16] 玩家2弃牌阶段无反应修复

**现象**：玩家2（playerId=1）弃牌阶段，选择手牌后点击"确认弃牌"无反应。

**根因**：`GameBoardWidget::onCardClicked`（Discarding 状态）和 `onDiscardConfirmed` 均硬编码 `m_bottomHandArea`，但 player 1 的弃牌手牌位于 `m_topHandArea`。选择牌时在 `m_bottomHandArea`（实际是 player 0 的手牌区）中查找 cardId 永远找不到，`selectedCardId()` 恒返回 -1，弃牌被静默跳过。

**修复**：
- 新增 `handAreaForPlayer(int playerId)` 辅助方法，根据 playerId 返回对应手牌区
- `onCardClicked`（Discarding 分支）、`onDiscardConfirmed` 改用 `handAreaForPlayer(m_currentPlayerId)`

编译通过，4/4 核心测试全部通过。

### [2026-07-16] 判定区显示 + 延时锦囊实际进入判定区

**现象**：乐不思蜀、兵粮寸断、闪电等延时锦囊牌没有判定显示，使用后无任何视觉反馈。

**修复**：
- **Card.cpp**：`LightningCard::execute` 不再调用空实现 `GameRule::executeLightning`，改为 `user->addJudgmentCard(this)`；`HappyCard::execute` 和 `FamineCard::execute` 则在无懈判定后将牌放入目标的判定区
- **ActionViewModel::playCard**：延时锦囊（Happy/Famine/Lightning）使用后不进弃牌堆（已放入判定区）
- **PlayerData**：新增 `judgmentCards`（`QVector<CardData>`）
- **GameViewModel::pushPlayerData**：从 `Player::judgmentCards()` 读取并填充判定区数据
- **PlayerInfoWidget**：技能标签上方新增紫色"判定: 🔮 乐不思蜀 | 🔮 兵粮寸断"显示区域

**判定区界面布局**：
```
[头像] [姓名 手牌×5]
       [❤ ❤ ❤ 🖤]
       [装备: ⚔ 青龙偃月刀(范围3)] [攻击范围3]
       [判定: 🔮 乐不思蜀 | 🔮 兵粮寸断]  ← 新增
       [技能: 制衡: 出牌阶段可弃任意张牌并摸等量的牌]
```

编译通过，4/4 核心测试全部通过。

### [2026-07-16] 判定阶段完整实现（闪电/乐不思蜀/兵粮寸断）

**现象**：延时锦囊无判定效果、乐不思蜀/兵粮寸断不消失、闪电不移位。

**实现**：
- **GameViewModel**：`executePhaseJudge` 为当前玩家判定区的每张牌执行判定：
  - 从牌堆顶摸判定牌 → 展示结果1.5s → 处理效果 → 移除/移动判定牌
  - **乐不思蜀**：判定♥则通过（无效），否则跳过出牌阶段
  - **兵粮寸断**：判定♣则通过（无效），否则跳过摸牌阶段
  - **闪电**：判定♠2-9则3点伤害，否则移至下家
  - 新增 `m_skipDrawPhase`/`m_skipPlayPhase` 标记，`setNextPhase` 检测后自动跳过相应阶段
  - `onJudgeTimerFired` 1.5s 后自动推进到下一阶段
- **GameBoardWidget**：新增 `onJudgmentPerformed` 显示判定牌花色点数 + 结果文本；生效红色高亮 / 无效绿色高亮
- **SGSApp**：连接 `judgmentPerformed` 信号

**判定展示效果**：
```
判定牌: ♠7   →  【乐不思蜀】判定: 非♥，跳过出牌阶段  ← 红色高亮
或
判定牌: ♥3 桃  →  【兵粮寸断】判定: ♣，判定通过    ← 绿色高亮
```

编译通过。

### [2026-07-16] 修复 tobefixed #1 #3 #4 #6

- **#1 扣血显示**：已损失体力改为 `❌`（原 `🤍`），视觉效果更明显
- **#3 移除五谷丰登/八卦阵/麒麟弓**：从牌堆初始化中删除，但保留类定义和 createCard 分支（后续可恢复）
- **#4 移除丈八蛇矛**：从牌堆中删除（需主动发动效果的武器）
- **#6 删除IP显示**：创建房间后不再显示本机IP，仅显示"端口: 9527  等待对手连接..."

编译通过，核心测试 4/4 通过。

### [2026-07-16] 修复 #7：主动技能按钮 + 关羽武圣 + 大乔流离

- **主动技能按键**：`Character` 新增 `hasActiveSkill()`，仅关羽/大乔/孙权返回 `true`；`ActionPanelWidget` 根据 `setSkillAvailable` 控制按钮显隐
- **关羽武圣**：点击"发动技能"→选择一张红色牌→自动选择目标→当【杀】使用（通过 `useActiveSkill` + `executeKill`）
- **大乔流离**：被指定为【杀】的目标时自动转移给另一个存活角色，日志显示"被【流离】转移至 XX"

编译通过，核心测试 4/4 通过。

### [2026-07-16] 修复选将后闪退

**现象**：选择武将后进入游戏立即崩溃退出。

**根因**：`canUseActiveSkill` 调用 `skillTransformCard(nullptr)` 以检测关羽武圣。`GuanYu::skillTransformCard` 对 nullptr 走 `card->cardType()` 分支，空指针崩溃。

**修复**：`GuanYu::skillTransformCard` 增加 nullptr 检查，提前返回 `CardType::Kill`。`canUseActiveSkill` 改用已有手牌检测武将转化能力，不再传 nullptr。

编译通过，核心测试 4/4 通过。

## Features

### [2026-07-16] 修复【决斗】无法交替出【杀】导致对局卡死

**现象**：A 对 B 使用【决斗】，B 打出一张【杀】后，即使 A 手中还有【杀】也无法继续响应，
对局失去可继续操作的待定动作。

**根因**：`ActionViewModel` 将所有 `requiredCardType == CardType::Kill` 的响应统一路由到
`handleAoeKillResponse`，把【决斗】误当成【南蛮入侵】处理。B 出【杀】后 AOE 处理器会清空
待定动作，却不会创建“轮到 A 继续出【杀】”的下一步。

**修复**：

- `executeDuel` 将实际【决斗】保存到 `PendingActionInfo::sourceCard`，作为待定响应的类型标识；
  现有 `PendingActionData::sourceCardId` 可直接向 View/网络层传递，无需修改网络协议。
- `GameRule` 新增 `handleDuelResponse`：目标先出【杀】，成功后交换 `source`/`target` 并创建下一次
  【杀】响应；任意一方不出时，由上一位成功出【杀】的角色对其造成 1 点伤害。
- `ActionViewModel::respondCard`/`skipResponse` 根据 `sourceCard` 区分【决斗】与【南蛮入侵】，
  分别进入正确的响应处理器；同时将【决斗】列入必须显式指定目标的卡牌。
- 更新【决斗】卡面描述，明确“目标先出【杀】，双方轮流出【杀】，未出者受到 1 点伤害”。

**涉及文件**：`src/Model/Card.cpp`、`src/Model/GameRule.h/cpp`、
`src/ViewModel/ActionViewModel.cpp`、`tests/model_test.cpp`、`tests/viewmodel_test.cpp`、
`README.md`、`interface.md`、`connection.md`、`plan2.0.md`。

**验证**：新增 Model/ViewModel 两层 `duelAlternatingResponses` 回归用例，覆盖
A 出【决斗】 → B 出【杀】 → A 继续出【杀】 → B 不出并扣 1 点体力的完整交替链。
删除 `build` 后按 README 方案二使用 Qt 6.11.1 + MinGW 13.1 全量重建成功；
`ModelSmokeTest` 95/95，`ModelTest`/`ViewModelTest`/`ViewTest`/`NetworkTest` 逐个运行均返回 0。

**测试环境注意**：Windows 默认 PATH 中 `C:\mingw64\bin` 的 GCC 8 运行库位于 Qt 之前时，
新编译的测试程序会因旧 `libstdc++-6.dll` 缺少 `_ZSt28__throw_bad_array_new_lengthv`
而在进入测试前以 `0xc0000139` (`STATUS_ENTRYPOINT_NOT_FOUND`) 退出。这是 MinGW DLL 版本错配，
不是测试断言或决斗逻辑失败；运行前需按 README 将 Qt 6.11.1/MinGW 13.1 的 `bin` 目录放到 PATH 最前。

**文档同步**：全量核对允许修改的根目录项目文档。README 已更新为 101 张牌、9 名武将、
本地/局域网双模式和 63 个 NetworkTest 测试函数；`interface.md` 已同步公开枚举、卡牌/武将、
装备 API、牌堆和 GameRule；`connection.md` 已去除“网络层开发中”旧描述；
`plan2.0.md` 以第 0 节区分当前实现与后续设计。

### [2026-07-16] 装备牌系统 + 卡牌拓展 — View 层适配

完成装备牌在 View 层的全面适配，包括：

- **Common 层**：`CommonTypes.h` 新增 `EquipSlot` 枚举 + 补全 7 种新锦囊（决斗/闪电/无懈/借刀/五谷/乐/兵粮）及 9 种装备牌（诸葛连弩/青龙刀/丈八矛/麒麟弓/青釭剑/寒冰剑/雌雄剑/八卦阵/仁王盾）的 `CardType`；`CardData.h` 新增 `equipSlot`(int)/`attackRange` 字段；`PlayerData.h` 新增 `attackRange`/`equipCards` 字段。
- **PlayerInfoWidget**：HP 下方新增装备区行，显示 ⚔ 武器名(范围) 及 🛡 防具名；攻击距离标签（仅当前回合玩家可见）。
- **CardWidget**：装备牌特殊青绿边框（普通牌灰色）；底部标签显示 ⚔装备/+%1 或 🛡装备。
- **MainWindow**：武将选择列表从 4 人扩展到 9 人（新增孙权、周瑜、吕布、大乔、司马懿）。
- **新增 NetworkConfigDialog**：联网配置对话框（IP/端口输入），纯 UI 不依赖 Network 层类型，通过信号传出 host/port。
- **MainWindow 联网入口**：选将页面新增「创建房间（房主）」「加入房间」按钮；`createRoomRequested()` / `joinRoomRequested(host, port)` 信号，供 App 层组装网络游戏。零依赖 Network 层类型。

编译通过，4/4 核心测试（ModelSmokeTest/ModelTest/ViewModelTest/ViewTest）全部通过。

### [2026-07-16] 联网模式接线完成 — 同机双人对战可用

完成 `SGSApp` 中联网模式的完整接线，实现同机（localhost）双人联机对战：

- **MainWindow**：新增 `showCharacterSelection(playerId, playerName)` 显示单角色选将页；`showWaitingForOpponent()` 显示等待对手界面。
- **SGSApp**：`onCreateRoom()` 启动 `ServerApp`（监听 9527）+ 本地 `ClientApp`（连 127.0.0.1）；`onJoinRoom(host, port)` 创建 `ClientApp` 连接远程服务器；`onCharacterConfirmed(charId)` 发送选将；`onClientGameStarted` 切换到游戏页面；`onConnectionError`/`onNetworkGameOver` 错误恢复和游戏结束清理。
- **CMakeLists**：`SanguoshaQt` 目标加入 `ServerApp`/`ClientApp` 源文件 + 链接 `SanguoshaNetwork` + `Qt::Network`。
- **MessageSerializer**：`PlayerData` 序列化字段同步更新（`attackRange` + `equipCards`），`CardData.equipSlot` 改为 `int` 序列化。

**使用方式**：启动两个实例 → 实例1点「创建房间」→ 实例2点「加入房间」(127.0.0.1:9527) → 双方各自选将 → 游戏开始。

编译通过，4/4 核心测试全部通过。

### [2026-07-15] 网络层 Step 9：进程内端到端对局（联调 M1-M3 预演）+ 文档收尾

新增 `NetworkTest::endToEndTwoClientGameHandshakeToGameOver`：单进程内起 `ServerApp`（内含真实
`GameServer`）+ 2 个真实 `GameClient`，完整跑握手 → 选将 → `GameStarted` → 出杀（2 人局唯一目标
自动结算）→ 玩家 1 持真实闪但主动放弃响应（`skipResponse`）→ 伤害致其死亡 → 濒死救援阶段玩家 0
无桃/酒自动跳过 → `checkGameOver` 在同一次 `skipResponse` 调用内同步完成 → 双端一致收到
`gameOver(0)`（M1-M3）。游戏结束后再验证两条 M4 异常路径：非当前回合玩家出牌被
`ActionViewModel::canPlayCard` 挡住、合法发起方但游戏已结束的 `AdvanceRequested` 被
`GameViewModel::advancePhase()` 的 `isGameOver()` 早退挡住——服务器不崩溃、阶段不变。

**调试记录（真实 bug，非环境噪音）**：初版实现里，清空玩家 0 桃/酒的时机放在 Draw 阶段摸牌**之前**
——Draw 阶段会给当前玩家摸 2 张新牌，偶尔重新引入桃/酒，导致濒死响应不再自动跳过；同时未显式给
玩家 1 一张真实的闪，若随机发牌恰好没有闪，`GameViewModel::onModelPendingActionCreated` 的自动
跳过分支会让 Dodge 待定动作根本不广播到网络。这两处叠加在全套件连续运行（60+ 用例）时约有
40-50% 概率复现为断言失败或超时，一度用 `git stash` 对比 Step 8 基线（连续 6 次全绿）与加入
Step 9 后（间歇失败，且牵连到无关既有用例 `thirdConnectionRejected`）误判为"用例数增长导致 TCP
资源紧张"的环境级 flakiness。定位真实根因（手牌构造的非确定性）后修正：清理桃/酒挪到摸牌之后、
显式给玩家 1 `addHandCard` 一张确定的闪，连续 12 次全量运行本用例 12/12 通过。

另在 `NetworkTest` 新增 `cleanup()` 私有槽，每个用例结束后冲刷一次事件循环（处理
`GameServer::dropClient()` 里 `deleteLater()` 的连接对象），作为全套件资源卫生的通用改进保留
（非本次具体失败的根因）。

**涉及文件**：`tests/network_test.cpp`（+1 新用例 + `cleanup()` 私有槽）、`connection.md` §7.8
（新建）、`CLAUDE.md`（Step 9 打勾）、`README.md`（用例数 60→61）、`plan2.0.md`（补充落地细节）。

**验证**：`endToEndTwoClientGameHandshakeToGameOver` 单独运行与连续多次全量运行均稳定通过；
全套件 5/5（`ModelSmokeTest`/`ModelTest`/`ViewModelTest`/`ViewTest`/`NetworkTest`）通过。
网络层 Step 1-9 全部完成，真机双开手动联调留给三人联调周（plan2.0.md §7.5 M1-M4）。

### [2026-07-15] 网络层 Step 8：心跳保活

`GameServer` 新增 `m_heartbeatTimer`（`QTimer`，默认周期 3000ms）：每次触发给所有已握手客户端
发一条 `Ping`；每个 `ClientSlot` 新增 `lastSeenTimer`（`QElapsedTimer`），`onSocketReadyRead`
每次收到任意帧（命令帧或 `Pong`）都 `restart()`，`onHeartbeatTick()` 检查若某客户端超过
`m_heartbeatTimeoutMs`（默认 10000ms）未续期则判定失联，走与正常断线相同的
`dropClient` + `emit clientDisconnected` 路径。`GameClient::dispatchMessage` 新增
`MessageType::Ping` 分支，收到后立即回 `Pong`（纯网络层应答，不 emit 任何对外信号）。

新增 `GameServer::setHeartbeatIntervalMs`/`setHeartbeatTimeoutMs` 两个方法，供测试注入短参数
（生产代码路径不调用，`ServerApp`/`ClientApp` 均使用默认值）。未握手完成的连接不计入心跳。
断线重连不做（首版明确砍掉，见 plan2.0.md §2.7）。

**涉及文件**：`src/Network/GameServer.h/cpp`（心跳定时器、`ClientSlot::lastSeenTimer`、
`onHeartbeatTick`）、`src/Network/GameClient.cpp`（`Ping` 分支自动回 `Pong`）、
`tests/network_test.cpp`（+2 新用例）、`connection.md` §7.7（新建）、`CLAUDE.md`（Step 8 打勾）、
`plan2.0.md` §2.7（补充落地细节说明）。

**验证**：`unresponsiveClientIsKickedAfterHeartbeatTimeout`——裸 `RawClient` 握手后不发任何帧
模拟卡死，短参数（100ms/300ms）下断言其被判定失联并踢出；
`respondingClientSurvivesHeartbeatViaGameClientAutoPong`——真实 `GameClient` 跨越多个心跳周期
后仍保持连接，验证自动 `Pong` 生效、正常客户端不被误踢。NetworkTest 增至 60 用例，全套件
5/5（`ModelSmokeTest`/`ModelTest`/`ViewModelTest`/`ViewTest`/`NetworkTest`）通过。

### [2026-07-15] 网络层 Step 7：ClientApp 组合根

新建 `src/App/ClientApp.h/cpp`：网络模式的客户端组合根，与本地模式的 `SGSApp::startLocalGame()`
完全对称——创建 `GameBoardWidget` + `GameClient`，在构造函数里按逐条相同的形状建立双向信号连接
（7 条 View→Client 命令信号 + 9 条 Client→View 状态信号）。零业务逻辑，只额外暴露
`connectToServer(host, port)`/`selectCharacter(int)` 两个转发方法，以及 `boardWidget()`/
`gameClient()` 两个访问器供调用方嵌入自己的窗口容器、接入登录/等待界面（`MainWindow` 联网入口和
`NetworkConfigDialog` 属丙，不在本计划内）。

**验收标准达成：`GameBoardWidget` 零改动。** 能做到零改动的原因是设计层面的——`GameBoardWidget`
从不知道、也不需要知道对面接的是本地 `GameViewModel`/`ActionViewModel` 还是网络 `GameClient`，
因为 `GameClient` 对外信号/槽形状与本地 VM 完全一致（Step 6 的设计前提）。

**涉及文件**：`src/App/ClientApp.h/cpp`（新建）、`CMakeLists.txt`（`CLIENT_APP_SOURCES`/
`CLIENT_APP_HEADERS` 变量组，`NetworkTest` 加 `Qt::Widgets` 链接）、`tests/network_test.cpp`
（+1 新用例，`QTEST_GUILESS_MAIN`→`QTEST_MAIN`）、`connection.md` §7.6（新建）、`CLAUDE.md`
（Step 7 计划项打勾）、`README.md`（目录结构 + 测试运行方式 + 测试用例数）。

**测试注意点**：`clientAppWiresBoardToGameClientWithZeroBoardChanges` 需要真实
`GameBoardWidget`（`ClientApp` 内部创建），因此本用例的存在把整个 `NetworkTest` 目标从
`QTEST_GUILESS_MAIN`（仅 `QCoreApplication`）切到 `QTEST_MAIN`（`QApplication`），CTest 侧沿用
`ViewTest` 的做法设置 `QT_QPA_PLATFORM=offscreen` 环境变量，避免依赖桌面显示服务器。

**验证**：`clientAppWiresBoardToGameClientWithZeroBoardChanges`——对接真实 `ServerApp`，起一个
`ClientApp`（真实 `GameBoardWidget`）和一个裸 `GameClient`（扮演对手，无 View）完成握手→选将→
驱动到 Play 阶段，用 `QTest::mouseClick` 真实点击 `ClientApp::boardWidget()` 里注入的杀对应的
`CardWidget`，断言服务器侧 `Player::handCards()` 确实移除了这张牌——覆盖从真实 QWidget 点击到
网络命令到服务器结算的完整链路，而不只是信号连接的存在性。NetworkTest 增至 58 用例，全套件
5/5（`ModelSmokeTest`/`ModelTest`/`ViewModelTest`/`ViewTest`/`NetworkTest`）通过。

### [2026-07-15] 网络层 Step 6：GameClient（"网络化 ViewModel"）

新建 `src/Network/GameClient.h/cpp`：客户端网络层，与服务器侧 `GameServer`（"网络化的 View"）对称，
定位是"网络化的 ViewModel"——对外暴露与 `GameViewModel`/`ActionViewModel` 完全相同形状的信号（内容
由收到的服务器广播反序列化后直接 `emit`，不经过任何本地 Model/规则计算），发送方法与
`GameBoardWidget` 的命令信号一一对应（只把参数打包发给服务器，不做任何本地校验或判断）。本类零
Model 依赖、零规则判断，符合 plan2.0.md §2 对网络层的约束（不持有/传递 `Model::Player*`/
`Model::Card*`，跨网络只传 Common 值类型）。

**方法**：`connectToServer(host, port)`/`disconnectFromServer()`/`isConnected()`/
`localPlayerId()`/`selectCharacter(int)`/`playCard`/`respondCard`/`selectTarget`/`discardCard`/
`endPlayPhase`/`advancePhase`/`skipResponse`。

**信号**：与 VM 相同形状的 `phaseChanged`/`playerDataUpdated`/`handCardsUpdated`/
`pendingActionCreated`/`pendingActionCleared`/`gameOver`/`logMessage`/`targetSelectionStarted`/
`targetSelectionFinished`；另加本地模式无对应物的网络生命周期信号 `connected(playerId)`/
`connectionRejected(reason)`/`connectionError(error)`/`disconnected()`/`gameStarted(char0, char1)`。

按 plan2.0.md §2.4 的结论，未新建 `RemoteGameVM`/`PlayerVMAdapter` 只读查询适配层——
`GameBoardWidget` 从不主动调用 VM 方法拉取状态，全部状态通过槽被动接收，`GameClient` 只需转发信号。

**涉及文件**：`src/Network/GameClient.h/cpp`（新建）、`CMakeLists.txt`（`NETWORK_SOURCES`/
`NETWORK_HEADERS` 加入 GameClient）、`tests/network_test.cpp`（+6 新用例）、`connection.md` §7.5（新建）、
`CLAUDE.md`（Step 6 计划项打勾）、`README.md`（目录结构 + 测试用例数）。

**测试注意点**：custom 值类型（`PhaseType`/`PlayerData`/`CardList`/`PendingActionData`）未注册 Qt
元类型，`QSignalSpy` 对这些信号的 `QVariant` 提取会失败——测试里改用 `connect` 到本地 lambda 直接捕获
参数，而不是走 `QSignalSpy::at(0).at(N)`（`connectionRejected`/`gameStarted` 等只含 `int`/`QString`
基础类型的信号仍可用 `QSignalSpy`）。首版实现里 `handSizeBefore` 曾在 `Draw` 阶段推进之前采样，
导致断言的"出牌后手牌数-1"目标值本身就不对（摸牌阶段会先摸 2 张），复现即改在 `Draw → Play` 之后
采样，与既有测试（`playCardCommandUsesConnectionIdentityNotClaimedPlayerId` 等）的写法保持一致。

**验证**：`gameClientHandshakeAssignsPlayerIdsAndRejectsThirdConnection`（对接真实 `GameServer`：
playerId 0/1 分配、第三连接 `connectionRejected`）、`gameClientSendsAllCommandsWithCorrectPayload`
（7 个发送方法逐一断言服务器收到的 `MessageType` 与 payload 字段）、
`gameClientReceivesGameStartedAndBroadcastsWithRedaction`（对接真实 `ServerApp`：`gameStarted`/
`phaseChanged`/`playerDataUpdated`/`logMessage` 正常 emit，`handCardsUpdated` 脱敏转发正确——己方
完整、对方占位）、`gameClientPlayCardRoundTripReflectedInBothClients`（两个 `GameClient` 出杀 →
双方收到 `pendingActionCreated(Dodge)`）、`gameClientDisconnectedSignalFiresWhenServerCloses`
（服务器关闭 → `disconnected()` emit，`localPlayerId()` 复位为 -1）。NetworkTest 增至 57 个用例，
12/13 次连续直接运行 + 全套件运行通过（1 次遇到 `thirdConnectionRejected`——Step 3 起就存在的
loopback 连接建立偶发慢速——的既有已知抖动，与本次改动无关，重跑即通过）。

### [2026-07-15] 网络层 Step 5：手牌脱敏

`GameServer` 广播 `HandCardsUpdated` 此前对两端一视同仁：`ServerApp::wireViewModelBroadcasts()`
里这条槽直接用 `GameServer::broadcast` 把同一份完整 `CardList` payload 发给两个客户端，对手能看到
己方的完整手牌牌面——网络对战最基本的信息安全前提被破坏（P0，不可砍）。

**修复方式**：

- 新增 `Protocol::redactCardList(const CardList&)`（`src/Network/Protocol.h/cpp`，纯函数、无 Model 依赖）：对每张 `CardData` 只保留 `cardId`/`ownerId`（结构信息），其余牌面字段（`cardType`/`suit`/`number`/`cardName`/`description`/`color`/`isBasic`/`isStrategy`/`suitSymbol`/`numberString`/`isEquipment`/`equipSlot`/`attackRange`/`isSelected`/`isPlayable`/`isHighlighted`）重置为默认构造的占位值，列表长度不变。
- `ServerApp::wireViewModelBroadcasts()` 的 `handCardsUpdated` 槽不再用 `broadcast`，改为两次 `GameServer::sendTo`：所有者本人收未经处理的完整 `CardList`，对手收 `redactCardList` 处理后的结果。`HandCardsMsg::playerId` 字段语义仍是"这是谁的手牌"，与"收件人是谁"无关。

**涉及文件**：`src/Network/Protocol.h`（+声明）、`src/Network/Protocol.cpp`（新建）、`CMakeLists.txt`（`NETWORK_SOURCES` 加入 `Protocol.cpp`）、`src/App/ServerApp.h/cpp`、`tests/network_test.cpp`（+2 新用例）、`connection.md` §7.1/7.1.1、`CLAUDE.md`（Step 5 计划项打勾）、`README.md`（测试用例数 + 目录结构说明）。

**验证**：`redactCardListKeepsIdentityDropsFace`（纯函数单测：cardId/ownerId 保留，牌面字段全部占位）+ `handCardsBroadcastRedactedForOpponentFullForOwner`（两个真实 TCP 连接跑完整对局：己方视角至少一张牌有真实牌名，对手视角牌名/描述/装备标记全部占位，双方 cardId 集合排序后一致）。NetworkTest 增至 50 个用例，10 次连续直接运行 + 3 次全套件运行全部通过，无抖动（其中一次单跑遇到过一次超时式失败，重跑及后续 10 次均通过，判定为 loopback 下的偶发调度抖动，非新增代码引入的问题）。

### [2026-07-15] 网络层 Step 4 加固第三轮：出牌/弃牌回合归属校验 + 待定动作重入保护

修复前两轮审查确认、但当时判定"边界超出网络接线、属甲的规则域"而搁置的两处深层规则缺口，用户决定现在修复：

- **出牌/弃牌全链路不校验 `player == currentPlayer()`（critical，两轮独立审查都发现）**：`ActionViewModel::canPlayCard`/`playCard`/`discardCard` 和各 `Card::canUse` 子类只查全局 `currentPhase()`，从不检查调用者是不是当前回合玩家。本地模式靠 View 只让当前玩家手牌可交互侥幸掩盖，网络模式下非当前回合玩家能用自己真实（非伪造）身份在对方回合出/弃自己的牌。
- **`GameState::setPendingAction` 无重入保护（critical，两轮独立审查都发现）**：即使调用者确实是当前回合玩家，出牌路径原先也不检查 `!hasPendingAction()`——当前回合玩家出杀后，趁对方的闪响应尚未结算，可以紧接着再出一张主动牌（如南蛮入侵/万箭齐发），`GameRule::executeKill`/`executeBarbarianInvasion` 等会无条件调用 `setPendingAction` 覆盖掉第一个待定动作，原来的闪响应永久悬空。

**修复方式（刻意避免触碰 `Card.cpp`/`GameRule.cpp`/`GameState.cpp`，与甲的装备牌开发保持文件隔离）**：在 `ActionViewModel::canPlayCard` 入口补两条早退校验——`player != m_state->currentPlayer()` 和 `m_state->hasPendingAction()`；`ActionViewModel::discardCard` 补 `player != m_state->currentPlayer()`。三个校验点位于 `canPlayCard`/`discardCard` 内部，同时保护 View 命令入口（`onPlayCardRequested`）和直接调用（`playCard`），本地模式行为不受影响（本地模式下这些条件恒成立）。

**涉及文件**：`src/ViewModel/ActionViewModel.cpp`、`tests/network_test.cpp`（+3 新用例）、`connection.md` §7.2/§7.3、`CLAUDE.md`（Step 4 计划项更新）。

**验证**：NetworkTest 50 个用例（47 + 3 新增），5 次连续直接运行 + 全套件运行全部通过，无抖动。

### [2026-07-15] 网络层 Step 4 加固第二轮：advancePhase 悬空待定动作 + GameStarted 时序

对第一轮加固后的代码做了第二轮独立对抗性审查（复用第一轮的 5 个视角 + 2 个之前因 API 中断未完成的维度），确认了以下额外问题并修复：

- **`advancePhase()` 离开 Play 阶段不检查 `hasPendingAction()`（critical，两轮独立审查都发现）**：与 `endPlayPhase()`（已有此检查）不对称。当前回合玩家出杀后，若紧接着发送 `AdvanceRequested`，阶段会被强制推进到 Discard，而闪的待定响应永久悬空——本地模式下 `AutoAdvanceTimer` 从不在 Play 阶段启动所以从未暴露，网络模式下当前回合玩家可以随时发送此命令。修复：`advancePhase()` 的 `Play` 分支补上与 `endPlayPhase()` 一致的 `hasPendingAction()` 检查。
- **`GameStarted` 在校验 characterId 之前就广播（major）**：非法 characterId 会让客户端先收到"游戏已开始"，之后却永远等不到任何后续状态。修复：新增 `GameViewModel::isValidCharacterId()` 静态校验，`ServerApp::onBothPlayersReady` 在广播前先校验，非法 id 直接不广播（协议暂无专门的拒绝消息类型，留待后续协议版本）。

**测试补充**（响应同一轮审查指出的覆盖缺口）：`RespondCardRequested` 的身份伪造防护此前无专门测试（与 PlayCard/Discard 不对称）；`discardCommandUsesConnectionIdentityNotClaimedPlayerId` 只断言手牌数量、未直接断言 `currentPhase` 这个真正被保护的不变量；`playCardCommandUsesConnectionIdentityNotClaimedPlayerId` 依赖随机发牌恰好有可出的牌（理论有极低概率抖动）。均已修复/补充。

**涉及文件**：`src/ViewModel/GameViewModel.h/cpp`（`advancePhase` 加固 + `isValidCharacterId`）、`src/App/ServerApp.cpp`、`tests/network_test.cpp`（+3 新用例，2 处存量用例加固）

**验证**：NetworkTest 47 个用例，10 次连续直接运行 + 1 次全套件运行全部 47/47、5/5 通过，无抖动复现。

**审查再次确认、仍未修的深层规则缺口（与第一轮相同结论：属甲的卡牌/规则域）**：① `ActionViewModel::canPlayCard`/`playCard`/`discardCard` 全链路不校验 `playerId == currentPlayer()`——非当前回合玩家能用自己真实身份出/弃自己的牌（两轮审查独立确认，critical/major）。② `GameState::setPendingAction` 无重入保护，`canPlayCard`/`onPlayCardRequested` 不检查 `!hasPendingAction()`——即使不借助 ①，仅靠同一当前回合玩家连续出两张牌（如张飞无限杀）也能让第二个待定动作静默覆盖第一个（两轮审查独立确认为 critical）。这两处都需要改 `Card.cpp`/`GameRule.cpp`/`ActionViewModel.cpp` 的核心规则判断逻辑，边界超出"网络接线"范畴，待用户决定由谁在哪一步修复。

### [2026-07-15] 网络层 Step 4 加固：对抗性代码审查后修复越权/状态破坏问题

对 Step 4 的接线代码做了一轮多视角对抗性审查（每条发现都经独立二次核实），修复了其中网络层可安全处理、不触及本地 View/SGSApp 的几处真实问题：

- **越权命令（critical）**：`EndPlayRequested`/`AdvanceRequested`/`SkipRequested` 三个无参命令原先在 `ServerApp::onClientCommandReceived` 里被直接转发、完全不校验发起方身份，导致任意已连接客户端都能提前结束/越权推进对方的回合，或替对方强制放弃响应（即使对方有闪可用）。修复：`EndPlay`/`Advance` 校验 `playerId == currentPlayerId()`，`Skip` 校验 `playerId == pendingResponderId()`（新增 `GameViewModel::pendingResponderId()` 只读辅助，返回 int 不暴露 Model 指针）后才转发。
- **重复 SelectCharacter 销毁对局（critical）**：`GameServer::ClientSlot::selectedCharacterId` 在开局后不清空，已连接客户端在对局进行中重发一条 `SelectCharacter` 会让 `bothPlayersReady` 二次触发、`startHeadlessGame` 无条件丢弃当前整局重开。修复：`ServerApp::onBothPlayersReady` 开头加 `if (isGameRunning()) return;` 防重放。
- **非法 characterId 破坏服务器状态（critical）**：`SelectCharacterMsg::characterId` 是客户端可任意填的字段，非法 id 会让 `GameViewModel::startGame` 在 `initGame` 之前 return，留下"`m_gvm` 非空但从未初始化"的僵尸状态而 `isGameRunning()` 仍返回 true。修复：`startGame` 改为返回 `bool`（非法 id 返回 false 并 delete 已建的 Character），`ServerApp::startHeadlessGame` 据此回滚 `m_gvm` 为 nullptr。

**涉及文件**：`src/App/ServerApp.cpp`、`src/ViewModel/GameViewModel.h/cpp`（`startGame` 返回值 + `pendingResponderId()`）、`tests/network_test.cpp`（4 个加固回归用例）

**验证**：NetworkTest 45 个用例；新增 `skipRequestedFromWrongPlayerIsRejected`、`endPlayAndAdvanceFromWrongPlayerAreRejected`、`repeatedSelectCharacterAfterGameStartedIsIgnored`、`invalidCharacterIdDoesNotCorruptServerState`。10+ 次连跑仅复现既有的 `thirdConnectionRejected` 环境抖动（loopback 第三连接偶发超时，与本次改动无关）。

**审查另外确认了两处更深层的规则层缺陷（暂未修，属甲的卡牌/规则域，待用户定夺归属）**：① `ActionViewModel::canPlayCard`/`playCard`/`discardCard` 和 `Card::canUse` 全链路只查全局 `currentPhase()`，从不校验 `playerId == currentPlayer()`——非当前回合玩家能在对方回合出自己的牌/弃自己的牌（本地模式靠 View 只让当前玩家手牌可交互而侥幸掩盖，网络模式直接暴露）。② `GameState::setPendingAction` 无条件覆盖，加上出牌路径不查 `!hasPendingAction()`，能让第二个待定动作静默冲掉第一个、原响应卡死。这两处需改核心 GameRule/Card/ViewModel（甲的域），且与"网络接线"边界不同，留待协调。

### [2026-07-15] 网络层 Step 4：ServerApp 接线 GameServer（plan2.0.md §2.3-2.4）

`ServerApp` 现在持有 `GameServer` 并双向接线：`wireViewModelBroadcasts()` 把 `GameViewModel`/`ActionViewModel` 对外的全部 9 条信号（对照 connection.md 第 1 节逐条映射）连到序列化广播；`onClientCommandReceived` 把客户端的 7 类命令消息反序列化后分发到 VM 的 public slots。`onBothPlayersReady` 先广播 `GameStarted` 再调用 `startHeadlessGame`，保证客户端收到顺序是 `GameStarted → PhaseChanged → ...`（`startHeadlessGame` 内部改为先接线再 `startGame()`，避免开局的初始状态推送在无人监听时被丢弃）。

**安全加固**：`PlayCardMsg`/`RespondCardMsg`/`DiscardCardMsg` 里客户端消息体自称的 `playerId`/`responderId` 字段一律被忽略，改用 `GameServer` 基于 TCP 连接分配的可信 `playerId`——否则一个连接可以在消息里冒充另一个玩家操作对方手牌。`TargetSelectedMsg` 因 `ActionViewModel::onTargetSelected` 签名不带 `playerId` 暂无法在网络层补这层校验，已记录为已知限制留给 Step 9。

`MessageSerializer` 新增 `encodePayload<Msg>`/`wrapFrame`（裸 payload 序列化 + 补帧头分离），`encode<Msg>` 重构为二者组合；`GameServer::sendTo` 改用 `wrapFrame` 消除重复的帧头拼装代码。

**顺带修复**：`GameViewModel::onDiscardCardRequested` 存在一处预先潜伏的 bug——原逻辑按"请求方自己的 `playerId`"查 `getDiscardCount()` 判断是否 `advancePhase()`，本地模式下这个方法只会被"当前弃牌阶段的玩家"调用所以从未暴露；网络模式下任意已连接客户端发一条名义上合法（`playerId` 是自己真实的，只是不该在这个时机弃牌）的 `DiscardCardRequested`，会导致提前结束**另一个玩家**的弃牌阶段。修复为只有 `playerId == currentPlayerId()` 时才允许推进阶段，本地模式行为不受影响。

**涉及文件**：`src/App/ServerApp.h/cpp`、`src/Network/MessageSerializer.h/cpp`、`src/Network/GameServer.cpp`、`src/ViewModel/GameViewModel.cpp`、`tests/network_test.cpp`、`connection.md`（新增 §7 网络模式连接表）

**验证**：7 个新用例（GameStarted 广播顺序、PlayCardRequested/DiscardCardRequested 身份伪造防护 + 合法路径、AdvanceRequested/EndPlayRequested 网络分发、RespondCardRequested/SkipRequested 完整响应流程），用 `viewmodel_test.cpp` 已有的"手动注入确定性杀/闪卡牌"手法绕开随机发牌以保证测试确定性。NetworkTest 41 个用例，全套件 5/5 通过（含多次重复运行确认无抖动；`thirdConnectionRejected` 出现过一次孤立超时，重跑 8 次未复现，判定为环境抖动而非回归）。

### [2026-07-15] 网络层 Step 3：GameServer 连接管理与握手（plan2.0.md §2.3）

新建 `src/Network/GameServer.h/cpp`：QTcpServer 监听（`listen(port, localOnly)`，`localOnly=true` 绑 127.0.0.1 供测试用、不触发 Windows 防火墙弹窗；生产默认绑 Any）、最多 2 连接、playerId 0/1 按空槽分配、逐客户端 recvBuffer 帧解码（流损坏即断开）、`Handshake`（含协议版本校验）→ `HandshakeAck` → `SelectCharacter` → `bothPlayersReady` 流程、超员连接回 `Ack(playerId=-1, "房间已满")` 后断开、断线释放槽位可重连。未握手的命令/选将一律忽略。命令消息经 `clientCommandReceived` 信号转出（Step 4 由 ServerApp 分发到 VM public slots）。

**涉及文件**：`src/Network/GameServer.h/cpp`、`tests/network_test.cpp`、`CMakeLists.txt`

**验证**：7 个新用例（playerId 分配、第三连接拒绝、版本不符拒绝、双方选将触发 ready、断线重连复用槽位、未握手命令忽略、损坏流断开），裸 QTcpSocket 连 loopback、`QTest::qWaitFor` 驱动事件循环（不能用 `waitForConnected`——同进程测试会阻塞主循环导致 QTcpServer 收不到连接）。全套件 5/5 通过；曾观察到重链接后首跑一次超时（疑似 Defender 扫描新 exe 延迟），已把测试超时提高到 8s，此后 10+ 次连跑稳定。

### [2026-07-15] 网络层 Step 2：ServerApp headless 启动路径（plan2.0.md §2.3）

新建 `src/App/ServerApp.h/cpp`：`startHeadlessGame(charId1, charId2)` 创建完整 `GameViewModel`（含 ActionViewModel/Model），不创建任何 QWidget；重复开局用 `deleteLater()` 释放上一局对象图（与 `SGSApp` 一致）；`gameOver` 透传为 `gameFinished(winnerId)` 信号，VM 保留到下局开始（末批状态推送需经事件循环送达）。暴露 `gameViewModel()`/`actionViewModel()` 供 Step 4 的 GameServer 命令分发使用。

**涉及文件**：`src/App/ServerApp.h/cpp`、`tests/network_test.cpp`、`CMakeLists.txt`（NetworkTest 目标加入 ViewModel/ServerApp 源与 SanguoshaModel 链接）

**验证**：NetworkTest 在 `QTEST_GUILESS_MAIN`（无 QApplication）下运行——headless 启动、VM 信号活性（phaseChanged/handCardsUpdated/playerDataUpdated）、完整回合循环（Prepare→…→弃牌→End→切换玩家）、重复开局释放旧 VM 共 4 个新用例；全套件 5/5 通过。

### [2026-07-15] 网络层 Step 1：协议定义 + 消息序列化 + 帧封装（plan2.0.md §2）

新建 `src/Network/Protocol.h`（协议版本号 v1、`MessageType` 枚举、消息结构体，字段与 `GameViewModel`/`ActionViewModel` 的 public slots/signals 一一对齐）和 `src/Network/MessageSerializer.h/cpp`（`CardData`/`PlayerData`/`PendingActionData` 的 `QDataStream <<`/`>>`，含甲新增的装备字段；帧格式为 quint32 长度前缀 + quint8 类型 + payload，`decodeFrames` 处理半包/粘包并拒绝损坏的长度前缀）。QDataStream 版本固定为 `Qt_5_15` 保证两端一致。

CMake 新增 `Qt::Network` 组件、`SanguoshaNetwork` 静态库和 `NetworkTest` 测试目标。

**涉及文件**：`src/Network/Protocol.h`、`src/Network/MessageSerializer.h/cpp`、`tests/network_test.cpp`、`CMakeLists.txt`

**验证**：`NetworkTest` 21 个用例通过（全部消息类型帧级 round-trip、逐字节半包、三帧粘包 + 半帧残留、损坏长度前缀拒绝）；全套件 5/5 通过。

## Bug Fixes

### [2026-07-14] 修复濒死响应、权限校验、目标选择和旧局生命周期

- 致命伤害进入濒死后不再清掉救援待定动作；`checkDeath()` 会发出一次 `Player::died` 信号。
- 响应牌必须属于当前响应者并匹配要求牌型；濒死响应者统一使用 `source`，且【酒】可用于救援。
- 出牌、弃牌和响应接口补充持有者校验；过河拆桥/顺手牵羊没有合法目标时不会被使用或消耗。
- 多目标出牌等待界面选择目标，并同步恢复/清理界面目标选择状态。
- 新局开始或对局结束时对旧 `GameViewModel` 使用 `deleteLater()`，避免旧对象图泄漏。

**验证**：`ModelSmokeTest`、`ModelTest`、`ViewModelTest`、`ViewTest` 共 4/4 通过，已移除对应的预期失败断言。

### [2026-07-14] 补充分层自动化测试

新增 Qt Test 测试目标，覆盖 Model、ViewModel、View 和 App 组合根，并通过 CTest 统一运行。View 测试在 `QT_QPA_PLATFORM=offscreen` 环境下执行，避免依赖桌面显示服务器。

**涉及文件**：`tests/model_test.cpp`、`tests/viewmodel_test.cpp`、`tests/view_test.cpp`、`CMakeLists.txt`

**验证**：新增测试目标最初共 4/4 通过；本轮已将此前跟踪的缺陷断言改为正常断言并完成修复。

### [2026-07-14] 出牌阶段无法主动使用武将转化技能（武圣/龙胆）

**现象**：关羽无法将红色牌当【杀】主动使用，赵云无法将【闪】当【杀】主动使用。转化牌在出牌阶段不高亮、双击无反应。（响应阶段的转化一直正常，如用红牌响应南蛮入侵。）

**根因**：`Character::skillTransformCard` 只接入了响应路径（`ActionViewModel::getResponseCardIds`、`GameRule::hasDodgeToRespond`/`hasKillToRespond`），出牌路径的三个入口全部只认卡牌本体：

1. `ActionViewModel::getPlayableCards` / `canPlayCard` 只检查 `card->canUse()`——关羽的红闪（`DodgeCard::canUse` 恒 false）和赵云的闪永远不可出
2. `ActionViewModel::getValidTargetIds` 调用 `card->getValidTargets()`——闪的目标列表恒为空
3. `ActionViewModel::playCard` 直接 `card->execute()`——即使放开校验，执行的也是原牌效果而非杀的结算

张飞（咆哮，`GameRule::canPlayKill` 已特判）和曹操（奸雄，被动触发）不受影响。

**修复**：`ActionViewModel` 新增私有判定 `playsAsKill(card, player)`（条件：出牌阶段、玩家存活、卡牌本体不可用、技能转化结果为杀、通过 `canPlayKill` 出杀次数检查），并接入出牌路径三个入口：

1. `getPlayableCards` / `canPlayCard`：本体不可用时，`playsAsKill` 成立即视为可出（高亮 + 校验放行）
2. `getValidTargetIds`：转化为杀时按杀的目标规则返回（除自己外的存活角色）
3. `playCard`：转化为杀时不执行原牌 `execute()`，改走 `GameRule::executeKill()` 结算（酒加成、闪响应流程与真杀一致），并输出"发动【武圣】，将【X】当【杀】使用"日志

**设计取舍**：本体可用时优先按本体使用——如关羽缺血时的红桃按桃使用，不能当杀（满血时红桃本体不可用，可当杀）。当前 UI 是双击出牌，没有"选择用法"的交互，此规则保证行为确定；后续如加装备/技能按钮 UI 可再放开。

**涉及文件**：`src/ViewModel/ActionViewModel.h/cpp`

**验证（当时）**：构建通过，冒烟测试 95/95；当时仍需手动运行 `SanguoshaQt.exe` 用关羽/赵云实测主动转化出杀。当前已补充分层 Qt Test，但关羽/赵云主动转化的端到端断言仍需单独补齐。

### [2026-07-14] 选将后点击「开始对战」闪退

**现象**：选择武将后点击开始对战，程序短暂无响应后异常中止。

**根因**：`MainWindow::onStartGame()` 中 `m_bootstrap->boardWidget()` 在 `startLocalGame()` 之前调用，此时 GameBoardWidget 尚未创建，
返回 `nullptr`。随后 `m_centralStack->addWidget(nullptr)` 导致 Qt 异常退出。

**修复**：将 `boardWidget()` 移到 `startLocalGame()` 之后。

**涉及文件**：`src/View/MainWindow.cpp`

### [2026-07-14] 卡在准备阶段，无法进入出牌

**现象**：游戏启动后始终停留在准备阶段，自动阶段不推进。

**根因**：`GameState` 构造函数中 `m_currentPhase` 默认值已是 `Prepare`。`initGame()` 调用 `setCurrentPhase(Prepare)` 时因为新旧值相同，条件判断 `if (m_currentPhase != phase) { emit phaseChanged(…); }` 不成立，不发射信号。`GameBoardWidget::onPhaseChanged` 从未被调用，自动推进计时器从未启动。

**修复**：在 `initGame()` 中 `setCurrentPhase(Prepare)` 之后，强制 `emit phaseChanged(Prepare)` 启动流程。

**涉及文件**：`src/ViewModel/GameViewModel.cpp`

### [2026-07-14] 双方手牌显示为牌背

**现象**：进入游戏后双方手牌均显示为红色牌背花纹，看不到牌面。

**根因**：`GameBoardWidget::onHandCardsUpdated` 调用 `m_bottomHandArea->setCards(data, false)` 传入了 `false`。`HandCardAreaWidget::setCards` 的第二参数已从原 `isOpponent` 重构为 `faceUp`，`false` 表示牌背。

**修复**：两处调用均改为 `setCards(data, true)`，同时修改 `HandCardAreaWidget` 的默认值 `faceUp = true`。

**涉及文件**：`src/View/GameBoardWidget.cpp`、`src/View/HandCardAreaWidget.h`

### [2026-07-14] 运行一段时间后无响应卡退

**现象**：出牌/响应流程中程序突然无响应后异常退出。

**根因**：`HandCardAreaWidget::clearWidgets()` 使用 `delete w` 同步销毁旧 `CardWidget`。
当用户在出牌阶段点击一张牌时调用链为：

```
CardWidget::mousePressEvent → emit clicked(cardId)
  → HandCardAreaWidget::onCardWidgetClicked → emit cardClicked(cardId)
    → GameBoardWidget::onCardClicked → emit playCardRequested(cardId, pid)
      → GameBootstrap::onPlayCardRequested → avm->playCard(…)
        → 卡牌从手牌移除 → emit handCardsChanged
          → GameViewModel::pushHandCards → emit handCardsUpdated
            → GameBoardWidget::onHandCardsUpdated
              → m_bottomHandArea->setCards(…) → clearWidgets()
                → delete w — 正在处理 mousePressEvent 的 CardWidget 自身！
```

`delete w` 在对象自身的方法调用栈执行期间销毁对象，导致 return 时 use-after-free 崩溃。

**修复**：`clearWidgets()` 中 `delete w` 改为 `w->deleteLater()`，推迟到事件循环空闲时再销毁旧控件。
此时 `CardDisplayData` 是值类型，旧控件即使暂存也有独立数据副本，不会访问已释放内存。

**涉及文件**：`src/View/HandCardAreaWidget.cpp`

### [2026-07-14] 待定动作阶段卡死 / 必须打出响应牌

**现象**：两种表现——
1. 无闪/杀可响应时，游戏卡在响应阶段无法继续
2. 有响应牌时必须打出，无法选择跳过承担后果

**根因**：
- 无响应牌时，`ActionPanelWidget::updateForPendingAction` 因 `canSkip=false` 隐藏了「跳过」按钮，且没有自动推进逻辑
- 有响应牌时，虽然「跳过」按钮不可见，但即使能访问到也会因为 `ActionViewModel::skipResponse(responderId, false)` 中 `if (!forceNoCard && !info.canSkip) return;` 直接返回，不推进游戏

**修复**：
1. `ActionPanelWidget::updateForPendingAction` 始终显示「跳过」按钮，让玩家自由选择
2. `GameBootstrap::onSkipRequested` 调用 `skipResponse(responderId, true)` 强制推进（`forceNoCard=true`）
3. 新增 `GameBootstrap::onPendingActionFromVM` 拦截 `pendingActionCreated` 信号：无响应牌时自动跳过，有牌时转发给 View 显示响应界面

**涉及文件**：
- `src/View/ActionPanelWidget.cpp`
- `src/App/GameBootstrap.h/cpp`

### [2026-07-14] 响应阶段点击响应牌无效

**现象**：需要打出响应牌（如杀后的闪）时，点击手牌无反应。

**根因**：`GameBoardWidget::onCardClicked` 在 `State::Responding` 分支中不能直接使用当前回合玩家作为响应者；普通响应的响应者是动作 `target`，濒死救援的响应者是动作 `source`。

**修复**：在 `onPendingActionCreated` 中按要求牌型记录响应者 ID：普通响应使用 `targetId`，濒死救援使用 `sourceId`；`ActionViewModel` 再校验动作目标、牌型和持有者。

**涉及文件**：`src/View/GameBoardWidget.h/cpp`

### [2026-07-14] 跳过响应后操作面板不更新，覆盖结束出牌按钮

**现象**：出牌阶段打出杀，点击「跳过」后提示文字不更新、跳过按钮不消失，且覆盖了「结束出牌」按钮，无法结束出牌。

**根因**：`GameBoardWidget::onPendingActionCleared` 只重置了 `m_state = Idle`，没有恢复 `ActionPanelWidget` 的显示。面板仍停留在响应模式（显示跳过按钮 + 旧的响应提示文字）。

跳过按钮被设为 `setVisible(true)` 后一直显示，而「结束出牌」按钮虽存在但被遮挡在跳过按钮之下，且因 `hideAllButtons()` → `setVisible(false)` 未被调用而不可见。

**修复**：
1. `onPendingActionCleared` 中调用 `m_actionPanel->updateForPhase(m_currentPhase, false)` 恢复面板
2. 新增 `m_currentPhase` 成员变量，在 `onPhaseChanged` 中更新

**涉及文件**：`src/View/GameBoardWidget.h/cpp`

### [2026-07-14] 弃牌阶段必须弃牌，未检查手牌上限

**现象**：出牌阶段结束后进入弃牌阶段，即使手牌未超限也会强制进入弃牌界面。

**根因**：重构后弃牌阶段的 `getDiscardCount` 检查被删除。`GameBoardWidget::onPhaseChanged` 无条件设置 `m_state = Discarding` 并显示弃牌提示，不检查手牌是否超过体力值对应的上限。

**修复**：`GameBootstrap` 拦截 `phaseChanged` 信号，在 `Discard` 阶段先调用 `avm->getDiscardCount(curId)`。若 ≤ 0 则直接 `gvm->advancePhase()` 跳过弃牌阶段。

**涉及文件**：`src/App/GameBootstrap.h/cpp`、`src/ViewModel/GameViewModel.h`

### [2026-07-14] 架构调整（第二次重构）：GameBootstrap 更名 SGSApp，路由/拦截逻辑下沉 ViewModel

> 晚于下一条（第一次重构）。本文件中早于本次重构的条目提到的 `GameBootstrap` 均对应现在的 `SGSApp`，`CardDisplayData`/`PlayerDisplayData`/`PendingActionVM` 均对应现在的 `CardData`/`PlayerData`/`PendingActionData`；历史条目不改写。

**调整内容**：
- `GameBootstrap` 更名 `SGSApp`，退化为纯组合根：只创建对象、在 `startLocalGame()` 里建立 View ↔ ViewModel 信号槽直连、管理对局生命周期，零业务逻辑
- 原 App 层路由槽（`onPlayCardRequested`/`onTargetSelected`/`onRespondCardRequested` → `ActionViewModel`；`onDiscardCardRequested`/`onEndPlayRequested`/`onAdvanceRequested`/`onSkipRequested` → `GameViewModel`）改为 ViewModel 的 public slots，View 信号直连
- 原 App 层拦截逻辑下沉：弃牌数 ≤0 自动跳过 → `GameViewModel::setNextPhase`；无响应牌自动跳过 → `GameViewModel::onModelPendingActionCreated`
- Common 值类型改名：`CardDisplayData` → `CardData`（新增列表别名 `CardList`）、`PlayerDisplayData` → `PlayerData`、`PendingActionVM` → `PendingActionData`

**涉及文件**：`src/App/SGSApp.h/cpp`（原 `GameBootstrap.h/cpp`）、`src/ViewModel/*`、`src/Common/*`、`src/View/*`（仅头文件名变更）、`src/main.cpp`

**文档同步**：`connection.md`、`README.md`、`CLAUDE.md`、`interface.md` §8/§9、`plan2.0.md` §2/§7 顶部阅读说明均已按新架构更新。

### [2026-07-14] 架构调整：GameBootstrap 作为程序入口

**调整内容**：
- `MainWindow` 不再包含 `GameBootstrap`，改为纯 UI 层，由 `GameBootstrap` 创建并拥有
- `GameBootstrap` 成为真正的 Composition Root，创建 `MainWindow`、`GameViewModel`、`GameBoardWidget`
- `main.cpp` 直接创建 `GameBootstrap`，不直接引用 `MainWindow`

**依赖关系变更**：
```
旧: main → MainWindow → GameBootstrap → GameViewModel/GameBoardWidget
新: main → GameBootstrap → MainWindow + GameViewModel + GameBoardWidget
```

**涉及文件**：`src/main.cpp`、`src/App/GameBootstrap.h/cpp`、`src/View/MainWindow.h/cpp`
