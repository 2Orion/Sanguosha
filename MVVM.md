# MVVM 架构分析：Sanguosha（三国杀）

## 概述

本项目的核心架构承诺是 **MVVM（Model-View-ViewModel）**，将游戏逻辑、状态管理和界面绘制三层完全解耦。三层之间通过 **Qt 信号槽** 进行单向或双向通信，保证了依赖方向的严格性。

---

## 1. 总体架构

```
                    Composition Root (src/App/GameBootstrap)
                    创建对象 + 建立信号连接
                         │
         ┌───────────────┼───────────────┐
         ▼               ▼               ▼
    ┌─────────┐    ┌──────────┐    ┌──────────┐
    │  Model  │    │ViewModel │    │   View   │
    │ QObject │───►│ QObject  │───►│ QWidget  │
    │  +信号   │    │  +信号/槽 │    │  +信号/槽 │
    └─────────┘    └──────────┘    └──────────┘
         │               │               │
         ▼               ▼               ▼
    ┌─────────────────────────────────────────┐
    │         Qt 事件循环（主线程）              │
    │   所有 emit / signal / slot 均在主线程    │
    └─────────────────────────────────────────┘
```

### 依赖方向

```
View ──►  include ViewModel 头文件
ViewModel ──► include Model 头文件（内部通过 int ID 对外隔离）
Model ──►  QObject（qt核心）
Common ──► 所有层引用（纯枚举 + 转发声明）
```

与经典 MVVM 不同，本项目没有使用 Command 模式来包装 View→ViewModel 的调用，而是通过 View 持有 ViewModel 指针直接调用方法。这是一种在 C++/Qt 中的务实简化——避免了不必要的间接层。

---

## 2. 各层职责分析

### 2.1 Model 层（QObject + Qt 信号）

**文件**：`src/Model/`（GameState, Player, Card, GameRule, CardManager, Character）

**职责**：
- 定义游戏数据结构（牌堆、手牌、玩家、武将）
- 实现游戏规则（出牌逻辑、伤害结算、技能触发、胜负判定）
- 通过 Qt 信号通知外部状态变化

```cpp
// Player.h — Model 层的信号定义
class Player : public QObject {
    Q_OBJECT
signals:
    void hpChanged(int hp);
    void dying(int playerId);
    void handCardAdded(int cardId);
    void stateChanged();
};
```

**关键特征**：
- 继承 `QObject`，使用 `Q_OBJECT` 宏
- 状态变更通过 `emit signalName(args)` 广播
- 信号参数使用值类型（`int`, `PhaseType`），避免裸指针暴露
- `Card` 和 `GameRule` 保持纯 C++ 类（无 QObject），因为它们不发布事件

### 2.2 ViewModel 层（QObject + Qt 信号/槽）

**文件**：`src/ViewModel/`（GameViewModel, PlayerViewModel, CardViewModel, ActionViewModel）

**职责**：
- 包装 Model 对象，为 View 提供可直接消费的状态和方法
- 在构造函数中用 `connect()` 将 Model 信号直接转发为自身同名信号
- 将 Model 的内部结构（`PendingActionInfo`）翻译为 View 友好的值类型（`PendingActionVM`）
- 管理 UI 相关的临时状态（卡牌是否选中/可打出）

#### 信号转发模式

PlayerViewModel 是最典型的例子——直接将 Model 的 Qt 信号连接到自己信号上：

```cpp
// PlayerViewModel 构造函数中
connect(m_player, &Player::hpChanged,     this, &PlayerViewModel::hpChanged);
connect(m_player, &Player::dying,         this, &PlayerViewModel::dying);
connect(m_player, &Player::handCardsChanged, this, &PlayerViewModel::handCardsChanged);
connect(m_player, &Player::stateChanged,  this, &PlayerViewModel::stateChanged);
```

**不需要任何手动 disconnect。** Qt 在 QObject 析构时自动断开所有连接。

#### PendingActionVM 值类型隔离

Model 层的 `PendingActionInfo` 包含裸指针：

```cpp
// Model 层 (GameState.h) — 裸指针版本
struct PendingActionInfo {
    Player* source = nullptr;           // ← 裸指针
    Player* target = nullptr;
    Card*   sourceCard = nullptr;
    std::vector<Player*> remainingTargets;
};
```

ViewModel 层在 `GameViewModel::onModelPendingActionCreated` 中将其翻译为纯值类型：

```cpp
// ViewModel 层 (GameViewModel.h) — 值类型版本
struct PendingActionVM {
    int sourceId = -1;
    int targetId = -1;
    int sourceCardId = -1;
    CardType requiredCardType;
    QString description;
    bool canSkip = false;
    std::vector<int> remainingTargetIds;
};
```

这样就保证了 View 层永远不需要 `#include` 任何 Model 头文件。

### 2.3 View 层（Qt Widgets + 信号槽）

**文件**：`src/View/`（7 个控件模块）

**职责**：
- 绘制游戏界面（卡牌渲染、布局排列、状态动画）
- 捕获用户输入（点击、双击、悬停）
- 通过 `connect()` 接收 ViewModel 信号更新 UI
- 通过 ViewModel 指针直接调用方法触发游戏操作

```cpp
// GameBoardWidget 中连接 ViewModel 信号到槽
connect(m_gvm, &GameViewModel::phaseChanged, this, &GameBoardWidget::onPhaseChanged);
connect(m_gvm, &GameViewModel::stateChanged, this, &GameBoardWidget::refreshDisplay);
connect(m_gvm, &GameViewModel::logMessage,   this, &GameBoardWidget::showLog);
```

### 2.4 Composition Root（组合根）

**文件**：`src/App/GameBootstrap`

组合根是应用程序中唯一了解所有三层具体类的模块。它负责：

1. 创建 GameViewModel（Model + ViewModel 层的聚合）
2. 创建 GameBoardWidget（View 层的主控件）
3. 将它们嵌入 MainWindow 的页面堆栈

GameBootstrap 承担了「依赖注入」的职责，使得三层之间不需要互相构造。

---

## 3. 关键技巧总结

### 3.1 Qt 信号/槽统一通信

Model → ViewModel → View 的所有事件流全部使用 Qt 信号/槽：

```
Player::hpChanged(int)
  → (直接 connect) → PlayerViewModel::hpChanged(int)
    → (直接 connect) → PlayerInfoWidget::updateHp()
```

优势：
- **自动生命周期管理**：QObject 析构时自动断开所有连接
- **类型安全**：信号/槽签名在编译期检查
- **零样板代码**：无需手动 connect/disconnect ID 管理
- **线程安全**：跨线程信号/槽自动序列化

### 3.2 值类型隔离（PendingActionVM vs PendingActionInfo）

ViewModel 层对外部只暴露值类型（`int`, `QString`, `PhaseType`, `PendingActionVM`），内部通过私有查找方法（`findPlayer()`, `findCard()`）将 int ID 转换为 Model 指针。View 层从不直接操作 Model 对象。

### 3.3 查找辅助方法

对于 View 层需要的显示数据，GameViewModel 提供了一系列查找辅助方法：

```cpp
QString cardNameById(int cardId) const;
CardType cardTypeById(int cardId) const;
QString playerDisplayName(int playerId) const;
```

View 层持有 int ID 调用这些方法获取显示数据，无需直接接触 Model 对象。

### 3.4 CardViewModel 的瞬态生命周期

```cpp
std::vector<std::unique_ptr<CardViewModel>> getPlayerCardVMs(int playerIndex) const;
```

CardViewModel 不是常驻的——每次需要显示手牌时重新创建。生命周期由 `HandCardAreaWidget` 通过 `std::move` 接管所有权。旧控件和旧 ViewModel 在刷新时立即销毁，避免悬垂指针。

### 3.5 Composition Root 统一管理

`GameBootstrap` 负责所有对象的创建和生命周期：

```
GameBootstrap::startLocalGame(charId1, charId2)
  ├── new GameViewModel(this)        ← Model + ViewModel
  ├── new GameBoardWidget(gvm, parent) ← View
  ├── connect(board, &gameFinished, this, &gameFinished)
  └── gvm->startGame(charId1, charId2)
```

GameViewModel 和 GameBoardWidget 的生命周期由 Qt 父子关系自动管理——GameBootstrap 析构时自动清理所有子对象。

---

## 4. View 层详细分析

### 4.1 模块清单

| 控件 | 文件 | 职责 |
|------|------|------|
| MainWindow | `MainWindow.h/cpp` | QMainWindow，选将页面和游戏页面的 QStackedWidget 切换 |
| GameBoardWidget | `GameBoardWidget.h/cpp` | 游戏桌面总布局，核心桥接层 |
| CardWidget | `CardWidget.h/cpp` | 单张卡牌的 QFrame，手绘渲染 |
| PlayerInfoWidget | `PlayerInfoWidget.h/cpp` | 玩家信息面板（体力、技能、手牌数） |
| HandCardAreaWidget | `HandCardAreaWidget.h/cpp` | 手牌区域（多张卡牌的排列与交互） |
| ActionPanelWidget | `ActionPanelWidget.h/cpp` | 操作按钮面板 |

### 4.2 GameBoardWidget — 核心协调器

#### 事件分发（ViewModel → View）

```
GameViewModel::phaseChanged
  → (Qt 信号) → GameBoardWidget::onPhaseChanged(PhaseType)
    → ActionPanelWidget::updateForPhase()
    → m_autoAdvanceTimer / advancePhase()
    → refreshDisplay()
```

GameBoardWidget 通过 7 个 Qt 信号连接接收 ViewModel 的状态更新：

| ViewModel 信号 | View 槽 | 作用 |
|---------------|--------|------|
| `phaseChanged` | `onPhaseChanged` | 阶段切换 → 更新按钮、自动推进 |
| `currentPlayerChanged` | `onPlayerChanged` | 当前玩家变更 → 切换信息面板 |
| `gameOver` | `onGameOver` | 游戏结束 → 弹窗 + 返回选将 |
| `stateChanged` | `refreshDisplay` | 通用刷新 → 重绘所有区域 |
| `logMessage` | `showLog` | 日志消息 → 更新提示条 |
| `pendingActionCreated` | `onPendingActionCreated` | 响应待定 → 进入响应模式 |
| `pendingActionCleared` | `onPendingActionCleared` | 响应清除 → 退出响应模式 |

#### 交互路由（View → ViewModel）

```
用户操作 → CardWidget / PlayerInfoWidget / ActionPanelWidget
  → (Qt 信号) → GameBoardWidget 的槽
    → (调用 ActionViewModel / GameViewModel 方法)
      → Model 状态变更 → Qt 信号通知 → UI 更新
```

### 4.3 状态机

GameBoardWidget 维护一个 `State` 枚举控制交互模式：

```cpp
enum class State { Idle, SelectingTarget, Responding, Discarding };
State m_state = State::Idle;
```

- **Idle（出牌阶段）**：双击/单击打出牌
- **SelectingTarget（目标选择）**：高亮可点击的玩家头像
- **Responding（响应阶段）**：高亮可响应的手牌 +「跳过」按钮
- **Discarding（弃牌阶段）**：选择要弃的牌 +「确认弃牌」

---

## 5. MVVM 符合度评估

### 完全符合

| 原则 | 实现 |
|------|------|
| Model 不依赖 View/ViewModel | ✅ QObject + 纯 Qt::Core，无任何 UI 依赖 |
| ViewModel 不依赖 View | ✅ QObject + 纯 Qt::Core，不引入 Qt Widgets |
| View 不直接访问 Model | ✅ View 所有数据通过 ViewModel 的 int ID 查询或信号获取 |
| 事件驱动 | ✅ 所有 Model→ViewModel→View 通过 Qt 信号传递 |
| 展示状态分离 | ✅ CardViewModel 管理 selected/playable/highlighted |
| 可测试性 | ✅ Model 层 95 项冒烟测试独立通过 |
| Composition Root | ✅ src/App/GameBootstrap 统一创建并连接 |

### 部分简化

| 方面 | 说明 | 评估 |
|------|------|------|
| View→ViewModel 缺少 Command 模式 | View 调用 ViewModel 方法而非通过 Command 对象 | 对于 C++/Qt 是合理的简化 |
| View 持有 ViewModel 裸指针 | `GameBoardWidget::m_gvm` | 生命周期由 QObject 父子关系保证 |
| CardViewModel 持有 `Card*` | CardViewModel 内部保持 Card 指针 | 瞬态对象，不暴露给 View |

### 与重构前的对比

| 维度 | 重构前（EventListener） | 重构后（Qt 信号槽） |
|------|----------------------|-------------------|
| **事件机制** | `EventListener<T>::connect/notify` | `QObject::connect` + `emit` |
| **生命周期** | 手动管理 connect/disconnect ID | Qt 父子关系自动管理 |
| **线程安全** | `QMetaObject::invokeMethod` + QueuedConnection | Qt 内置 |
| **Model 依赖** | 纯 C++17，零 Qt | QObject + Qt::Core |
| **通信代码量** | 每事件 3-5 行桥接代码 | 1 行 `connect()` |
| **ViewModel 可移植性** | 可在纯 C++ 环境中复用 | 需 Qt 环境 |

### 结论

**本项目完全实现了 MVVM 架构。** 重构后三层全部使用 Qt 信号槽进行通信，消除了自定义 EventListener 的维护成本，获得了 Qt 框架的自动生命周期管理和类型安全检查。ViewModel 层的值类型隔离设计（`PendingActionVM`、int ID 查找辅助）确保了 View 层无法越过边界访问 Model，保持了 MVVM 的核心约束。
