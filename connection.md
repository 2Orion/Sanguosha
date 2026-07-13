# 模块间通信机制分析

本项目采用分层 MVVM 架构，模块间通信遵循严格的方向约束。本文按方向梳理所有通信路径，并给出完整代码示例。

---

## 通信方向总览

```
┌─────────────────────────────────────────────────────────────┐
│  View 层 (Qt)                                               │
│  ┌──────────────┐        Qt signals/slots       ┌─────────┐ │
│  │ GameBoard    │ ◄──────────────────────────► │ CardWdg │ │
│  │ Widget       │          ┌──────────┐         └─────────┘ │
│  └──────┬───────┘  ┌──────│ActionPanel│───────┐            │
│         │          │      └──────────┘       │            │
│         │◄─── ① ───┤                         ├─── ① ──► │
│         ▼          │      ┌──────────┐       │            │
│  ┌────────────┐    └──────│PlayerInfo│───────┘            │
│  │ GameVM     │           └──────────┘                    │
│  │ (ViewModel)│                                            │
│  └──────┬─────┘                                            │
│         │                                                  │
│         │◄─── ② ───┤         ┌─────────────┐              │
│         ▼          └─────────│ ActionVM    │              │
│  ┌────────────┐              │ (ViewModel)  │              │
│  │ GameState  │              └─────────────┘              │
│  │ (Model)    │              ┌─────────────┐              │
│  └──────┬─────┘              │ PlayerVM    │              │
│         │                    │ (ViewModel)  │              │
│         ▼                    └─────────────┘              │
│  ┌────────────┐              ┌─────────────┐              │
│  │ Player     │◄─────────────│ CardVM      │              │
│  │ (Model)    │              │ (ViewModel)  │              │
│  └────────────┘              └─────────────┘              │
└─────────────────────────────────────────────────────────────┘
```

### 三种通信方向

| 编号 | 方向 | 机制 | 示例 |
|------|------|------|------|
| ① | ViewModel → View | EventListener + Qt::QueuedConnection 异步回调 | 体力变化 → 更新血条 |
| ② | View → ViewModel | 普通 C++ 函数同步调用 | 点击卡牌 → playCard() |
| ③ | Model → ViewModel | EventListener 同步回调 + 参数转换/转发 | 牌堆变化 → 刷新手牌 |

---

## 1. Model → ViewModel（事件监听 + 值类型转发）

### 机制

Model 层通过 `Core/Event.h` 的 `EventListener<Args...>` 发布事件。ViewModel 层在构造时 `connect` 这些事件，在回调中**转换参数类型**（裸指针 → 值类型）后，在自身的 `EventListener` 上 `notify` 转发给 View。

### 实现要点

- `EventListener::notify` 是**同步的**：所有注册回调依次执行
- 每个 Model 事件在 ViewModel 中都有**1:1 对应的转发事件**
- ViewModel 在析构时 `disconnect` 所有 Model 事件，防止悬垂回调

### 完整示例

```
事件链：Player::hpChanged → PlayerViewModel::hpChanged → View 更新
```

#### 第 1 步：Model 层定义事件

```cpp
// src/Model/Player.h
class Player {
    EventListener<int> hpChanged;    // 参数：新体力值 int
    EventListener<Player*> died;     // 参数：死亡玩家 Player*
};
```

Model 层在体力变化时 `notify`：

```cpp
// src/Model/Player.cpp
void Player::damage(int value) {
    m_hp -= value;
    hpChanged.notify(m_hp);    // 同步调用所有回调
    stateChanged.notify();
}
```

#### 第 2 步：ViewModel 层监听并转发

```cpp
// src/ViewModel/PlayerViewModel.cpp
void PlayerViewModel::connectModelEvents() {
    // 类型不变：int → int
    m_conn.hpChangedId = m_player->hpChanged.connect(
        [this](int hp) { this->hpChanged.notify(hp); });
    
    // 类型转换：Player* → int playerId
    m_conn.diedId = m_player->died.connect(
        [this](Player* p) { this->died.notify(p->playerId()); });
    
    // 类型转换：Card* → int cardId
    m_conn.handCardAddedId = m_player->handCardAdded.connect(
        [this](Card* c) { this->handCardAdded.notify(c ? c->id() : -1); });
}
```

#### 第 3 步：View 层桥接到 Qt 事件循环

```cpp
// src/View/PlayerInfoWidget.cpp
void PlayerInfoWidget::connectViewModelEvents() {
    // hpChanged → 更新体力显示
    auto id1 = m_pvm->hpChanged.connect([this](int /*hp*/) {
        QMetaObject::invokeMethod(this, [this]() {
            updateHp();       // 刷新 ❤/🖤 显示
        }, Qt::QueuedConnection);
    });
    m_connectionIds.push_back(id1);

    // died → 标红边框
    auto id5 = m_pvm->stateChanged.connect([this]() {
        QMetaObject::invokeMethod(this, [this]() {
            refreshAll();
        }, Qt::QueuedConnection);
    });
    m_connectionIds.push_back(id5);
}
```

### 其他 Model → ViewModel 转发示例

```
Model::GameState::phaseChanged(PhaseType)
  → GameViewModel::phaseChanged.notify(PhaseType)
    → View 接收

Model::GameState::pendingActionCreated(const PendingActionInfo&)
  → GameViewModel 内部调用 onPendingActionCreated(info)
    → 创建 PendingActionVM（值类型，指针→int ID）
    → GameViewModel::pendingActionCreated.notify(PendingActionVM)
      → View 接收

Model::Player::dying(Player*)
  → PlayerViewModel::dying.notify(int playerId)
    → View 接收并显示濒死特效
```

---

## 2. ViewModel → View（事件 + Qt 事件循环桥接）

### 机制

View 通过 `EventListener::connect` 监听 ViewModel 事件，但不在回调中直接更新 UI。而是使用 `QMetaObject::invokeMethod(this, lambda, Qt::QueuedConnection)` 将 UI 更新**投递到 Qt 主事件循环**中执行。

### 为什么需要 QueuedConnection

```cpp
// 不安全的做法（直接在主线程也可能安全，但不够健壮）
pvm->hpChanged.connect([this](int hp) {
    m_hpLabel->setText(...);  // 如果回调在非主线程触发 → CRASH
});

// 安全的做法
pvm->hpChanged.connect([this](int) {
    QMetaObject::invokeMethod(this, [this]() {
        m_hpLabel->setText(...);  // 始终在 Qt 主线程执行
    }, Qt::QueuedConnection);
});
```

### 完整示例

```cpp
// src/View/GameBoardWidget.cpp — ViewModel → View 通信
void GameBoardWidget::connectViewModel() {
    // 1. 阶段变化
    m_gvm->phaseChanged.connect([this](PhaseType phase) {
        QMetaObject::invokeMethod(this, [this, phase]() {
            onPhaseChanged(phase);    // 更新 UI
        }, Qt::QueuedConnection);
    });

    // 2. 当前玩家变化
    m_gvm->currentPlayerChanged.connect([this](int idx) {
        QMetaObject::invokeMethod(this, [this, idx]() {
            onPlayerChanged(idx);     // 切换信息面板
        }, Qt::QueuedConnection);
    });

    // 3. 游戏结束
    m_gvm->gameOver.connect([this](int winnerId) {
        QMetaObject::invokeMethod(this, [this, winnerId]() {
            onGameOver(winnerId);     // 弹窗 + 返回菜单
        }, Qt::QueuedConnection);
    });

    // 4. 日志消息
    m_gvm->logMessage.connect([this](const std::string& msg) {
        QMetaObject::invokeMethod(this, [this, msg]() {
            showLog(QString::fromStdString(msg));
        }, Qt::QueuedConnection);
    });

    // 5. 全面刷新
    m_gvm->stateChanged.connect([this]() {
        QMetaObject::invokeMethod(this, [this]() {
            refreshDisplay();
        }, Qt::QueuedConnection);
    });

    // 6. 响应待定
    // 注意：需要监听 GameViewModel 的事件（不是 Model 的）
    m_gvm->pendingActionCreated.connect([this](const PendingActionVM& info) {
        QMetaObject::invokeMethod(this, [this, info]() {
            onPendingActionCreated(info);
        }, Qt::QueuedConnection);
    });
}
```

### 事件连接的生命周期管理

```cpp
// GameBoardWidget 析构时断开所有连接
GameBoardWidget::~GameBoardWidget() {
    disconnectViewModel();
}

void GameBoardWidget::disconnectViewModel() {
    for (auto id : m_connectionIds) {
        m_gvm->phaseChanged.disconnect(id);
        m_gvm->currentPlayerChanged.disconnect(id);
        m_gvm->gameOver.disconnect(id);
        m_gvm->stateChanged.disconnect(id);
        m_gvm->logMessage.disconnect(id);
        m_gvm->pendingActionCreated.disconnect(id);
        m_gvm->pendingActionCleared.disconnect(id);
    }
    m_connectionIds.clear();
}
```

---

## 3. View → ViewModel（同步方法调用）

### 机制

View 层直接调用 ViewModel 的公共方法。这是**同步的**（当前线程阻塞等待返回），因为所有的游戏逻辑都在主线程中执行，不需要异步。

### 接口设计原则

ViewModel 的公共方法使用 **int ID** 而非指针，避免 View 层引入 Model 头文件：

```cpp
// ActionViewModel 公共接口（View 调用）
ActionResult playCard(int cardId, int playerId, const std::vector<int>& targetIds);
void respondCard(int cardId, int playerId);
void skipResponse(int playerId, bool forceNoCard);
void discardCard(int cardId, int playerId);
std::vector<int> getPlayableCardIds(int playerId) const;
```

View 层永远不会出现 `Card*` 或 `Player*`：

```cpp
// View 层代码中合法的调用：
avm->playCard(cardId, curPlayerId, {targetId});
avm->respondCard(cardId, responderId);
avm->skipResponse(playerId, true);

// View 层代码中永远不出现：
// avm->playCard(card, player, {target});          ✗ 错误
// avm->respondCard(card, responder);              ✗ 错误
```

### 完整示例：点击卡牌出牌

```
用户双击卡牌 → CardWidget::mouseDoubleClickEvent
  → emit CardWidget::doubleClicked(int cardId)
    → HandCardAreaWidget::onCardWidgetDoubleClicked(cardId)
      → emit HandCardAreaWidget::cardDoubleClicked(int cardId)
        → GameBoardWidget::onCardDoubleClicked(int cardId)
          → GameBoardWidget::onCardClicked(int cardId)
```

```cpp
// src/View/GameBoardWidget.cpp
void GameBoardWidget::onCardClicked(int cardId)
{
    if (!m_gvm) return;

    GameViewModel* gvm = m_gvm;
    int curPlayerId = gvm->currentPlayerId();
    ActionViewModel* avm = gvm->actionVM();

    switch (m_state) {
    case State::Idle:
    {
        // 出牌阶段 — 尝试使用牌
        if (gvm->currentPhase() != PhaseType::Play) return;

        // 只能使用当前玩家自己的手牌
        if (!avm->isOwnCard(cardId, curPlayerId)) {
            showLog(QStringLiteral("不能使用对方的牌"));
            return;
        }

        // 检查卡牌是否可用
        if (!avm->canPlayCard(cardId, curPlayerId)) {
            showLog(QStringLiteral("这张牌不能使用"));
            return;
        }

        // 获取合法目标
        std::vector<int> targetIds = avm->getValidTargetIds(cardId, curPlayerId);

        if (targetIds.empty()) {
            avm->playCard(cardId, curPlayerId, {});
            refreshDisplay();
        } else if (targetIds.size() == 1) {
            avm->playCard(cardId, curPlayerId, {targetIds[0]});
            refreshDisplay();
        } else {
            enterTargetSelection(cardId, curPlayerId, targetIds);
        }
        break;
    }

    case State::Responding:
    {
        // 响应模式 — 打出这张牌作为响应
        const PendingActionVM info = gvm->pendingActionVM();
        avm->respondCard(cardId, info.targetId);
        break;
    }

    case State::Discarding:
    {
        // 弃牌阶段 — 选中/取消选中（在确认时统一处理）
        break;
    }
    }
}
```

### 其他 View → ViewModel 调用场景

| 用户操作 | View 方法 | ViewModel 调用 |
|---------|-----------|---------------|
| 双击卡牌 | `onCardDoubleClicked` | `avm->playCard(cardId, playerId, targetIds)` |
| 点击目标 | `onTargetClicked` | `avm->playCard(cardId, playerId, {targetId})` |
| 结束出牌 | `onPlayPhaseEnded` | `gvm->endPlayPhase()` |
| 跳过响应 | `onResponseSkipped` | `avm->skipResponse(playerId, forceNoCard)` |
| 确认弃牌 | `onDiscardConfirmed` | `avm->discardCard(cardId, playerId)` |
| 选择武将+开始 | `onStartGame` | `gvm->startGame(charId1, charId2)` |

---

## 4. ViewModel → Model（内部方法调用 + 事件订阅）

### 机制

ViewModel **内部**持有 Model 对象的 `unique_ptr` 或裸指针，直接调用 Model 的公共方法修改游戏状态。Model 状态变更后通过 `EventListener::notify` 通知 ViewModel，ViewModel 再转发给 View。

### 完整示例：PlayCard 的完整链路

```
View 调用 → ViewModel 操作 Model → Model 状态变更 → 事件通知 → UI 更新
```

```cpp
// 第 1 步：View 调用 ViewModel
avm->playCard(cardId, curPlayerId, {targetId});

// 第 2 步：ActionViewModel 内部调用 Model
ActionResult ActionViewModel::playCard(int cardId, int playerId, const std::vector<int>& targetIds)
{
    Player* player = findPlayer(playerId);         // ID → Model 指针
    Card* card = findCard(cardId);                 // ID → Model 指针

    ActionResult result = GameRule::executeCard(m_state, m_state->currentPlayer(),
                                                 card, players);
    actionCompleted.notify();
    return result;
}

// 第 3 步：GameRule 修改 Model 状态
// (以杀为例) GameRule::executeKill
void executeKill(GameState* state, Player* user, Player* target) {
    user->setUsedKillThisTurn(true);               // 修改 Player 状态
    // 创建待定动作
    PendingActionInfo info;
    info.source = user;
    info.target = target;
    info.requiredCardType = CardType::Dodge;
    info.canSkip = false;
    state->setPendingAction(info);                 // 设置 Model 待定动作
    // → 触发 GameState::pendingActionCreated.notify()
}

// 第 4 步：GameViewModel 接收到 Model 事件
m_modelConn.pendingActionCreatedId = m_state->pendingActionCreated.connect(
    [this](const PendingActionInfo& info) {
        this->onPendingActionCreated(info);
    });

// 第 5 步：GameViewModel 转换为 PendingActionVM 并转发
void GameViewModel::onPendingActionCreated(const PendingActionInfo& info) {
    PendingActionVM vm;
    vm.sourceId = info.source ? info.source->playerId() : -1;
    vm.targetId = info.target ? info.target->playerId() : -1;
    vm.requiredCardType = info.requiredCardType;
    vm.canSkip = info.canSkip;
    // ...
    this->pendingActionCreated.notify(vm);          // → View 接收
    this->stateChanged.notify();
}

// 第 6 步：View 最终更新 UI (经过 QueuedConnection 桥接)
```

### ViewModel 持有 Model 的方式

```cpp
// GameViewModel 的成员 — 通过 unique_ptr 拥有 Model 对象
class GameViewModel {
    std::unique_ptr<GameState> m_state;              // 唯一拥有者
    std::unique_ptr<CardManager> m_cardManager;      // 唯一拥有者
    std::unique_ptr<ActionViewModel> m_actionVM;     // 子 ViewModel
};

// PlayerViewModel — 通过裸指针观察，不拥有
class PlayerViewModel {
    Player* m_player;                                // 由 GameState 拥有
    // 析构时只 disconnect，不 delete
};
```

---

## 5. View 内部（Qt 信号槽）

### 机制

View 层内部使用标准的 Qt 信号槽机制进行控件间通信。所有 View 内部通信**不走 EventListener**。

### 完整示例：卡牌控件的信号链

```cpp
// CardWidget → HandCardAreaWidget（Qt 信号连接）
// HandCardAreaWidget::rebuildWidgets() 中：
connect(cw, &CardWidget::clicked, this, &HandCardAreaWidget::onCardWidgetClicked);
connect(cw, &CardWidget::doubleClicked, this, &HandCardAreaWidget::onCardWidgetDoubleClicked);

// CardWidget 发出信号
void CardWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_cvm) {
        emit clicked(m_cvm->id());
    }
    QFrame::mousePressEvent(event);
}

void CardWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_cvm) {
        emit doubleClicked(m_cvm->id());
    }
    QFrame::mouseDoubleClickEvent(event);
}

// HandCardAreaWidget 处理并向上转发
void HandCardAreaWidget::onCardWidgetClicked(int cardId) {
    // 切换选中状态（单选）
    for (size_t i = 0; i < m_cardWidgets.size() && i < m_cardViewModels.size(); ++i) {
        if (m_cardWidgets[i]->cardId() == cardId) {
            bool newlySelected = !m_cardWidgets[i]->isSelected();
            if (newlySelected) {
                for (auto* cw : m_cardWidgets) {
                    if (cw->cardId() != cardId) cw->setSelected(false);
                }
            }
            m_cardWidgets[i]->setSelected(newlySelected);
            emit cardClicked(cardId);              // 转发给 GameBoardWidget
            break;
        }
    }
}
```

```cpp
// HandCardAreaWidget → GameBoardWidget（Qt 信号连接）
// GameBoardWidget::connectViewModel() 中：
connect(m_bottomHandArea, &HandCardAreaWidget::cardClicked,
        this, &GameBoardWidget::onCardClicked);
connect(m_bottomHandArea, &HandCardAreaWidget::cardDoubleClicked,
        this, &GameBoardWidget::onCardDoubleClicked);
connect(m_topHandArea, &HandCardAreaWidget::cardClicked,
        this, &GameBoardWidget::onCardClicked);
connect(m_topHandArea, &HandCardAreaWidget::cardDoubleClicked,
        this, &GameBoardWidget::onCardDoubleClicked);
```

### View 内部信号汇总

| 源控件 | 信号 | 目标控件 | 方向 |
|--------|------|---------|------|
| CardWidget | `clicked(int cardId)` | HandCardAreaWidget | 子→父 |
| CardWidget | `doubleClicked(int cardId)` | HandCardAreaWidget | 子→父 |
| HandCardAreaWidget | `cardClicked(int cardId)` | GameBoardWidget | 子→父 |
| HandCardAreaWidget | `cardDoubleClicked(int cardId)` | GameBoardWidget | 子→父 |
| PlayerInfoWidget | `clicked(int playerIndex)` | GameBoardWidget | 子→父 |
| ActionPanelWidget | `playPhaseEnded()` | GameBoardWidget | 子→父 |
| ActionPanelWidget | `respondSkipped()` | GameBoardWidget | 子→父 |
| ActionPanelWidget | `discardConfirmed()` | GameBoardWidget | 子→父 |
| GameBoardWidget | `gameFinished()` | MainWindow | 子→父 |

---

## 6. 通信机制对比总结

| 特性 | EventListener | Qt 信号槽 | 普通函数调用 |
|------|--------------|-----------|------------|
| **使用范围** | Model ↔ ViewModel 内部 | View 内部控件间 | View → ViewModel |
| **同步/异步** | 同步 | 同步（也可 Queued） | 同步 |
| **线程模型** | 调用者线程 | 调用者/Qt 主循环 | 调用者线程 |
| **参数类型** | 任意（含引用） | 已注册类型/QVariant | 任意 |
| **生命周期** | 手动 disconnect | Qt 父子自动管理 | 无 |
| **解耦度** | 需手动管理连接 | 需头文件注册 | 强耦合 |
| **典型用途** | 数据变更通知 | UI 事件传递 | 业务操作 |

### 为什么不统一用一种机制？

- **Model/ViewModel 不用 Qt 信号槽**：为了保持这两层为纯 C++17，零 Qt 依赖。使得 Model 层可以脱离 Qt 编译测试。
- **View 层不用 EventListener**：Qt 信号槽提供自动生命周期管理、调试友好的字符串名称、Qt Designer 集成等优势。
- **View → ViewModel 不用事件**：命令模式会增加不必要的复杂度。函数调用是最自然直接的通信方式。

### 一次完整的双向通信实例（杀 → 闪）

```
View:  用户双击「杀」 → onCardClicked(cardId)
  ↓ (同步函数调用)
VM:    ActionVM::playCard(cardId, playerId, {targetId})
  ↓ (同步函数调用)
Model: GameRule::executeKill → PendingAction (requires Dodge)
  ↓ (EventListener::notify 同步)
VM:    GameVM::onPendingActionCreated(PendingActionInfo)
  ↓ (转换为 PendingActionVM)
VM:    GameVM::pendingActionCreated.notify(PendingActionVM)
  ↓ (EventListner::notify 同步，但 View 的回调使用 QueuedConnection)

  ─── 控制权回到 Qt 事件循环 ───

  ↓ (Qt::QueuedConnection 将 lambda 投递到事件循环)
View:  GameBoardWidget::onPendingActionCreated(PendingActionVM)
  ↓
View:  ActionPanelWidget::updateForPendingAction(info)  // 显示「跳过」按钮
View:  高亮目标手牌中可用的「闪」

  ─── 用户点击「跳过」或双击「闪」 ───

View:  onResponseSkipped() / onCardClicked(cardId)
  ↓ (同步函数调用)
VM:    ActionVM::skipResponse(playerId, forceNoCard)
         / ActionVM::respondCard(cardId, playerId)
  ↓ (同步函数调用)
Model: GameRule::handleKillResponse / handleAoeSkipResponse
  ↓ (EventListener::notify 同步)
VM → View: 事件转发 → UI 更新
```
