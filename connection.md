# 模块间通信机制

> 本文档已按第二次重构（`GameBootstrap` 更名 `SGSApp`，路由/拦截逻辑从 App 层下沉到 ViewModel）同步更新。当前架构：View 层完全不依赖 ViewModel，所有跨层通信通过 **Qt 信号/槽 直连**，连接关系由 **App/SGSApp（组合根）** 在 `startLocalGame()` 里集中建立。SGSApp 本身不再拦截或路由任何消息。

---

## 通信总图

```
Model (信号) ──► ViewModel (信号) ────直连────► View (槽)
View (信号) ────直连────► ViewModel (public slots)
                    ▲
         连接均由 SGSApp::startLocalGame() 建立
```

两种通信路径：

| 方向 | 机制 | 谁建立连接 |
|------|------|-----------|
| ViewModel → View | Qt 信号直达 View 槽 | SGSApp（组合根） |
| View → ViewModel | View 信号直达 ViewModel 的 public slots | SGSApp（组合根） |

待定动作的响应者规则保持一致：普通【杀】/AOE 响应由 `target` 负责，濒死救援（`requiredCardType == Peach`）由 `source` 负责；因此 View、GameViewModel 自动跳过和 ActionViewModel 校验都按该规则取响应者。

与第一次重构的区别：`GameBootstrap` 时代 View 信号先进 App 层的私有槽做校验/路由再调 VM 方法，且 `phaseChanged`/`pendingActionCreated` 会被 App 层拦截。现在这些逻辑全部在 ViewModel 内部（见第 3 节），SGSApp 只做 `connect`，View 和 VM 之间是纯直连。

---

## 1. ViewModel → View（数据推送）

ViewModel 通过 Qt 信号推送值类型数据，不暴露任何内部指针。

### 示例：手牌更新

```cpp
// GameViewModel 在状态变化时发出信号
void GameViewModel::pushHandCards(int playerId) {
    QVector<CardData> cards; // 值类型（CardList = QVector<CardData>）
    // ... 填充数据，含 isPlayable 高亮标记 ...
    emit handCardsUpdated(playerId, cards);
}
```

```cpp
// SGSApp 连接
connect(m_gvm, &GameViewModel::handCardsUpdated,
        m_board, &GameBoardWidget::onHandCardsUpdated);
```

```cpp
// GameBoardWidget 接收，不含任何 ViewModel 头文件
void GameBoardWidget::onHandCardsUpdated(int playerId, const CardList& cards) { ... }
```

### 全部 ViewModel → View 信号（均为直连）

| ViewModel 信号 | View 槽 | 数据类型 |
|---------------|--------|---------|
| `phaseChanged(PhaseType)` | `onPhaseChanged` | 枚举 |
| `playerDataUpdated(int, PlayerData)` | `onPlayerDataUpdated` | 值类型 |
| `handCardsUpdated(int, CardList)` | `onHandCardsUpdated` | 值类型列表 |
| `pendingActionCreated(PendingActionData)` | `onPendingActionCreated` | 值类型 |
| `pendingActionCleared()` | `onPendingActionCleared` | 无参数 |
| `gameOver(int)` | `onGameOver` | int |
| `logMessage(QString)` | `onLogMessage` | QString |
| `ActionViewModel::targetSelectionStarted(QVector<int>)` | `onTargetSelectionStarted` | 合法目标 ID |
| `ActionViewModel::targetSelectionFinished()` | `onTargetSelectionFinished` | 无参数 |

原来由 App 层拦截槽（`onPhaseFromVM`/`onPendingActionFromVM`）承担的"自动跳过"判断，现在在信号发出**之前**就已在 GameViewModel 内部完成（见第 3 节），所以 View 收到的信号都是"确实需要 UI 呈现"的。

---

## 2. View → ViewModel（命令）

View 不知道 ViewModel 的存在。它发出信号，SGSApp 把这些信号直连到 ViewModel 的 public slots。

### 示例：出牌

```
用户双击卡牌
→ CardWidget::mouseDoubleClickEvent
→ emit doubleClicked(cardId)
→ HandCardAreaWidget → GameBoardWidget::onCardClicked(cardId)
→ emit playCardRequested(cardId, m_currentPlayerId)
→ ActionViewModel::onPlayCardRequested(cardId, playerId)   ← 直连，无 App 中转
  → 检查 isOwnCard + canPlayCard（含技能转化判定 playsAsKill）
  → 获取合法目标 getValidTargetIds
  → 0/1 目标：直接 playCard(…)
  → N 目标：暂存 m_pendingCardId/m_pendingTargetIds，发出 targetSelectionStarted
              → View 进入 SelectingTarget，用户点击目标
              → targetPlayerSelected → targetSelectionFinished → playCard(…)
```

### 全部 View → ViewModel 命令（均为直连）

| View 信号 | ViewModel public slot |
|----------|----------------------|
| `playCardRequested(int, int)` | `ActionViewModel::onPlayCardRequested` |
| `respondCardRequested(int, int)` | `ActionViewModel::onRespondCardRequested` |
| `targetPlayerSelected(int)` | `ActionViewModel::onTargetSelected` |
| `discardCardRequested(int, int)` | `GameViewModel::onDiscardCardRequested` |
| `endPlayRequested()` | `GameViewModel::onEndPlayRequested` |
| `advanceRequested()` | `GameViewModel::onAdvanceRequested` |
| `skipRequested()` | `GameViewModel::onSkipRequested` |

出牌相关命令（出牌/响应/选目标）进 `ActionViewModel`，阶段推进相关命令（弃牌/结束出牌/推进/跳过）进 `GameViewModel`。多目标选择的暂存状态（原 GameBootstrap 的 `m_pendingCardId` 等）也已随路由逻辑迁入 `ActionViewModel`。

---

## 3. SGSApp 连接表与拦截逻辑的新位置

### 3.1 SGSApp：只连接，不路由

`SGSApp::startLocalGame()` 创建 `GameViewModel`/`GameBoardWidget` 后直接完成所有连接（实际代码见 `src/App/SGSApp.cpp`）：

```cpp
void SGSApp::startLocalGame(int charId1, int charId2) {
    m_gvm = new GameViewModel(this);
    m_board = new GameBoardWidget();
    auto* avm = m_gvm->actionVM();

    // View → ViewModel（直连）
    connect(m_board, &GameBoardWidget::playCardRequested,    avm,   &ActionViewModel::onPlayCardRequested);
    connect(m_board, &GameBoardWidget::respondCardRequested, avm,   &ActionViewModel::onRespondCardRequested);
    connect(m_board, &GameBoardWidget::targetPlayerSelected, avm,   &ActionViewModel::onTargetSelected);
    connect(m_board, &GameBoardWidget::discardCardRequested, m_gvm, &GameViewModel::onDiscardCardRequested);
    connect(m_board, &GameBoardWidget::endPlayRequested,     m_gvm, &GameViewModel::onEndPlayRequested);
    connect(m_board, &GameBoardWidget::advanceRequested,     m_gvm, &GameViewModel::onAdvanceRequested);
    connect(m_board, &GameBoardWidget::skipRequested,        m_gvm, &GameViewModel::onSkipRequested);

    // ViewModel → View（直连）
    connect(m_gvm, &GameViewModel::phaseChanged,         m_board, &GameBoardWidget::onPhaseChanged);
    connect(m_gvm, &GameViewModel::playerDataUpdated,    m_board, &GameBoardWidget::onPlayerDataUpdated);
    connect(m_gvm, &GameViewModel::handCardsUpdated,     m_board, &GameBoardWidget::onHandCardsUpdated);
    connect(avm, &ActionViewModel::targetSelectionStarted,  m_board, &GameBoardWidget::onTargetSelectionStarted);
    connect(avm, &ActionViewModel::targetSelectionFinished, m_board, &GameBoardWidget::onTargetSelectionFinished);
    connect(m_gvm, &GameViewModel::pendingActionCreated, m_board, &GameBoardWidget::onPendingActionCreated);
    connect(m_gvm, &GameViewModel::pendingActionCleared, m_board, &GameBoardWidget::onPendingActionCleared);
    connect(m_gvm, &GameViewModel::gameOver,             m_board, &GameBoardWidget::onGameOver);
    connect(m_gvm, &GameViewModel::logMessage,           m_board, &GameBoardWidget::onLogMessage);

    // 生命周期
    connect(m_board, &GameBoardWidget::gameFinished, this, &SGSApp::onGameFinished);

    m_gvm->startGame(charId1, charId2);
}
```

`SGSApp::startLocalGame()` 在创建新局前会对上一局的 Board 和 GameViewModel 调用 `deleteLater()`；对局结束时同样安排两者删除，再切回选将页，避免旧 Model、卡牌和信号连接残留。

### 3.2 拦截逻辑的新位置：GameViewModel 内部

原 `GameBootstrap::onPhaseFromVM`/`onPendingActionFromVM` 两个拦截槽已删除，对应逻辑下沉到 `GameViewModel`，在信号发出之前完成判断：

```cpp
// GameViewModel::setNextPhase —— 自动跳过弃牌阶段（原 onPhaseFromVM）
void GameViewModel::setNextPhase(PhaseType phase) {
    if (phase == PhaseType::Discard) {
        Player* cur = currentPlayer();
        if (cur && GameRule::getDiscardCount(cur) <= 0) {
            setNextPhase(PhaseType::End);   // 手牌未超限，直接进入 End，不发 Discard 信号
            return;
        }
    }
    m_state->setCurrentPhase(phase);
    ...
}

// GameViewModel::onModelPendingActionCreated —— 无响应牌自动跳过（原 onPendingActionFromVM）
void GameViewModel::onModelPendingActionCreated(const PendingActionInfo& info) {
    PendingActionData vm; /* Model 结构体 → 值类型翻译 */
    int responderId = vm.requiredCardType == CardType::Peach ? vm.sourceId : vm.targetId;
    auto responseCards = m_actionVM->getResponseCardIds(responderId, vm.requiredCardType);
    if (responseCards.empty()) {
        m_actionVM->skipResponse(responderId, true);  // 不 emit pendingActionCreated
        return;
    }
    emit pendingActionCreated(vm);
    ...
}
```

**新增同类拦截逻辑的落点**：加在 `GameViewModel`（阶段/待定动作相关）或 `ActionViewModel`（出牌校验相关）内部、发信号之前，不要加在 `SGSApp`（它只做连接）或 `GameBoardWidget`（View 零业务逻辑）。这与第一次重构时"加在 GameBootstrap 拦截槽"的指引**相反**，以本节为准。

---

## 4. Model ↔ ViewModel（双向通信）

（此部分机制未变。）

### 4.1 Model → ViewModel：Qt 信号

Model 层对象继承 `QObject`，状态变化时 `emit signal()`。ViewModel 在 `initGame()` 时 `connect` 这些信号。

| Model 信号 | ViewModel 处理 | 说明 |
|-----------|---------------|------|
| `GameState::phaseChanged` | → `GameViewModel::phaseChanged`（信号直转） | 阶段变化转发 |
| `GameState::currentPlayerChanged` | → `GameViewModel::currentPlayerChanged`（信号直转） | 当前玩家转发 |
| `GameState::pendingActionCreated` | → `GameViewModel::onModelPendingActionCreated`（槽） | 翻译为 PendingActionData，无响应牌时自动跳过 |
| `GameState::pendingActionCleared` | → `GameViewModel::onModelPendingActionCleared`（槽） | 清空待定动作 |
| `GameState::gameOver` | → `GameViewModel::onModelGameOver`（槽） | 处理游戏结束 |
| `Player::handCardsChanged` | → lambda → `pushHandCards` | 手牌变化推送 |
| `Player::stateChanged` | → lambda → `pushPlayerData` | 玩家状态推送 |
| `Player::died` | → `GameViewModel::onModelPlayerDied`（槽） | 阵亡处理 |

### 4.2 ViewModel → Model：直接方法调用

ViewModel **持有** Model 对象（`unique_ptr`），直接调用其方法修改状态；Model 状态变化后通过信号通知 ViewModel，形成闭环。

```cpp
class GameViewModel : public QObject {
    std::unique_ptr<GameState> m_state;
    std::unique_ptr<CardManager> m_cardManager;
    std::unique_ptr<ActionViewModel> m_actionVM;
};
```

### 4.3 完整闭环示例：出杀后跳过响应

```
P2 点击"跳过":
  ① GameBoardWidget emit skipRequested()
  ②   → GameViewModel::onSkipRequested()               ← 直连（原经 GameBootstrap）
  ③     → ActionViewModel::skipResponse(responderId, true)
  ④       → GameRule::handleKillResponse(state, responder, nullptr)   ← VM → Model
  ⑤         → Player::damage(1) → emit stateChanged    ← Model 信号 → pushPlayerData
  ⑥       → state->clearPendingAction()
  ⑦         → emit pendingActionCleared()              ← Model 信号
  ⑧           → GameViewModel::onModelPendingActionCleared()
  ⑨             → emit pendingActionCleared()          ← VM 信号
  ⑩               → GameBoardWidget::onPendingActionCleared()  ← View 恢复面板
```

### 4.4 小结

| 方向 | 机制 |
|------|------|
| **Model → ViewModel** | Qt 信号 |
| **ViewModel → Model** | 方法调用 |

---

## 5. 值类型（Common 层）

> ⚠️ 本次重构中三个值类型已改名：`CardDisplayData` → **`CardData`**、`PlayerDisplayData` → **`PlayerData`**、`PendingActionVM` → **`PendingActionData`**；列表别名 `CardDisplayList` → **`CardList`**。文件同步改名为 `CardData.h`/`PlayerData.h`/`PendingActionData.h`。

三个跨层值类型结构体，放在 `src/Common/` 中，所有层（包括 View）都可以引用：

```cpp
// CardData.h
struct CardData {
    int cardId; CardType cardType; CardSuit suit;
    int number; QString cardName; QString description;
    CardColor color; bool isBasic; bool isStrategy;
    QString suitSymbol; QString numberString;
    bool isSelected; bool isPlayable; bool isHighlighted;
    int ownerId;
};
using CardList = QVector<CardData>;

// PlayerData.h
struct PlayerData {
    int playerId; QString displayName; QString characterName;
    QString skillName; QString skillDescription;
    int hp; int maxHp; bool isAlive; bool isDying;
    int handCardCount; int handCardLimit; bool isCurrentPlayer;
};

// PendingActionData.h
struct PendingActionData {
    int sourceId; int targetId; int sourceCardId;
    CardType requiredCardType; QString description;
    bool canSkip; QVector<int> remainingTargetIds;
};
```

---

## 6. 解耦效果

```
View ──#include──► Common（仅值类型）
View ──零持有──► ViewModel
View ──emit信号──► ViewModel public slots（SGSApp 建立直连）
ViewModel ──信号──► View 槽（SGSApp 建立直连）
```

| 指标 | 现状 |
|------|------|
| View 中 ViewModel 头文件引用 | **0** |
| View 持有 ViewModel 指针 | **0** |
| 信号连接位置 | 集中在 `SGSApp::startLocalGame()` |
| 出牌校验/路由逻辑 | `ActionViewModel`（onPlayCardRequested 等 public slots） |
| 自动跳过弃牌/响应逻辑 | `GameViewModel`（setNextPhase / onModelPendingActionCreated） |
| SGSApp 职责 | 纯组合根：创建对象 + connect + 生命周期，零业务逻辑 |
