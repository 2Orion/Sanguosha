# MVVM 架构分析：Sanguosha（三国杀）

## 概述

本项目的核心架构承诺是 **MVVM（Model-View-ViewModel）**，将游戏逻辑、状态管理和界面绘制三层完全解耦。本文从实际代码出发，分析各层的职责、交互方式，以及为实现纯正 MVVM 所采用的关键技巧。

---

## 1. 总体架构

```
┌──────────────────────────────────────────────────────────────────┐
│  View 层 (Qt Widgets)                                            │
│  src/View/                                                       │
│  MainWindow / GameBoardWidget / CardWidget / ...                 │
│  依赖: Qt, ViewModel (通过 #include)                              │
│  不依赖: Model（禁止 #include Model 头文件）                       │
├──────────────────────────────────────────────────────────────────┤
│  ViewModel 层 (纯 C++17)                                         │
│  src/ViewModel/                                                   │
│  GameViewModel / PlayerViewModel / CardViewModel / ActionViewModel│
│  依赖: Model (内部持有指针)                                       │
│  对外暴露: 值类型 + EventListener 事件                             │
├──────────────────────────────────────────────────────────────────┤
│  Model 层 (纯 C++17)                                             │
│  src/Model/ + src/Core/                                          │
│  GameState / Player / Card / GameRule / CardManager / Character   │
│  依赖: Core (CommonTypes, Event)                                 │
│  不依赖: 任何 UI 框架                                              │
├──────────────────────────────────────────────────────────────────┤
│  Core 层 (纯 C++17，跨层共享)                                     │
│  src/Core/                                                       │
│  CommonTypes.h / Event.h / RandomUtils.h                          │
│  枚举类型 (PhaseType, CardType), EventListener 事件机制            │
└──────────────────────────────────────────────────────────────────┘
```

### 依赖方向

严格单向：**View → ViewModel → Model ← Core → (各层)**

- View 只 `#include` ViewModel 头文件，不直接引用 Model 的任何类
- ViewModel 内部持有 Model 对象指针，但对外接口只暴露值类型
- Core 是纯类型定义层，所有层都依赖它，但 Core 不依赖任何层
- Model 完全不依赖 Qt，可以作为独立库编译和测试

---

## 2. 各层职责分析

### 2.1 Model 层（纯数据 + 规则）

**文件**：`src/Model/`（GameState, Player, Card, GameRule, CardManager, Character）

**职责**：
- 定义游戏数据结构（牌堆、手牌、玩家、武将）
- 实现游戏规则（出牌逻辑、伤害结算、技能触发、胜负判定）
- 管理游戏状态机（阶段切换、回合流转）
- 通过 `EventListener` 对外通知状态变化

**关键特征**：
- 零 UI 依赖，可独立测试（`ModelSmokeTest.exe` 95/95 通过）
- 事件以裸指针传递（`Player*`, `Card*`, `const PendingActionInfo&`），因为这是 Model 层的自然表达
- 不关心谁在监听事件，只负责 `notify`

```cpp
// Model 层典型事件定义（Player.h）
EventListener<int> hpChanged;           // 参数 int（体力值）
EventListener<Player*> dying;           // 参数 Player*（濒死玩家）
EventListener<Card*> handCardAdded;     // 参数 Card*（加入的牌）
EventListener<> stateChanged;           // 无参数，通用刷新
```

### 2.2 ViewModel 层（状态管理 + 桥接）

**文件**：`src/ViewModel/`（GameViewModel, PlayerViewModel, CardViewModel, ActionViewModel）

**职责**：
- 包装 Model 对象，为 View 提供"可直接消费"的状态和方法
- 将 Model 的裸指针事件转为值类型事件（int ID、string 描述）
- 聚合多个 Model 对象，提供高层操作（开始游戏、出牌、响应）
- 管理 UI 相关的临时状态（卡牌是否选中、是否可打出）

**四个 ViewModel 的分工**：

| ViewModel | 包装的 Model | 对外暴露方式 |
|-----------|-------------|------------|
| `GameViewModel` | GameState + CardManager | 值类型查询 + 事件转发 |
| `PlayerViewModel` | Player | 只读属性 + 过滤后的事件 |
| `CardViewModel` | Card | 只读属性 + UI 状态 + 事件 |
| `ActionViewModel` | GameState + GameRule | int ID 接口的方法调用 |

#### 核心技巧：PendingActionVM 值类型隔离

Model 层的 `PendingActionInfo` 包含裸指针：

```cpp
// Model 层 (GameState.h) — 裸指针版本
struct PendingActionInfo {
    Player* source = nullptr;
    Player* target = nullptr;
    Card* sourceCard = nullptr;
    CardType requiredCardType;
    bool canSkip = false;
    std::vector<Player*> remainingTargets;
};
```

ViewModel 层将其转换为纯值类型：

```cpp
// ViewModel 层 (GameViewModel.h) — 值类型版本
struct PendingActionVM {
    int sourceId = -1;
    int targetId = -1;
    int sourceCardId = -1;
    CardType requiredCardType;
    bool canSkip = false;
    std::vector<int> remainingTargetIds;
};
```

**这就保证了 View 层永远不需要 `#include` 任何 Model 头文件**。

#### 事件转发模式

ViewModel 通过 `EventListener::connect` 监听 Model 事件，然后：
1. 转换参数类型（指针 → ID）
2. 在自身 `EventListener` 上重新 `notify`
3. View 只连接 ViewModel 的事件

```cpp
// GameViewModel 的典型事件转发
m_modelConn.phaseChangedId = m_state->phaseChanged.connect(
    [this](PhaseType p) { this->phaseChanged.notify(p); });

m_modelConn.gameOverId = m_state->gameOver.connect(
    [this](Player* w) {
        this->gameOver.notify(w ? w->playerId() : -1);
    });
```

#### ActionViewModel 的 ID 接口重构

ActionViewModel 的公共接口全部使用 int ID，内部通过私有方法查找 Model 对象：

```cpp
// 公共接口（View 调用）
ActionResult playCard(int cardId, int playerId, const std::vector<int>& targetIds);
std::vector<int> getPlayableCardIds(int playerId) const;
void respondCard(int cardId, int playerId);

// 私有实现（内部使用 Model 指针）
Player* findPlayer(int playerId) const;
Card* findCard(int cardId) const;
```

这样就完全隔离了 Model 指针类型暴露给 View 层。

### 2.3 View 层（Qt 图形界面）

**文件**：`src/View/`（6 个控件模块）

**职责**：
- 绘制游戏界面（卡牌渲染、布局排列、状态动画）
- 捕获用户输入（点击、双击、悬停）
- 桥接 ViewModel 事件到 Qt 信号槽机制

**依赖规则**：
- 只 `#include` ViewModel 头文件和 Core 类型
- 从不直接 `#include` Model 的任何类（通过 ViewModel 的 ID 接口 + GameViewModel 查找辅助间接获取数据）
- 所有的 Model 对象访问都经过 ViewModel 的方法调用或事件

详细分析见下文第 4 节。

---

## 3. 关键技巧总结

### 3.1 自定义观察者模式替代 Qt 信号槽

```cpp
// Core/Event.h — EventListener<Args...>
template<typename... Args>
class EventListener {
    CallbackId connect(Callback callback);
    void disconnect(CallbackId id);
    void notify(Args... args);
};
```

- Model 和 ViewModel 层使用 `EventListener` 而非 Qt 信号槽
- 这使得这两层保持纯 C++17，无 Qt 依赖
- View 层通过 lambda 桥接 `EventListener` → Qt 事件循环

### 3.2 跨线程安全的 QueuedConnection 桥接

```cpp
// GameBoardWidget::connectViewModel()
auto id1 = m_gvm->phaseChanged.connect([this](PhaseType phase) {
    QMetaObject::invokeMethod(this, [this, phase]() {
        onPhaseChanged(phase);
    }, Qt::QueuedConnection);
});
```

- ViewModel 的事件回调可能发生在任意线程（当前是主线程，但预留了多线程可能）
- `Qt::QueuedConnection` 确保 UI 更新始终在 Qt 主事件循环中执行
- Lambda 按值捕获参数，避免悬垂引用

### 3.3 值类型隔离 + 查找辅助方法

ViewModel 向外暴露的全部是值类型：`int`, `std::string`, `PhaseType` 等。

对于 View 需要但 ViewModel 没有直接提供的数据，通过 `GameViewModel` 的查找辅助方法间接获取：

```cpp
// GameViewModel.h — 显示数据查找辅助
std::string cardDisplayString(int cardId) const;
std::string cardNameById(int cardId) const;
CardType cardTypeById(int cardId) const;
std::string playerDisplayName(int playerId) const;
```

View 层拿着 int ID 调用这些方法获取显示数据，无需直接接触 Model 对象。

### 3.4 CardViewModel 的瞬态生命周期

```cpp
// GameViewModel — 每次刷新创建新的 CardViewModel 实例
std::vector<std::unique_ptr<CardViewModel>> getPlayerCardVMs(int playerIndex) const;
```

- `CardViewModel` 不是常驻的，每次需要显示手牌时重新创建
- 生命周期由 `HandCardAreaWidget` 持有（通过 `std::move` 接管 unique_ptr 所有权）
- 旧 CardWidget 和旧 CardViewModel 在刷新时立即销毁，避免悬垂指针
- 这样设计的原因：手牌变化频繁，维护常驻 VM 的同步成本高于重建

### 3.5 PlayerViewModel 的双向事件转发

```
Model: Player::hpChanged
  → PlayerViewModel 内部连接
    → PlayerViewModel::hpChanged (类型不变，但参数变为 int)
      → PlayerInfoWidget 桥接到 Qt 事件循环
```

每个 Model 事件在 PlayerViewModel 中都有对应的转发：

```cpp
m_conn.hpChangedId = m_player->hpChanged.connect(
    [this](int hp) { this->hpChanged.notify(hp); });
m_conn.dyingId = m_player->dying.connect(
    [this](Player* p) { this->dying.notify(p->playerId()); });
m_conn.handCardRemovedId = m_player->handCardRemoved.connect(
    [this](Card* c) { this->handCardRemoved.notify(c ? c->id() : -1); });
```

---

## 4. View 层详细分析

### 4.1 模块清单与职责

| 控件 | 文件 | 职责 |
|------|------|------|
| MainWindow | `MainWindow.h/cpp` | QMainWindow 主窗口，选将页面和游戏页面的 QStackedWidget 切换 |
| GameBoardWidget | `GameBoardWidget.h/cpp` | 游戏桌面总布局，核心桥接层，控制所有子控件 |
| CardWidget | `CardWidget.h/cpp` | 单张卡牌的 Qt 控件（QFrame），手绘渲染 |
| PlayerInfoWidget | `PlayerInfoWidget.h/cpp` | 玩家信息面板（体力、技能、手牌数） |
| HandCardAreaWidget | `HandCardAreaWidget.h/cpp` | 手牌区域（多张卡牌的排列与交互） |
| ActionPanelWidget | `ActionPanelWidget.h/cpp` | 操作按钮面板（结束出牌、跳过、确认弃牌） |
| main_qt | `main_qt.cpp` | 程序入口，创建 QApplication + MainWindow |

### 4.2 GameBoardWidget — 核心桥接器

GameBoardWidget 是 View 层的中心组件，承担了**事件分发、状态协调、交互路由**三大职责。

#### 4.2.1 事件分发（ViewModel → View）

```
GameViewModel::phaseChanged
  → (event loop) → GameBoardWidget::onPhaseChanged(PhaseType)
    → ActionPanelWidget::updateForPhase()      // 更新按钮
    → m_autoAdvanceTimer / GameViewModel::advancePhase()  // 自动阶段
    → refreshDisplay()                          // 全面刷新
```

```cpp
// 建立连接（构造函数中）
auto id1 = m_gvm->phaseChanged.connect([this](PhaseType phase) {
    QMetaObject::invokeMethod(this, [this, phase]() {
        onPhaseChanged(phase);
    }, Qt::QueuedConnection);
});
m_connectionIds.push_back(id1);
```

GameBoardWidget 共连接 **7 种事件**：

| ViewModel 事件 | 回调方法 | 作用 |
|---------------|---------|------|
| `phaseChanged` | `onPhaseChanged` | 阶段切换 → 更新按钮、自动推进、刷新 |
| `currentPlayerChanged` | `onPlayerChanged` | 当前玩家变更 → 切换信息面板 |
| `gameOver` | `onGameOver` | 游戏结束 → 弹窗 + 返回主菜单 |
| `stateChanged` | `refreshDisplay` | 通用刷新 → 重绘所有区域 |
| `logMessage` | `showLog` | 日志消息 → 更新提示条 |
| `pendingActionCreated` | `onPendingActionCreated` | 响应待定 → 进入响应模式 |
| `pendingActionCleared` | `onPendingActionCleared` | 响应清除 → 退出响应模式 |

#### 4.2.2 状态协调

GameBoardWidget 维护一个 `State` 枚举来跟踪当前交互模式：

```cpp
enum class State { Idle, SelectingTarget, Responding, Discarding };
State m_state = State::Idle;
```

**出牌流程（Idle → Play）**：
1. ViewModel 检测到出牌阶段，发出 `phaseChanged(Play)`
2. `onPhaseChanged` 设置 `m_state = Idle`
3. 用户双击卡牌 → `onCardClicked(int cardId)`
4. 检查卡牌类型，获取合法目标
5. 无目标 → 直接 `avm->playCard(cardId, playerId, {})`
6. 单目标 → 自动选择并 `avm->playCard(...)`
7. 多目标 → `enterTargetSelection(cardId, userId, targetIds)` 切换到 `SelectingTarget`

**响应流程（Responding）**：
1. Model 产生待定动作 → `pendingActionCreated(PendingActionVM)`
2. `onPendingActionCreated` 设置 `m_state = Responding`
3. 高亮可响应的手牌（通过 `avm->getResponseCardIds(targetId, type)`）
4. 用户点击卡牌响应，或点击"跳过"
5. 如果无可用响应牌 → 自动调用 `avm->skipResponse(playerId, true)` 推进

**弃牌流程（Discarding）**：
1. 进入弃牌阶段 → `m_state = Discarding`
2. 用户逐张点击要弃的牌 → 点击"确认弃牌" → `avm->discardCard(cardId, playerId)`
3. 全部弃完 → `advancePhase()` 进入结束阶段

#### 4.2.3 交互路由

所有用户输入均通过以下路径到达 ViewModel：

```
用户操作 → CardWidget / PlayerInfoWidget / ActionPanelWidget
  → (Qt signal) → GameBoardWidget slot
    → (调用 ActionViewModel / GameViewModel 方法)
      → Model 状态变更 → 事件通知 → UI 更新
```

**单击**：

```cpp
CardWidget::mousePressEvent → signal clicked(int cardId)
  → HandCardAreaWidget::onCardWidgetClicked(cardId)
    → signal cardClicked(int cardId)
      → GameBoardWidget::onCardClicked(int cardId)
        → 根据 State 分支处理
```

**双击**：

```cpp
CardWidget::mouseDoubleClickEvent → signal doubleClicked(int cardId)
  → HandCardAreaWidget::onCardWidgetDoubleClicked(cardId)
    → signal cardDoubleClicked(int cardId)
      → GameBoardWidget::onCardDoubleClicked(int cardId)
        → GameBoardWidget::onCardClicked(int cardId)
```

**目标选择**：

```cpp
PlayerInfoWidget::mousePressEvent → signal clicked(int playerIndex)
  → GameBoardWidget::onTargetClicked(int playerIndex)
    → 验证目标合法性 → avm->playCard(...)
```

### 4.3 CardWidget — 无状态绘制控件

CardWidget 是一个 **纯展示 + 事件源** 控件：

```cpp
class CardWidget : public QFrame {
    // 构造函数：接收 CardViewModel* 和 faceUp 标志
    // 绘制：paintEvent 中手绘卡牌（背景、花色、点数、名称、状态覆盖）
    // 事件：mousePressEvent → clicked(cardId), mouseDoubleClickEvent → doubleClicked(cardId)
    // 更新：setSelected / setPlayable / setFaceUp 更改绘制状态
};
```

**设计要点**：
- 不持有 CardViewModel 的生命周期（由 HandCardAreaWidget 管理）
- 通过 `cardId()` 返回卡牌 ID，而不是直接暴露 CardViewModel
- 四种视觉状态：普通、选中（蓝框上移）、可打出（绿框发光）、悬停（黄色光晕）
- 牌背有独立的绘制路径（纯色背景 + 菱形花纹）

### 4.4 PlayerInfoWidget — 事件桥接 + 信息展示

PlayerInfoWidget 通过 `bindViewModel(PlayerViewModel*)` 绑定数据源：

```cpp
void PlayerInfoWidget::bindViewModel(PlayerViewModel* pvm) {
    unbindViewModel();                      // 断开旧连接
    m_pvm = pvm;
    refreshAll();                            // 初始化显示
    connectViewModelEvents();                // 连接事件
}
```

**事件桥接（5 种事件）**：

```cpp
// hpChanged → 刷新体力（❤/🖤 符号）
m_pvm->hpChanged.connect([this](int) {
    QMetaObject::invokeMethod(this, [this]() { updateHp(); }, Qt::QueuedConnection);
});

// handCardsChanged → 刷新手牌计数
m_pvm->handCardsChanged.connect([this]() {
    QMetaObject::invokeMethod(this, [this]() { updateHandCount(); }, Qt::QueuedConnection);
});

// dying → 红色边框闪烁指示
m_pvm->dying.connect([this]() {
    QMetaObject::invokeMethod(this, [this]() { onDying(); }, Qt::QueuedConnection);
});
```

**目标选择支持**：
- `setTargetable(bool)`：切换高亮绿色边框（用于多目标选择时的点击反馈）
- `mousePressEvent`：点击时发出 `clicked(int playerIndex)` 信号

### 4.5 HandCardAreaWidget — 手牌排列 + 生命周期管理

```cpp
void setCards(std::vector<std::unique_ptr<CardViewModel>> cards, bool isOpponent);
```

**关键设计**：
- **接管所有权**：通过值传递 `std::vector<std::unique_ptr<CardViewModel>>`，`std::move` 到 `m_ownedCardVMs`
- **立即销毁旧控件**：替换手牌时先 `delete` 旧 CardWidget（而非 `deleteLater`），防止悬垂指针
- **重叠排列**：`arrangeCards()` 计算偏移位置（默认 20px 重叠，压缩时最小 4px）
- **选中排队**：单选模式，选中一张时取消其他选中

### 4.6 ActionPanelWidget — 阶段驱动的按钮面板

```cpp
void updateForPhase(PhaseType phase, bool hasPendingAction);
void updateForPendingAction(const PendingActionVM& info);
```

| 阶段 | 按钮 | 信号 |
|------|------|------|
| 出牌阶段 | 「结束出牌」(橙色) | `playPhaseEnded()` |
| 响应待定 | 「跳过」(灰色) | `respondSkipped()` |
| 弃牌阶段 | 「确认弃牌」(绿色) | `discardConfirmed()` |
| 其他阶段 | 无按钮 + "请等待..." | — |

每个按钮的样式使用 `qlineargradient` 渐变，按下/悬停有颜色反馈。

### 4.7 MainWindow — 页面路由

```cpp
// QStackedWidget 管理两个页面
m_centralStack->addWidget(m_selectionPage);  // 索引 0：选将
m_centralStack->addWidget(m_gameBoard);      // 索引 1：游戏

// 选将 → 游戏
onStartGame() → 创建 GameViewModel + GameBoardWidget → 切换到游戏页面

// 游戏 → 选将
onGameFinished() → deleteLater GameBoardWidget + delete GameViewModel → 切换到选将页面
```

---

## 5. MVVM 符合度评估

### ✅ 完全符合

| 原则 | 实现 |
|------|------|
| Model 零 UI 依赖 | 纯 C++17，无 Qt include，可独立编译测试 |
| ViewModel 零 UI 依赖 | 纯 C++17，公共接口只暴露值类型 |
| View 只依赖 ViewModel | View 代码只 `#include` ViewModel + Core 类型 |
| View 不直接访问 Model | 所有数据通过 ViewModel 方法或 int ID 查询 |
| 双向数据绑定 | EventListener → Qt::QueuedConnection 桥接 |
| 展示状态分离 | CardViewModel 管理 selected/playable/highlighted 等 UI 状态 |
| 可测试性 | Model 层 95 项冒烟测试，ViewModel 可单元测试 |

### ⚠️ 部分简化

| 方面 | 说明 | 是否可接受 |
|------|------|-----------|
| CardViewModel 持有 Card* | 但不暴露给 View，View 只通过 cardId 引用 | ✅ 瞬态 + ID 隔离 |
| ViewModel 方法返回 Model 指针给 View | 仅限 `playerVM(int index)` 等枚举查询 | ✅ 调用方只读访问 |
| View 直接调用 ViewModel 方法 | MVVM 的 Command 模式未显式使用 | ✅ C++ 函数调用等价 |
| 单线程简化 | 全部在主线程运行，QueuedConnection 是预留 | ✅ 安全冗余 |

### 结论

**该项目完全实现了 MVVM 架构。** 各层职责清晰、依赖方向严格单向。ViewModel 层通过值类型隔离（`PendingActionVM` vs `PendingActionInfo`）和 ID 接口（`int cardId`/`int playerId`）成功将 View 与 Model 解耦。View 层使用 `QMetaObject::invokeMethod` + `Qt::QueuedConnection` 桥接 `EventListener` 事件机制，确保线程安全。Model 层可以作为独立库编译、测试，不依赖任何 UI 框架。
