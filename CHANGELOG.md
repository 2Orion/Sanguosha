# Changelog

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
