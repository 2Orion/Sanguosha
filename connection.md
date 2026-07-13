# 模块间通信机制

重构后 View 层完全不依赖 ViewModel，所有跨层通信通过 **Qt 信号/槽** 经 **App/GameBootstrap** 中转。

---

## 通信总图

```
Model (信号) ──► ViewModel (信号) ──► GameBootstrap ──► View (槽)
                                                ▲
View (信号) ─────────────────────────────────────┘
```

两种通信路径：

| 方向 | 机制 | 中转 |
|------|------|------|
| ViewModel → View | Qt 信号直达 View 槽 | GameBootstrap 连接信号/槽 |
| View → ViewModel | Qt 信号 → GameBootstrap 槽 → 调用 VM 方法 | GameBootstrap 作为中介者 |

---

## 1. ViewModel → View（数据推送）

ViewModel 通过 Qt 信号推送值类型数据，不暴露任何内部指针。

### 示例：手牌更新

```cpp
// GameViewModel 在状态变化时发出信号
void GameViewModel::pushHandCards(int playerId) {
    CardDisplayList cards; // 值类型 QVector<CardDisplayData>
    // ... 填充数据 ...
    emit handCardsUpdated(playerId, cards);
}
```

```cpp
// GameBootstrap 连接
connect(m_gvm, &GameViewModel::handCardsUpdated,
        m_board, &GameBoardWidget::onHandCardsUpdated);
```

```cpp
// GameBoardWidget 接收，不含任何 ViewModel 头文件
void GameBoardWidget::onHandCardsUpdated(int playerId, const CardDisplayList& cards) {
    if (playerId == m_currentPlayerId)
        m_bottomHandArea->setCards(cards, false);
    else
        m_topHandArea->setCards(cards, false);
}
```

### 全部 ViewModel → View 信号

| ViewModel 信号 | View 槽 | 数据类型 |
|---------------|--------|---------|
| `phaseChanged(PhaseType)` | `onPhaseChanged` | 枚举 |
| `playerDataUpdated(int, PlayerDisplayData)` | `onPlayerDataUpdated` | 值类型 |
| `handCardsUpdated(int, CardDisplayList)` | `onHandCardsUpdated` | 值类型列表 |
| `pendingActionCreated(PendingActionVM)` | `onPendingActionCreated` | 值类型 |
| `pendingActionCleared()` | `onPendingActionCleared` | 无参数 |
| `gameOver(int)` | `onGameOver` | int |
| `logMessage(QString)` | `onLogMessage` | QString |

---

## 2. View → ViewModel（命令 — 通过 GameBootstrap 中转）

View 不知道 ViewModel 的存在。它发出信号，GameBootstrap 接收后调用 ViewModel 方法。

### 示例：出牌

```
用户双击卡牌
→ CardWidget::mouseDoubleClickEvent
→ emit doubleClicked(cardId)
→ HandCardAreaWidget::onCardWidgetDoubleClicked
→ emit cardDoubleClicked(cardId)
→ GameBoardWidget::onCardClicked(cardId)
→ emit playCardRequested(cardId, m_currentPlayerId)
→ GameBootstrap::onPlayCardRequested(cardId, playerId)
  → 检查 isOwnCard + canPlayCard
  → 获取合法目标
  → 0/1 目标：直接 ActionViewModel::playCard(…)
  → N 目标：暂存目标，等待 targetPlayerSelected
```

```cpp
// GameBootstrap.cpp — 中介者逻辑
void GameBootstrap::onPlayCardRequested(int cardId, int playerId) {
    auto* avm = m_gvm->actionVM();
    if (!avm->isOwnCard(cardId, playerId)) return;
    if (!avm->canPlayCard(cardId, playerId)) return;

    auto targets = avm->getValidTargetIds(cardId, playerId);
    if (targets.empty()) {
        avm->playCard(cardId, playerId, {});
    } else if (targets.size() == 1) {
        avm->playCard(cardId, playerId, {targets[0]});
    } else {
        // 多目标选择
        m_pendingCardId = cardId;
        m_pendingTargetIds = QVector<int>(targets.begin(), targets.end());
        // 等待 targetPlayerSelected 信号
    }
}
```

### 全部 View → ViewModel 命令

| View 信号 | GameBootstrap 槽 | ViewModel 调用 |
|----------|-----------------|---------------|
| `playCardRequested(int, int)` | `onPlayCardRequested` | `avm->playCard(…)` |
| `respondCardRequested(int, int)` | `onRespondCardRequested` | `avm->respondCard(…)` |
| `discardCardRequested(int, int)` | `onDiscardCardRequested` | `avm->discardCard(…)` |
| `targetPlayerSelected(int)` | `onTargetSelected` | `avm->playCard(…)` |
| `endPlayRequested()` | `onEndPlayRequested` | `gvm->endPlayPhase()` |
| `advanceRequested()` | `onAdvanceRequested` | `gvm->advancePhase()` |
| `skipRequested()` | `onSkipRequested` | `avm->skipResponse(…)` |

---

## 3. GameBootstrap 连接表

GameBootstrap 构造函数中完成所有连接：

```cpp
void GameBootstrap::wireAll() {
    // ViewModel 信号 → View 槽
    connect(m_gvm, &GameViewModel::phaseChanged,      m_board, &GameBoardWidget::onPhaseChanged);
    connect(m_gvm, &GameViewModel::playerDataUpdated,  m_board, &GameBoardWidget::onPlayerDataUpdated);
    connect(m_gvm, &GameViewModel::handCardsUpdated,   m_board, &GameBoardWidget::onHandCardsUpdated);
    connect(m_gvm, &GameViewModel::pendingActionCreated, m_board, &GameBoardWidget::onPendingActionCreated);
    connect(m_gvm, &GameViewModel::pendingActionCleared, m_board, &GameBoardWidget::onPendingActionCleared);
    connect(m_gvm, &GameViewModel::gameOver,           m_board, &GameBoardWidget::onGameOver);
    connect(m_gvm, &GameViewModel::logMessage,         m_board, &GameBoardWidget::onLogMessage);

    // View 信号 → GameBootstrap 中转
    connect(m_board, &GameBoardWidget::playCardRequested,    this, &GameBootstrap::onPlayCardRequested);
    connect(m_board, &GameBoardWidget::respondCardRequested, this, &GameBootstrap::onRespondCardRequested);
    connect(m_board, &GameBoardWidget::discardCardRequested, this, &GameBootstrap::onDiscardCardRequested);
    connect(m_board, &GameBoardWidget::targetPlayerSelected, this, &GameBootstrap::onTargetSelected);
    connect(m_board, &GameBoardWidget::endPlayRequested,     this, &GameBootstrap::onEndPlayRequested);
    connect(m_board, &GameBoardWidget::advanceRequested,     this, &GameBootstrap::onAdvanceRequested);
    connect(m_board, &GameBoardWidget::skipRequested,        this, &GameBootstrap::onSkipRequested);
}
```

---

## 4. 值类型（Common 层）

三个跨层值类型结构体，放在`src/Common/`中，所有层（包括 View）都可以引用：

```cpp
// CardDisplayData.h
struct CardDisplayData {
    int cardId; CardType cardType; CardSuit suit;
    int number; QString cardName; QString description;
    CardColor color; bool isBasic; bool isStrategy;
    QString suitSymbol; QString numberString;
    bool isSelected; bool isPlayable; bool isHighlighted;
    int ownerId;
};

// PlayerDisplayData.h
struct PlayerDisplayData {
    int playerId; QString displayName; QString characterName;
    QString skillName; QString skillDescription;
    int hp; int maxHp; bool isAlive; bool isDying;
    int handCardCount; int handCardLimit; bool isCurrentPlayer;
};

// PendingActionVM.h
struct PendingActionVM {
    int sourceId; int targetId; int sourceCardId;
    CardType requiredCardType; QString description;
    bool canSkip; QVector<int> remainingTargetIds;
};
```

---

## 5. 解耦效果

```
重构前：                              重构后：
View ──#include──► ViewModel          View ──#include──► Common
View ──持有指针──► ViewModel          View ──零持有──► ViewModel
View ──直接调用──► VM 方法             View ──emit信号──► App中转──► VM
                                      ViewModel ──信号──► View (经 App 连接)
```

| 指标 | 重构前 | 重构后 |
|------|--------|--------|
| View 中 ViewModel 头文件引用 | 每文件 1-3 个 | **0** |
| View 持有 ViewModel 指针 | GameBoardWidget、MainWindow | **0** |
| 信号连接位置 | 分散在各 View 的 connectViewModel() | 集中在 GameBootstrap |
| View 验证卡片逻辑 | 直接在 onCardClicked 中 | 由 GameBootstrap 中介 |
