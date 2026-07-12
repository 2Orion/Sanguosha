# Changelog

## [Unreleased] — MVVM 严格化重构

### 架构：MVVM 分层修复

**问题**：ViewModel 层通过 `Card* card()`、`Player* player()` 等方法暴露原始 Model 指针，View 层拿到指针后直接调用 Model 方法（如 `card->cardName()`、`player->displayName()`），绕过了 ViewModel，违反了 MVVM 原则。

**修复**：ViewModel 层公共接口现在只暴露值类型（`int`、`std::string`、`bool`），View 层通过 ViewModel 获取所有数据。

#### ViewModel 层

- **CardViewModel** — 移除 `Card* card()` 公共访问器；所有显示数据通过已有透传方法获取（`cardName()`、`suitSymbol()` 等）
- **PlayerViewModel** — 移除 `Player* player()` 和 `const std::vector<Card*>& handCards()`；事件类型 `Player*` → `int`/`void`（`dying`、`died`、`revived`、`handCardAdded`、`handCardRemoved`）
- **GameViewModel** — 新增 `PendingActionVM` 结构体（不含原始指针的 ViewModel 层 DTO）；`currentPlayer()` → `currentPlayerId()`；`opponentPlayer()` → `opponentPlayerId()`；`winner()` → `winnerId()`；`pendingActionInfo()` → `pendingActionVM()`；`gameOver` 事件 `Player*` → `int winnerId`；新增 `cardDisplayString()`、`cardNameById()`、`playerDisplayName()` 等值类型辅助方法；原始指针方法移至 `private`
- **ActionViewModel** — 所有公共方法参数和返回值 `Card*`/`Player*` → `int cardId`/`int playerId`；内部通过 `findCard()`/`findPlayer()` 查找 Model 对象；保留 `getPlayableCards(Player*)` 为 `private` 供内部使用

#### View 层

- **GameBoardWidget** — 所有交互状态 `Card*`/`Player*` → `int` ID；事件处理器改用 `PendingActionVM` 和 `int winnerId`；显示数据通过 `CardViewModel`/`PlayerViewModel` 查找
- **HandCardAreaWidget** — 信号 `cardClicked(Card*)` → `cardClicked(int cardId)`；`selectedCard()` → `selectedCardId()`；新增 `cardVM(int cardId)` 查找器
- **ActionPanelWidget** — `updateForPendingAction()` 参数 `PendingActionInfo` → `PendingActionVM`
- **PlayerInfoWidget** — 更新 `dying` 事件回调以匹配新的 `EventListener<>` 类型
- **main.cpp**（控制台版） — 完全重写，使用 ViewModel 值类型接口，零原始 Model 指针访问

### 构建系统

- **CMakeLists.txt** — 拆分为 3 个独立 target：
  - `sgs_console` — 控制台版（`src/main.cpp`）
  - `sgs_qt` — Qt GUI 版（`src/View/main_qt.cpp`）
  - `sgs_test` — Model 冒烟测试（`tests/smoke_test.cpp`）
- Qt5 → Qt6（`find_package` 和 `target_link_libraries`）
- C++ 标准 C++11 → C++14（`std::make_unique` 需要）
- `include_directories` 添加 `src`（修复 `#include "Core/CommonTypes.h"` 解析）

### Bug 修复

- **Core/Event.h** — 修复 `notify()` 中 `pair.second` → `pair.callback`，`disconnect()` 中 `pair.first` → `pair.id`（`CallbackPair` 使用命名成员而非 `std::pair`）
- **main.cpp** — 修复 `NOMINMAX` 重复定义警告

### 涉及文件

| 文件 | 变更类型 |
|------|---------|
| `CMakeLists.txt` | 重写 |
| `src/Core/Event.h` | Bug 修复 |
| `src/ViewModel/CardViewModel.h` | 移除 `card()` |
| `src/ViewModel/CardViewModel.cpp` | 移除 `card()` 实现 |
| `src/ViewModel/PlayerViewModel.h` | 移除原始指针，修复事件类型 |
| `src/ViewModel/PlayerViewModel.cpp` | 事件转发使用值类型 |
| `src/ViewModel/GameViewModel.h` | 新增 `PendingActionVM`，接口值类型化 |
| `src/ViewModel/GameViewModel.cpp` | 实现值类型 API，翻译层逻辑 |
| `src/ViewModel/ActionViewModel.h` | 参数/返回值 ID 化 |
| `src/ViewModel/ActionViewModel.cpp` | ID 实现 + 内部查找 |
| `src/View/GameBoardWidget.h` | 状态成员 ID 化 |
| `src/View/GameBoardWidget.cpp` | 所有交互使用 ID |
| `src/View/HandCardAreaWidget.h` | 信号 ID 化 |
| `src/View/HandCardAreaWidget.cpp` | 信号发射 ID 化 |
| `src/View/ActionPanelWidget.h` | 参数类型更新 |
| `src/View/ActionPanelWidget.cpp` | 实现更新 |
| `src/View/PlayerInfoWidget.cpp` | 事件回调更新 |
| `src/main.cpp` | 完全重写（值类型） |
