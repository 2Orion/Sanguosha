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

---

## 7. 网络模式：ServerApp 连接表（plan2.0.md §2，开发中）

> 服务器侧组合根 `ServerApp`（`src/App/ServerApp.h/cpp`）复用与本地模式完全相同的
> `GameViewModel`/`ActionViewModel`，只是把 SGSApp 连接给 `GameBoardWidget` 的两端
> 换成连接给 `GameServer`（`src/Network/GameServer.h/cpp`，QTcpServer）。
> 客户端侧 `ClientApp`/`GameClient` 尚未实现（见 CLAUDE.md「乙成员任务分步计划」）。

### 7.1 VM → GameServer（广播，`ServerApp::wireViewModelBroadcasts()`）

与第 1 节"全部 ViewModel → View 信号"表完全一一对应，只是接收端从 `GameBoardWidget` 的槽
换成序列化后调用 `GameServer::broadcast(MessageType, payload)`：

| ViewModel 信号 | MessageType | 消息结构体 |
|---------------|-------------|-----------|
| `phaseChanged(PhaseType)` | `PhaseChanged` | `PhaseChangedMsg` |
| `playerDataUpdated(int, PlayerData)` | `PlayerDataUpdated` | `PlayerDataMsg` |
| `handCardsUpdated(int, CardList)` | `HandCardsUpdated` | `HandCardsMsg`（**已脱敏，见下方 7.1.1**） |
| `pendingActionCreated(PendingActionData)` | `PendingActionCreated` | `PendingActionMsg` |
| `pendingActionCleared()` | `PendingActionCleared` | 无 payload |
| `gameOver(int)` | `GameOver` | `GameOverMsg` |
| `logMessage(QString)` | `LogMessage` | `LogMessageMsg` |
| `ActionViewModel::targetSelectionStarted(QVector<int>)` | `TargetSelectionStarted` | `TargetSelectionStartedMsg` |
| `ActionViewModel::targetSelectionFinished()` | `TargetSelectionFinished` | 无 payload |

额外的 `GameStarted`（`GameStartedMsg`）由 `ServerApp::onBothPlayersReady` 在调用
`startHeadlessGame()` **之前**广播，本地模式没有对应物——保证客户端收到的顺序是
`GameStarted → PhaseChanged → PlayerDataUpdated/HandCardsUpdated → ...`
（`startHeadlessGame` 内部 `startGame()` 会同步 emit 这些初始状态信号，若接线/广播 GameStarted
晚于这一步，客户端会先收到状态更新、后收到 GameStarted，顺序就反了）。

#### 7.1.1 手牌脱敏（Step 5）

`handCardsUpdated(int playerId, CardList)` 不能像其余信号一样用 `GameServer::broadcast`
统一广播同一份 payload——手牌所有者本人需要完整牌面，对手不能看到。`ServerApp::
wireViewModelBroadcasts()` 里这一条槽改为两次 `GameServer::sendTo`：

```cpp
connect(m_gvm, &GameViewModel::handCardsUpdated, this,
        [this](int playerId, const CardList& cards) {
    HandCardsMsg fullMsg; fullMsg.playerId = playerId; fullMsg.cards = cards;
    m_server->sendTo(playerId, MessageType::HandCardsUpdated,
                     MessageSerializer::encodePayload(fullMsg));

    HandCardsMsg redactedMsg;
    redactedMsg.playerId = playerId;
    redactedMsg.cards = redactCardList(cards);   // Protocol::redactCardList
    m_server->sendTo(1 - playerId, MessageType::HandCardsUpdated,
                     MessageSerializer::encodePayload(redactedMsg));
});
```

`Protocol::redactCardList`（`src/Network/Protocol.h/cpp`）对每张 `CardData` 只保留
`cardId`/`ownerId`，其余牌面字段（`cardType`/`suit`/`number`/`cardName`/`description`/
`color`/`isBasic`/`isStrategy`/`suitSymbol`/`numberString`/`isEquipment`/`equipSlot`/
`attackRange`/`isSelected`/`isPlayable`/`isHighlighted`）重置为默认构造的占位值；
列表长度（张数）不变。`HandCardsMsg::playerId` 字段语义保持"这是谁的手牌"，与"收件人是谁"
无关——同一次 Model 更新会产生两条不同 payload 的消息，分别 `sendTo` 给所有者和对手。

回归测试见 `tests/network_test.cpp`：`redactCardListKeepsIdentityDropsFace`（纯函数单测）、
`handCardsBroadcastRedactedForOpponentFullForOwner`（两个真实连接互看：己方完整、对方占位、
cardId 集合仍一致）。

### 7.2 GameServer → VM（命令分发，`ServerApp::onClientCommandReceived`）

与第 2 节"全部 View → ViewModel 命令"表完全一一对应，只是发出端从 `GameBoardWidget` 的信号
换成客户端 TCP 消息反序列化：

| MessageType | 消息结构体 | ViewModel public slot |
|-------------|-----------|----------------------|
| `PlayCardRequested` | `PlayCardMsg` | `ActionViewModel::onPlayCardRequested` |
| `RespondCardRequested` | `RespondCardMsg` | `ActionViewModel::onRespondCardRequested` |
| `TargetSelected` | `TargetSelectedMsg` | `ActionViewModel::onTargetSelected` |
| `DiscardCardRequested` | `DiscardCardMsg` | `GameViewModel::onDiscardCardRequested` |
| `EndPlayRequested` | 无 payload | `GameViewModel::onEndPlayRequested` |
| `AdvanceRequested` | 无 payload | `GameViewModel::onAdvanceRequested` |
| `SkipRequested` | 无 payload | `GameViewModel::onSkipRequested` |

**关键差异（本地模式没有这个问题）**：`PlayCardMsg::playerId`、`RespondCardMsg::responderId`、
`DiscardCardMsg::playerId` 都是客户端消息体里自己填的字段，**服务器分发时一律忽略它们，
改用 `GameServer::clientCommandReceived(int playerId, ...)` 信号里连接层分配的可信
`playerId`**。原因：本地模式下 `GameBoardWidget` 只有一份，发出命令的 `playerId` 天然就是
UI 当前操作者；网络模式下两个客户端是独立连接，如果信任消息体自称的 `playerId`，玩家 1 的
连接可以在消息里填 `playerId=0` 来操作玩家 0 的手牌（`ActionViewModel::isOwnCard` 只按传入的
`playerId` 查归属，不知道请求实际来自哪个连接）。

三个无参命令（`EndPlayRequested`/`AdvanceRequested`/`SkipRequested`）的对应 VM 槽本身不接收
`playerId`（本地模式下只会被当前回合玩家/响应者的 View 触发），网络模式下 `ServerApp` 在转发
前用连接层可信 `playerId` 校验发起方身份：`EndPlay`/`Advance` 要求 `playerId == currentPlayerId()`，
`Skip` 要求 `playerId == GameViewModel::pendingResponderId()`（否则任意一方都能提前结束对方回合、
越权推进阶段、或替对方强制放弃响应）。

**已知限制**：
- `TargetSelectedMsg` 没有可信身份可覆盖——`ActionViewModel::onTargetSelected(int playerIndex)`
  签名本身不接收 `playerId`，而是依赖内部 `m_pendingCardId`/`m_pendingUserId` 单例状态。网络模式下
  如果一个客户端在"不是自己发起多目标选择"期间发送 `TargetSelected`，会被当成当前暂存的那次选择
  处理。修复需要改这个槽的签名，会同时影响本地 `SGSApp`，留待后续（Step 9 / M4）。

**已修复（2026-07-15，第三轮加固）**：`ActionViewModel::canPlayCard`/`playCard`/`discardCard` 原先
全链路只查 `Card::canUse` 的全局 `currentPhase()`、`isOwnCard`，从不校验 `player == currentPlayer()`，
本地模式靠 View 只让当前玩家手牌可交互侥幸未暴露，网络模式下非当前回合玩家能用自己真实身份在对方
回合出/弃自己的牌。另外 `GameState::setPendingAction` 本身无重入保护，出牌路径原先也不查
`!hasPendingAction()`，同一当前回合玩家能在待定响应（如杀等待闪）未结算时再出一张主动牌
（`executeKill`/`executeBarbarianInvasion` 等会静默覆盖第一个待定动作，原响应永久悬空）。
修复方式：在 `ActionViewModel::canPlayCard` 入口补 `player != m_state->currentPlayer()` 和
`m_state->hasPendingAction()` 两条早退校验，`discardCard` 补 `player != m_state->currentPlayer()`——
两处修复都只改了 `ActionViewModel.cpp`，未触碰 `Card.cpp`/`GameRule.cpp`/`GameState.cpp`，不影响甲的
卡牌/规则域。回归测试见 `tests/network_test.cpp` 的
`playCardFromNonCurrentPlayerWithOwnIdentityIsRejected`/
`discardCardFromNonCurrentPlayerWithOwnIdentityIsRejected`/
`secondActiveCardWhilePendingActionUnresolvedIsRejected`。

### 7.3 与本地模式共享的一处修复

`GameViewModel::onDiscardCardRequested` 原实现只按"请求方自己的 `playerId`"查
`getDiscardCount()` 来判断是否 `advancePhase()`——本地模式下这个方法只会被"当前弃牌阶段的
那位玩家"调用（View 只让该玩家的手牌区可交互），所以从未暴露问题；但网络模式下任意已连接客户端
都可能发一条名义上"合法"（`playerId` 是自己真实的、只是不该在这个时机弃牌）的
`DiscardCardRequested`，导致 `getDiscardCount(自己的playerId)` 恒 `<=0`、从而提前结束了
**另一个玩家**的弃牌阶段。已修复为只有 `playerId == currentPlayerId()` 时才允许**推进阶段**，
本地模式行为不受影响（因为本地模式下这个等式恒成立）。

> 这处修复当时只挡住了"提前推进阶段"这一副作用，`ActionViewModel::discardCard` 本体仍不校验
> `playerId == currentPlayer()`；该根因已在上面"已修复（第三轮加固）"一节中一并解决。

### 7.4 第二轮加固：advancePhase 悬空待定动作 + GameStarted 时序

`GameViewModel::advancePhase()` 的 `Play` 分支原本没有 `hasPendingAction()` 检查，与
`endPlayPhase()`（已有此检查）不对称——当前回合玩家出杀后立即发送 `AdvanceRequested`，会把阶段
强制推进到 Discard，闪的待定响应永久悬空（本地模式下 `AutoAdvanceTimer` 从不在 Play 阶段启动，
这条路径从未被触达）。已修复为与 `endPlayPhase()` 一致。

`ServerApp::onBothPlayersReady` 原本先广播 `GameStarted` 再校验 characterId，非法 id 会让客户端
先收到"游戏已开始"、之后却永远等不到任何状态更新。新增 `GameViewModel::isValidCharacterId()`，
广播前先校验，非法 id 直接不广播（协议暂无专门的拒绝消息类型）。

### 7.5 GameClient：客户端网络层（Step 6）

`GameClient`（`src/Network/GameClient.h/cpp`）是"网络化的 ViewModel"——对外暴露与
`GameViewModel`/`ActionViewModel` 完全相同形状的信号，供 `ClientApp`（Step 7）直接连到
`GameBoardWidget` 的槽；发送方法与 `GameBoardWidget` 的命令信号一一对应，供 `ClientApp` 直接连接。
本类零 Model 依赖、零规则判断——所有校验信任服务器结果，与服务器侧 `GameServer` 的"零 Model
依赖、零规则判断"对称。

**发送方法 → MessageType**（`GameBoardWidget` 命令信号 → `ClientApp` 待接线 → `GameClient` 方法）：

| GameClient 方法 | MessageType | 消息结构体 |
|-----------------|-------------|-----------|
| `selectCharacter(int)` | `SelectCharacter` | `SelectCharacterMsg` |
| `playCard(int cardId, int playerId)` | `PlayCardRequested` | `PlayCardMsg` |
| `respondCard(int cardId, int responderId)` | `RespondCardRequested` | `RespondCardMsg` |
| `selectTarget(int playerIndex)` | `TargetSelected` | `TargetSelectedMsg` |
| `discardCard(int cardId, int playerId)` | `DiscardCardRequested` | `DiscardCardMsg` |
| `endPlayPhase()` | `EndPlayRequested` | 无 payload |
| `advancePhase()` | `AdvanceRequested` | 无 payload |
| `skipResponse()` | `SkipRequested` | 无 payload |

**接收信号 ← MessageType**（服务器广播反序列化后直接 `emit`，不经过任何本地计算）：

| GameClient 信号 | MessageType | 消息结构体 |
|-----------------|-------------|-----------|
| `phaseChanged(PhaseType)` | `PhaseChanged` | `PhaseChangedMsg` |
| `playerDataUpdated(int, PlayerData)` | `PlayerDataUpdated` | `PlayerDataMsg` |
| `handCardsUpdated(int, CardList)` | `HandCardsUpdated` | `HandCardsMsg`（服务器已按接收方脱敏，见 7.1.1；`GameClient` 原样转发，不重复处理） |
| `pendingActionCreated(PendingActionData)` | `PendingActionCreated` | `PendingActionMsg` |
| `pendingActionCleared()` | `PendingActionCleared` | 无 payload |
| `gameOver(int)` | `GameOver` | `GameOverMsg` |
| `logMessage(QString)` | `LogMessage` | `LogMessageMsg` |
| `targetSelectionStarted(QVector<int>)` | `TargetSelectionStarted` | `TargetSelectionStartedMsg` |
| `targetSelectionFinished()` | `TargetSelectionFinished` | 无 payload |

`GameClient` 还额外暴露本地模式没有对应物的网络生命周期信号：`connected(int playerId)`（握手成功，
分配的 playerId，即 `localPlayerId()`）、`connectionRejected(QString reason)`（版本不符/房间已满）、
`connectionError(QString)`（socket 层错误）、`disconnected()`、`gameStarted(int char0, int char1)`
（对应 `GameStartedMsg`，本地模式无此信号——本地模式一进游戏页面就已经是双方选好将的状态）。

**不需要 `RemoteGameVM`/`PlayerVMAdapter` 这类只读查询适配层**（plan2.0.md §2.4 已有此结论，此处
按实现确认）：`GameBoardWidget` 从不主动调用 VM 方法拉取状态，全部状态都通过槽被动接收，所以
`GameClient` 只需要转发信号，不需要维护一份本地状态镜像。

回归测试见 `tests/network_test.cpp`：`gameClientHandshakeAssignsPlayerIdsAndRejectsThirdConnection`
（对接真实 `GameServer`，验证 playerId 分配与超员拒绝）、`gameClientSendsAllCommandsWithCorrectPayload`
（7 个发送方法逐一验证 payload 字段）、`gameClientReceivesGameStartedAndBroadcastsWithRedaction`
（对接真实 `ServerApp`，验证 `gameStarted`/`phaseChanged`/`playerDataUpdated`/`logMessage` 正常
emit，且 `handCardsUpdated` 脱敏结果通过 `GameClient` 转发后仍然正确——己方完整、对方占位）、
`gameClientPlayCardRoundTripReflectedInBothClients`（两个 `GameClient` 对接同一个 `ServerApp`，
出杀 → 双方收到 `pendingActionCreated(Dodge)`）、`gameClientDisconnectedSignalFiresWhenServerCloses`
（服务器主动关闭时 `disconnected()` 正确 emit，`localPlayerId()` 复位为 -1）。

### 7.6 ClientApp：客户端组合根（Step 7）

`ClientApp`（`src/App/ClientApp.h/cpp`）是网络模式的组合根，与 `SGSApp::startLocalGame()`
完全对称：创建 `GameBoardWidget` + `GameClient`，在构造函数里把两者的信号/槽按**与本地模式
逐条相同的形状**直连。零业务逻辑，只做两件事之外的事：`connectToServer(host, port)` 和
`selectCharacter(int)` 转发给 `m_client`。

**验收标准已满足：`GameBoardWidget` 零改动。** 之所以能零改动——`GameBoardWidget` 从设计
上就不知道对面接的是本地 `GameViewModel`/`ActionViewModel` 还是网络 `GameClient`，因为
`GameClient` 对外信号/槽形状与本地 VM 完全一致（见 7.5）。`ClientApp` 的连接表逐条对照
`SGSApp.cpp`：

| 方向 | 信号/槽 | 对端 |
|------|---------|------|
| View → Client | `playCardRequested` | `GameClient::playCard` |
| View → Client | `respondCardRequested` | `GameClient::respondCard` |
| View → Client | `targetPlayerSelected` | `GameClient::selectTarget` |
| View → Client | `discardCardRequested` | `GameClient::discardCard` |
| View → Client | `endPlayRequested` | `GameClient::endPlayPhase` |
| View → Client | `advanceRequested` | `GameClient::advancePhase` |
| View → Client | `skipRequested` | `GameClient::skipResponse` |
| Client → View | `phaseChanged` | `GameBoardWidget::onPhaseChanged` |
| Client → View | `playerDataUpdated` | `GameBoardWidget::onPlayerDataUpdated` |
| Client → View | `handCardsUpdated` | `GameBoardWidget::onHandCardsUpdated` |
| Client → View | `targetSelectionStarted` | `GameBoardWidget::onTargetSelectionStarted` |
| Client → View | `targetSelectionFinished` | `GameBoardWidget::onTargetSelectionFinished` |
| Client → View | `pendingActionCreated` | `GameBoardWidget::onPendingActionCreated` |
| Client → View | `pendingActionCleared` | `GameBoardWidget::onPendingActionCleared` |
| Client → View | `gameOver` | `GameBoardWidget::onGameOver` |
| Client → View | `logMessage` | `GameBoardWidget::onLogMessage` |

`ClientApp` 不创建 `MainWindow`——`boardWidget()` 把 `GameBoardWidget*` 暴露给调用方自行嵌入
窗口容器；`gameClient()` 暴露 `GameClient*` 供调用方接入登录/等待界面（联网入口和
`NetworkConfigDialog` 属丙，不在本计划内）。

回归测试见 `tests/network_test.cpp` 的 `clientAppWiresBoardToGameClientWithZeroBoardChanges`：
对接真实 `ServerApp`，起两个客户端（一个用 `ClientApp`，一个用裸 `GameClient` 扮演对手）完成
握手→选将→驱动到 Play 阶段，然后**真实 `QTest::mouseClick` 点击 `GameBoardWidget` 里注入的杀
对应的 `CardWidget`**，断言服务器侧 `Player::handCards()` 确实移除了这张牌——验证从真实 QWidget
点击到网络命令再到服务器结算的完整链路。测试因此把 `network_test.cpp` 从
`QTEST_GUILESS_MAIN` 切到 `QTEST_MAIN`（`ClientApp` 内部创建真实 `GameBoardWidget` 需要
`QApplication`），CMake 侧给 `NetworkTest` 加了 `Qt::Widgets` 链接和 `QT_QPA_PLATFORM=offscreen`
环境变量（CTest 层面，与 `ViewTest` 做法一致）。
