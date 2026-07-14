# Changelog

## Bug Fixes

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

**根因**：`GameBoardWidget::onCardClicked` 在 `State::Responding` 分支中发射 `respondCardRequested(cardId, m_currentPlayerId)`，其中 `m_currentPlayerId` 是当前回合玩家（攻击者），而非需要响应的玩家（目标）。ViewModel 收到错误 ID 后找不到对应的响应者，`respondCard` 因 `!responder` 直接返回。

**修复**：在 `onPendingActionCreated` 中记录 `m_responderId = info.targetId`，`onCardClicked` 的 Responding 分支改为 `emit respondCardRequested(cardId, m_responderId)`。

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
