# 三国杀（Sanguosha）

一个基于 MVVM + Qt 信号槽架构的双人本地对战三国杀简化实现。

---

## 架构

```
┌──────────────────────────────────────────────────────────────┐
│  App 层 — GameBootstrap（组合根 / 中介者）                   │
│  唯一知道 ViewModel 和 View 具体类型的模块                    │
│  连接 View 信号 ↔ ViewModel 方法 / ViewModel 信号 ↔ View 槽  │
├──────────────────────────────────────────────────────────────┤
│                                                               │
│  View 层 (Qt Widgets)             ViewModel 层 (QObject)      │
│  只依赖 Common + App              依赖 Model + Common          │
│  不依赖 ViewModel                 对外暴露 Qt 信号 + 值类型    │
│  不依赖 Model                                                  │
│                                                               │
│  Common 层（值类型结构体，跨层传递）                             │
│  CardDisplayData / PlayerDisplayData / PendingActionVM         │
│                                                               │
└──────────────────────────────────────────────────────────────┘
```

View 层 **零编译依赖**于 ViewModel 和 Model，只包含 Common 层头文件。所有通信通过 Qt 信号/槽经 GameBootstrap 中转。

---

## 目录结构

```
Sanguosha/
├── CMakeLists.txt
├── src/
│   ├── main.cpp              # 应用程序入口
│   ├── Common/               # 值类型结构体（跨层共享）
│   │   ├── CommonTypes.h     # 枚举
│   │   ├── CardDisplayData.h # 卡牌展示数据
│   │   ├── PlayerDisplayData.h # 玩家展示数据
│   │   └── PendingActionVM.h # 待定动作值类型
│   ├── Model/                # QObject + 信号
│   ├── ViewModel/            # QObject + 信号/槽
│   │   ├── ActionViewModel.h/cpp
│   │   └── GameViewModel.h/cpp
│   ├── View/                 # Qt Widgets
│   │   ├── MainWindow.h/cpp
│   │   ├── GameBoardWidget.h/cpp
│   │   ├── CardWidget.h/cpp
│   │   ├── PlayerInfoWidget.h/cpp
│   │   ├── HandCardAreaWidget.h/cpp
│   │   └── ActionPanelWidget.h/cpp
│   └── App/                  # 组合根 + 中介者
│       └── GameBootstrap.h/cpp
└── tests/
    └── smoke_test.cpp
```

---

## 通信流向

```
ViewModel → View (信号)
  GameViewModel::handCardsUpdated(CardDisplayList)
    → GameBoardWidget::onHandCardsUpdated
  GameViewModel::playerDataUpdated(PlayerDisplayData)
    → GameBoardWidget::onPlayerDataUpdated

View → ViewModel (信号 → GameBootstrap 中转)
  GameBoardWidget::playCardRequested(cardId, playerId)
    → GameBootstrap::onPlayCardRequested
      → ActionViewModel::playCard(…)
```

详见 [`connection.md`](connection.md)。

---

## 构建

需要 **CMake ≥ 3.16** + **Qt 6（或 5.15+）**。

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
make -j
```

产物：`SanguoshaQt.exe`、`libSanguoshaModel.a`、`ModelSmokeTest.exe`

---

## 运行

```bash
cd build
./SanguoshaQt.exe
```

---

## 测试

```bash
cd build
./ModelSmokeTest.exe     # 95/95
```
