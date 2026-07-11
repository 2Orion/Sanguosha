# 三国杀（Sanguosha）

一个基于 MVVM 架构的双人本地对战三国杀简化实现。详细设计文档见 [`plan.md`](plan.md)（架构与规则设计）和 [`interface.md`](interface.md)（Model 层接口契约）。

## 当前进度

- ✅ **Model 层**：已完整实现，且已从 Qt 依赖改造为纯 C++17 标准库实现（不再需要 Qt/vcpkg 才能编译运行 Model 层）。
- ✅ **Model 层冒烟测试**：`tests/smoke_test.cpp`，覆盖牌堆管理、杀/闪/桃/酒、南蛮入侵/万箭齐发连锁响应、濒死求救链、胜负判定等核心流程，共 95 项检查。
- ✅ **ViewModel 层**：已实现（纯 C++17），包含 `GameViewModel`、`PlayerViewModel`、`CardViewModel`、`ActionViewModel`，完整的回合状态机和操作逻辑。
- ✅ **控制台版游戏**：`src/main.cpp` 提供了可交互的控制台版三国杀，用于端到端验证。
- ✅ **View 层（Qt 图形界面）**：已实现，包含选将界面、游戏桌面（手牌区、玩家信息、操作按钮）、卡牌控件，支持完整的双人本地对战流程。

## 架构

```
View 层 (Qt Widgets)      界面绘制、用户交互（Qt 6/5）
ViewModel 层 (纯 C++17)   状态管理、View↔Model 桥梁
Model 层 (纯 C++17)       数据结构、游戏规则、卡牌/武将数据
```

Model 层不再依赖 Qt：原先的信号槽机制由 `Event.h` 中的 `EventListener<Args...>` 观察者模式实现替代（用法与 Qt 的 `connect`/`emit` 类似，见下方示例）。

```cpp
player->hpChanged.connect([](int hp) {
    std::cout << "HP changed to: " << hp << std::endl;
});
player->damage(1); // 触发 hpChanged 事件
```

## 目录结构

```
Sanguosha/
├── CMakeLists.txt
├── plan.md              # 架构与规则设计文档
├── interface.md          # Model 层接口契约文档
├── src/
│   ├── main.cpp              # 控制台版游戏入口
│   ├── Model/
│   │   ├── CommonTypes.h     # 公共枚举（卡牌/阶段/事件等类型）
│   │   ├── Event.h           # EventListener 观察者模式（替代 Qt 信号槽）
│   │   ├── RandomUtils.h     # 随机数工具（替代 QRandomGenerator）
│   │   ├── GameState.h/cpp   # 游戏状态、待定动作（PendingActionInfo）
│   │   ├── Player.h/cpp      # 玩家对象（手牌、体力、武将）
│   │   ├── Character.h/cpp   # 武将基类及具体武将（曹操/关羽/张飞/赵云）
│   │   ├── Card.h/cpp        # 卡牌基类及具体卡牌
│   │   ├── CardManager.h/cpp # 牌堆管理（洗牌、摸牌、弃牌）
│   │   └── GameRule.h/cpp    # 规则引擎（杀闪判定、伤害结算、技能触发）
│   └── ViewModel/
│       ├── CardViewModel.h/cpp    # 卡牌 VM（展示状态：选中/可打出/高亮）
│       ├── PlayerViewModel.h/cpp  # 玩家 VM（体力/手牌数/状态转发）
│       ├── ActionViewModel.h/cpp  # 操作 VM（出牌/响应/目标选择/弃牌）
│       └── GameViewModel.h/cpp    # 游戏 VM（回合管理、阶段调度、生命周期）
│   └── View/                     # Qt 图形界面
│       ├── main_qt.cpp           # Qt 版入口
│       ├── MainWindow.h/cpp      # 主窗口（选将界面 ↔ 游戏界面）
│       ├── GameBoardWidget.h/cpp # 游戏桌面总布局
│       ├── CardWidget.h/cpp      # 单张卡牌控件（绘制 + 交互）
│       ├── PlayerInfoWidget.h/cpp    # 玩家信息面板
│       ├── HandCardAreaWidget.h/cpp  # 手牌区域
│       └── ActionPanelWidget.h/cpp   # 操作按钮面板
└── tests/
    └── smoke_test.cpp   # Model 层冒烟测试
```

## 已实现内容

- **武将**：曹操（奸雄）、关羽（武圣）、张飞（咆哮）、赵云（龙胆）
- **基本牌**：杀、闪、桃、酒
- **锦囊牌**：过河拆桥、顺手牵羊、无中生有、南蛮入侵、万箭齐发、桃园结义
- **核心规则**：出牌/响应阶段判定、伤害结算、濒死求救（含多人依次响应链）、AOE 锦囊的多目标链式响应、胜负判定

## 构建

需要支持 C++17 的编译器（已用 MinGW-W64 g++ 13.2.0 验证）和 CMake ≥ 3.16。

**编译图形界面版需要安装 Qt 6（或 Qt 5.15+）**，Model 层和 ViewModel 层以及控制台版则不需要。

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"      # 或你平台上惯用的生成器
make -j
```

产物：
- `build/libSanguoshaModel.a`（Model 静态库）
- `build/ModelSmokeTest.exe`（冒烟测试）
- `build/Sanguosha.exe`（控制台版游戏）
- `build/SanguoshaQt.exe`（Qt 图形界面版游戏）

也可直接用 g++ 编译控制台版（无需 CMake、无需 Qt）：

```bash
g++ -std=c++17 -Isrc -Isrc/Model -Isrc/ViewModel \
  src/Model/*.cpp src/ViewModel/*.cpp src/main.cpp \
  -o Sanguosha.exe
```

## 运行游戏

### 图形界面版（推荐）

```bash
cd build
./SanguoshaQt.exe
```

交互方式：选将界面点击选择武将 → 开始对战 → 游戏界面中双击/单击卡牌出牌，点击「结束出牌」或「确认弃牌」推进回合。双方同屏操作。

### 控制台版

```bash
cd build
./Sanguosha.exe
```

交互方式：输入数字选择武将、选择手牌、选择目标，输入 `-1` 结束出牌阶段。

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
