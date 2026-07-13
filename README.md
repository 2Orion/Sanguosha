# 三国杀（Sanguosha）

一个基于 **严格 MVVM** 架构的双人本地对战三国杀简化实现。

- 架构设计：[`plan.md`](plan.md)
- Model 层接口契约：[`interface.md`](interface.md)
- 更新日志：[`CHANGELOG.md`](CHANGELOG.md)

## 当前进度

- ✅ **Model 层** — 完整实现，纯 C++ 标准库，零外部依赖
- ✅ **ViewModel 层** — 完整实现，严格值类型 API（View 不接触 Model 指针）
- ✅ **View 层（Qt GUI）** — 完整实现，选将界面 + 游戏桌面
- ✅ **控制台版** — 可交互的控制台版三国杀
- ✅ **冒烟测试** — 覆盖核心流程的 Model 层测试

## 架构

```
View (Qt Widgets)  →  ViewModel (值类型)  →  Model (纯 C++)  →  Core (共享类型)
```

| 层 | 职责 | 依赖方向 |
|----|------|---------|
| **Core** | 共享枚举、`EventListener` 观察者模式、随机工具 | 无依赖 |
| **Model** | 卡牌/武将数据、游戏规则引擎、牌堆管理 | → Core |
| **ViewModel** | 状态管理、回合调度、操作逻辑，**仅暴露值类型**（`int`、`std::string`） | → Model, Core |
| **View** | Qt 界面绘制、用户交互，**不直接接触 Model** | → ViewModel, Core |

跨层事件通过 `EventListener<Args...>` 观察者模式实现：

```cpp
player->hpChanged.connect([](int hp) {
    std::cout << "HP changed to: " << hp << std::endl;
});
```

## 目录结构

```
Sanguosha/
├── CMakeLists.txt
├── plan.md                         # 架构与规则设计
├── interface.md                    # Model 层接口契约
├── CHANGELOG.md                    # 更新日志
├── src/
│   ├── main.cpp                    # 控制台版入口
│   ├── Core/
│   │   ├── CommonTypes.h           # 共享枚举（CardType, PhaseType 等）
│   │   ├── Event.h                 # EventListener 观察者模式
│   │   └── RandomUtils.h           # 随机数工具
│   ├── Model/
│   │   ├── Card.h/cpp              # 卡牌基类及具体卡牌（杀/闪/桃/酒/锦囊）
│   │   ├── CardManager.h/cpp       # 牌堆管理（洗牌、摸牌、弃牌回收）
│   │   ├── Character.h/cpp         # 武将基类（曹操/关羽/张飞/赵云）
│   │   ├── GameRule.h/cpp          # 规则引擎（杀闪判定、伤害、AOE、濒死）
│   │   ├── GameState.h/cpp         # 游戏状态、PendingActionInfo
│   │   └── Player.h/cpp            # 玩家对象（手牌、体力、回合状态）
│   ├── ViewModel/
│   │   ├── GameViewModel.h/cpp     # 中央协调器（生命周期、阶段调度）
│   │   ├── PlayerViewModel.h/cpp   # 玩家 VM（体力/手牌数/状态事件）
│   │   ├── CardViewModel.h/cpp     # 卡牌 VM（展示属性 + UI 状态）
│   │   └── ActionViewModel.h/cpp   # 操作 VM（出牌/响应/目标/弃牌）
│   └── View/
│       ├── main_qt.cpp             # Qt GUI 版入口
│       ├── MainWindow.h/cpp        # 主窗口（选将 → 游戏切换）
│       ├── GameBoardWidget.h/cpp   # 游戏桌面总布局
│       ├── PlayerInfoWidget.h/cpp  # 玩家信息面板（体力/技能/手牌数）
│       ├── HandCardAreaWidget.h/cpp# 手牌区域（卡牌排列/选中）
│       ├── CardWidget.h/cpp        # 单张卡牌控件（绘制/悬停/选中）
│       └── ActionPanelWidget.h/cpp # 操作按钮面板（出牌/响应/弃牌）
└── tests/
    └── smoke_test.cpp              # Model 层冒烟测试
```

## 已实现内容

- **武将**：曹操（奸雄）、关羽（武圣）、张飞（咆哮）、赵云（龙胆）
- **基本牌**：杀、闪、桃、酒
- **锦囊牌**：过河拆桥、顺手牵羊、无中生有、南蛮入侵、万箭齐发、桃园结义
- **核心规则**：出牌/响应判定、伤害结算、酒杀、濒死求救链、AOE 多目标链式响应、胜负判定
- **GUI**：武将选择界面、双人对战桌面、卡牌绘制（牌面/牌背）、选中/高亮/悬停状态

## 构建

需要 Qt 6、CMake ≥ 3.16、支持 C++14 的编译器。

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=<Qt6安装路径>
make -j
```

产物：

| 可执行文件 | 说明 |
|-----------|------|
| `build/sgs_console.exe` | 控制台版游戏（仅需 `Qt6::Core`） |
| `build/sgs_qt.exe` | Qt GUI 版游戏 |
| `build/sgs_test.exe` | Model 层冒烟测试 |

## 运行

**Qt GUI 版**：
```bash
./build/sgs_qt.exe
```

**控制台版**：
```bash
./build/sgs_console.exe
```

交互方式：输入数字选择武将、选择手牌、选择目标，输入 `-1` 结束出牌阶段。

**冒烟测试**：
```bash
./build/sgs_test.exe
```
