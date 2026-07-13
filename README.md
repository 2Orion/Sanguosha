# 三国杀（Sanguosha）

一个基于 MVVM + Qt 信号槽架构的双人本地对战三国杀简化实现。

详细设计文档见 [`plan.md`](plan.md)（架构与规则设计）和 [`interface.md`](interface.md)（Model 层接口契约）。

---

## 当前进度

- ✅ **Model 层**：已完整实现，继承 `QObject`，通过 Qt 信号发布状态变更
- ✅ **Model 层冒烟测试**：`tests/smoke_test.cpp`，覆盖牌堆管理、杀/闪/桃/酒、南蛮入侵/万箭齐发连锁响应、濒死求救链、胜负判定等核心流程，共 95 项检查
- ✅ **ViewModel 层**：已实现（`QObject`），包含 `GameViewModel`、`PlayerViewModel`、`CardViewModel`、`ActionViewModel`，完整的回合状态机和操作逻辑
- ✅ **View 层（Qt 图形界面）**：已实现，包含选将界面、游戏桌面（手牌区、玩家信息、操作按钮）、卡牌控件，支持完整的双人本地对战流程
- ✅ **组合根（Composition Root）**：`src/App/GameBootstrap` 统一创建并连接三层对象

---

## 架构

```
┌──────────────────────────────────────────────────────────┐
│  Composition Root (src/App/GameBootstrap)                 │
│  创建并连接 Model ↔ ViewModel ↔ View                      │
├──────────────────────────────────────────────────────────┤
│                                                           │
│  View 层 (Qt Widgets + 信号槽)                             │
│  界面绘制、用户交互 — 通过 Qt 信号/槽与 ViewModel 通信      │
│                                                           │
│  ViewModel 层 (QObject + 信号/槽)                          │
│  状态管理、View↔Model 桥梁 — 通过 Qt 信号发布事件           │
│                                                           │
│  Model 层 (QObject + 信号)                                 │
│  数据结构、游戏规则 — 通过 Qt 信号通知状态变化             │
│                                                           │
└──────────────────────────────────────────────────────────┘
```

### 通信机制

三层之间的所有事件通信统一使用 **Qt 信号槽**：

```cpp
// 信号/槽建立连接（在 View 内部或 Composition Root 中完成）
connect(viewModel, &GameViewModel::phaseChanged,
        board,     &GameBoardWidget::onPhaseChanged);

// View → ViewModel 通过 View 持有的 ViewModel 指针直接调用方法
void GameBoardWidget::onCardClicked(int cardId) {
    m_gvm->actionVM()->playCard(cardId, curPlayerId, targets);
}
```

`QObject` 的父子关系自动管理连接生命周期，无需手动 `disconnect`。

---

## 目录结构

```
Sanguosha/
├── CMakeLists.txt
├── plan.md                   # 架构与规则设计文档
├── interface.md              # Model 层接口契约文档
├── src/
│   ├── main.cpp              # 应用程序入口（QApplication + MainWindow）
│   ├── Common/               # 跨层共享类型
│   │   └── CommonTypes.h     # 公共枚举（卡牌/阶段/事件等类型）
│   ├── Model/                # QObject + Qt 信号
│   │   ├── GameState.h/cpp   # 游戏状态、待定动作（PendingActionInfo）
│   │   ├── Player.h/cpp      # 玩家对象（手牌、体力、武将）
│   │   ├── Character.h/cpp   # 武将基类及具体武将（曹操/关羽/张飞/赵云）
│   │   ├── Card.h/cpp        # 卡牌基类及具体卡牌
│   │   ├── CardManager.h/cpp # 牌堆管理（洗牌、摸牌、弃牌）
│   │   └── GameRule.h/cpp    # 规则引擎（杀闪判定、伤害结算、技能触发）
│   ├── ViewModel/            # QObject + Qt 信号/槽
│   │   ├── CardViewModel.h/cpp    # 卡牌 VM（展示状态：选中/可打出/高亮）
│   │   ├── PlayerViewModel.h/cpp  # 玩家 VM（体力/手牌数/状态转发）
│   │   ├── ActionViewModel.h/cpp  # 操作 VM（出牌/响应/目标选择/弃牌）
│   │   └── GameViewModel.h/cpp    # 游戏 VM（回合管理、阶段调度、生命周期）
│   ├── View/                 # Qt Widgets 图形界面
│   │   ├── MainWindow.h/cpp      # 主窗口（选将界面 ↔ 游戏界面切换）
│   │   ├── GameBoardWidget.h/cpp # 游戏桌面总布局
│   │   ├── CardWidget.h/cpp      # 单张卡牌控件（手绘渲染 + 交互）
│   │   ├── PlayerInfoWidget.h/cpp    # 玩家信息面板（体力❤/技能/手牌数）
│   │   ├── HandCardAreaWidget.h/cpp  # 手牌区域（排列 + 多选）
│   │   └── ActionPanelWidget.h/cpp   # 操作按钮面板（阶段相关按钮）
│   └── App/                 # 组合根（Composition Root）
│       └── GameBootstrap.h/cpp   # 创建并连接 Model ↔ ViewModel ↔ View
└── tests/
    └── smoke_test.cpp       # Model 层冒烟测试（95 项检查）
```

---

## 已实现内容

| 类别 | 内容 |
|------|------|
| **武将** | 曹操（奸雄）、关羽（武圣）、张飞（咆哮）、赵云（龙胆） |
| **基本牌** | 杀、闪、桃、酒 |
| **锦囊牌** | 过河拆桥、顺手牵羊、无中生有、南蛮入侵、万箭齐发、桃园结义 |
| **核心规则** | 出牌/响应阶段判定、伤害结算、濒死求救（含多人依次响应链）、AOE 锦囊的多目标链式响应、胜负判定 |
| **图形界面** | 选将界面（4 武将单选 + 同武将禁止重复）、卡牌手绘渲染（花色/点数/状态覆盖/牌背）、玩家体力条（❤/🖤）、手牌重叠排列、阶段相关按钮、响应模式、弃牌模式、游戏结束弹窗 |

---

## 构建

需要 **CMake ≥ 3.16** 以及 **Qt 6**（或 Qt 5.15+，含 Widgets 模块）。

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"      # 或你平台上惯用的生成器
make -j
```

产物：
- `build/SanguoshaQt.exe` — Qt 图形界面版游戏
- `build/libSanguoshaModel.a` — Model 静态库
- `build/ModelSmokeTest.exe` — 冒烟测试（95 项检查）

---

## 运行游戏

```bash
cd build
./SanguoshaQt.exe
```

**操作流程**：
1. **选将界面**：玩家 1 和玩家 2 分别点击选择武将，点击「开始对战」（同名武将不可重复选）
2. **出牌阶段**：双击卡牌快速打出，或单击选中后等待目标选择
3. **响应阶段**：需要出闪/出杀/出桃时，高亮可用的手牌，点击响应或点击「跳过」
4. **弃牌阶段**：点击要弃置的牌（选中），点击「确认弃牌」
5. **结束**：游戏结束弹出胜利信息，可返回选将重新开局

> 双人同屏模式，双方手牌均正面可见且可点击。

---

## 运行测试

```bash
cd build
./ModelSmokeTest.exe     # 直接运行
# 或
ctest --output-on-failure
```

全部通过时输出形如：

```
总检查项: 95，失败: 0
```
