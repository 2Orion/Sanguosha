# 三国杀（Sanguosha）

一个基于 MVVM + Qt 信号槽架构的双人本地对战三国杀简化实现。

---

## 架构

```
┌──────────────────────────────────────────────────────────────┐
│  App 层 — SGSApp（纯组合根）                                  │
│  唯一知道 ViewModel 和 View 具体类型的模块                    │
│  只做：创建对象 + 建立信号槽直连 + 生命周期，零业务逻辑        │
├──────────────────────────────────────────────────────────────┤
│                                                               │
│  View 层 (Qt Widgets)             ViewModel 层 (QObject)      │
│  只依赖 Common                    依赖 Model + Common          │
│  不依赖 ViewModel                 对外暴露 Qt 信号 + 值类型    │
│  不依赖 Model                     含全部路由/拦截逻辑          │
│                                                               │
│  Common 层（值类型结构体，跨层传递）                             │
│  CardData / PlayerData / PendingActionData                     │
│                                                               │
└──────────────────────────────────────────────────────────────┘
```

View 层 **零编译依赖**于 ViewModel 和 Model，只包含 Common 层头文件。View 信号直连 ViewModel 的 public slots，ViewModel 信号直连 View 的槽，所有连接由 `SGSApp::startLocalGame()` 集中建立。

---

## 目录结构

```
Sanguosha/
├── CMakeLists.txt
├── src/
│   ├── main.cpp              # 应用程序入口
│   ├── Common/               # 值类型结构体（跨层共享）
│   │   ├── CommonTypes.h     # 枚举
│   │   ├── CardData.h        # 卡牌展示数据（含 CardList 别名）
│   │   ├── PlayerData.h      # 玩家展示数据
│   │   └── PendingActionData.h # 待定动作值类型
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
│   └── App/                  # 组合根
│       └── SGSApp.h/cpp
└── tests/
    ├── smoke_test.cpp          # 无框架 Model 冒烟测试
    ├── model_test.cpp          # Model Qt Test
    ├── viewmodel_test.cpp      # ViewModel Qt Test
    └── view_test.cpp           # View/App Qt Widgets Test
```

---

## 通信流向

```
ViewModel → View (信号直连)
  GameViewModel::handCardsUpdated(CardList)
    → GameBoardWidget::onHandCardsUpdated
  GameViewModel::playerDataUpdated(PlayerData)
    → GameBoardWidget::onPlayerDataUpdated

View → ViewModel (信号直连 public slots)
  GameBoardWidget::playCardRequested(cardId, playerId)
    → ActionViewModel::onPlayCardRequested
      → 校验 + 目标选择 + playCard(…)
```

详见 [`connection.md`](connection.md)。

---

## 构建

需要 **CMake ≥ 3.16** + **Qt 6（或 5.15+）**，编译器需与 Qt 版本匹配（Qt 6.11 要求 GCC 11+）。在 Windows 上必须使用 **Windows 原生终端**（PowerShell / Git Bash / cmd），不能用 WSL——WSL 的 CMake 不认识 `MinGW Makefiles` 生成器，也无法运行 Windows 版编译器。

以下命令为 **PowerShell** 语法（续行符是反引号 `` ` ``，不是 `\`）。

### 版本一：系统 PATH 里的编译器版本已满足要求（GCC ≥ 11，且 `make`/`mingw32-make` 已在 PATH 中）

```powershell
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
make -j
```

### 版本二：系统 PATH 中默认编译器版本过低（如本机 `C:/mingw64` 是 GCC 8.1，编不过 Qt 6.11 头文件），显式指定 Qt 自带的 MinGW 工具链

```powershell
cmake -B build -G "MinGW Makefiles" `
  -DCMAKE_CXX_COMPILER=D:/QT/Tools/mingw1310_64/bin/g++.exe `
  -DCMAKE_MAKE_PROGRAM=D:/QT/Tools/mingw1310_64/bin/mingw32-make.exe
cmake --build build -j
```

运行/测试前需把 Qt DLL 加入 PATH（每个新终端执行一次，否则 exe 会因找不到 `Qt6Core.dll` 等而启动失败）：

```powershell
$env:PATH = "D:\QT\6.11.1\mingw_64\bin;D:\QT\Tools\mingw1310_64\bin;$env:PATH"
```

> 判断该用哪个版本：先跑 `g++ --version` 和 `make --version`，若报错或版本 < 11，用版本二；若系统只有 `mingw32-make` 没有 `make`，版本一的 `make -j` 也要相应换成 `mingw32-make -j`。

产物：`SanguoshaQt.exe`、`libSanguoshaModel.a`、`ModelSmokeTest.exe`、`ModelTest.exe`、`ViewModelTest.exe`、`ViewTest.exe`

---

## 运行

```powershell
cd build
.\SanguoshaQt.exe   # 终端运行需先将 Qt 与 MinGW 的 bin 目录加入 PATH
```

---

## 测试

```powershell
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

当前注册 4 个测试：`ModelSmokeTest`、`ModelTest`、`ViewModelTest` 和 `ViewTest`。`ViewTest` 通过 CTest 自动使用 `QT_QPA_PLATFORM=offscreen`，不需要桌面显示服务器。

直接运行单个 Qt Test 时，可以使用对应的构建产物：

```powershell
.\build\ModelTest.exe
.\build\ViewModelTest.exe
$env:QT_QPA_PLATFORM = "offscreen"; .\build\ViewTest.exe
```

当前完整套件通过，4 个测试目标均为正常断言通过；覆盖卡牌规则、响应权限、濒死救援、目标选择、主要 QWidget 和 App 生命周期。
