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

Model/ViewModel 层不再依赖 Qt：原先的信号槽机制由 `Event.h` 中的 `EventListener<Args...>` 观察者模式替代。View 层通过 lambda 桥接 `EventListener` 与 Qt 信号槽：

```cpp
// EventListener → Qt 信号桥接
gameVM->phaseChanged.connect([this](PhaseType phase) {
    QMetaObject::invokeMethod(this, [this, phase]() {
        onPhaseChanged(phase);
    }, Qt::QueuedConnection);
});
```

## 目录结构

```
Sanguosha/
├── CMakeLists.txt
├── plan.md                   # 架构与规则设计文档
├── interface.md              # Model 层接口契约文档
├── src/
│   ├── main.cpp              # 控制台版游戏入口
│   ├── Core/                 # 跨层共享基础类型
│   │   ├── CommonTypes.h     # 公共枚举（卡牌/阶段/事件等类型）
│   │   ├── Event.h           # EventListener 观察者模式（替代 Qt 信号槽）
│   │   └── RandomUtils.h     # 随机数工具
│   ├── Model/
│   │   ├── GameState.h/cpp   # 游戏状态、待定动作（PendingActionInfo）
│   │   ├── Player.h/cpp      # 玩家对象（手牌、体力、武将）
│   │   ├── Character.h/cpp   # 武将基类及具体武将（曹操/关羽/张飞/赵云）
│   │   ├── Card.h/cpp        # 卡牌基类及具体卡牌
│   │   ├── CardManager.h/cpp # 牌堆管理（洗牌、摸牌、弃牌）
│   │   └── GameRule.h/cpp    # 规则引擎（杀闪判定、伤害结算、技能触发）
│   ├── ViewModel/
│   │   ├── CardViewModel.h/cpp    # 卡牌 VM（展示状态：选中/可打出/高亮）
│   │   ├── PlayerViewModel.h/cpp  # 玩家 VM（体力/手牌数/状态转发）
│   │   ├── ActionViewModel.h/cpp  # 操作 VM（出牌/响应/目标选择/弃牌）
│   │   └── GameViewModel.h/cpp    # 游戏 VM（回合管理、阶段调度、生命周期）
│   └── View/                     # Qt 图形界面
│       ├── main_qt.cpp           # Qt 版入口
│       ├── MainWindow.h/cpp      # 主窗口（选将界面 ↔ 游戏界面切换）
│       ├── GameBoardWidget.h/cpp # 游戏桌面总布局（事件桥接 + 全流程串联）
│       ├── CardWidget.h/cpp      # 单张卡牌控件（手绘渲染 + 交互）
│       ├── PlayerInfoWidget.h/cpp    # 玩家信息面板（体力❤/技能/手牌数）
│       ├── HandCardAreaWidget.h/cpp  # 手牌区域（排列 + 多选）
│       └── ActionPanelWidget.h/cpp   # 操作按钮面板（阶段相关按钮）
└── tests/
    └── smoke_test.cpp       # Model 层冒烟测试
```

## 已实现内容

- **武将**：曹操（奸雄）、关羽（武圣）、张飞（咆哮）、赵云（龙胆）
- **基本牌**：杀、闪、桃、酒
- **锦囊牌**：过河拆桥、顺手牵羊、无中生有、南蛮入侵、万箭齐发、桃园结义
- **核心规则**：出牌/响应阶段判定、伤害结算、濒死求救（含多人依次响应链）、AOE 锦囊的多目标链式响应、胜负判定
- **图形界面**：主窗口（QStackedWidget 页面切换）、卡牌手绘渲染（花色/点数/状态覆盖）、玩家体力条（❤/🖤）、手牌重叠排列、阶段相关按钮、Qt::QueuedConnection 桥接所有事件

## 构建

需要支持 C++17 的编译器和 CMake ≥ 3.16。

- **编译控制台版**不需要 Qt
- **编译图形界面版**需要安装 **Qt 6**（或 Qt 5.15+，包含 Widgets 模块）

### 无 Qt（仅控制台版 + 测试）

```bash
cmake -B build -S .
cmake --build build -j
```

### 有 Qt（含图形界面版）

需指定 Qt 安装路径：

```bash
cmake -B build -G "MinGW Makefiles" \
  -DCMAKE_PREFIX_PATH="D:/QT/6.11.1/mingw_64" \
  -DCMAKE_CXX_COMPILER="D:/QT/Tools/mingw1310_64/bin/g++.exe"
cmake --build build -j
```

> **注意**：必须使用 Qt 自带的 MinGW 编译器，否则版本不匹配。另外需将 `D:/QT/6.11.1/mingw_64/bin` 添加到系统环境变量 PATH 中，运行时才能找到 Qt DLL。

产物：
- `build/libSanguoshaModel.a`（Model 静态库）
- `build/ModelSmokeTest.exe`（冒烟测试，95 项检查）
- `build/Sanguosha.exe`（控制台版游戏）
- `build/SanguoshaQt.exe`（Qt 图形界面版游戏）

也可直接用 g++ 编译控制台版（无需 CMake、无需 Qt）：

```bash
g++ -std=c++17 -Isrc -Isrc/Core -Isrc/Model -Isrc/ViewModel \
  src/Model/*.cpp src/ViewModel/*.cpp src/main.cpp \
  -o Sanguosha.exe
```

## 运行游戏

### 图形界面版（推荐）

```bash
cd build
./SanguoshaQt.exe
```

**操作流程**：
1. **选将界面**：玩家 1 和玩家 2 分别点击选择武将，点击「开始对战」
2. **出牌阶段**：双击卡牌快速打出，或单击选中后等待目标选择
3. **响应阶段**：需要出闪/出杀/出桃时，高亮可用的手牌，点击响应或点击「跳过」
4. **弃牌阶段**：点击要弃置的牌（选中），点击「确认弃牌」
5. **结束**：游戏结束弹出胜利信息，可返回选将重新开局

> 双人同屏模式，双方手牌均正面可见且可点击。

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
