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
│   ├── Network/              # 网络层（局域网对战，开发中）
│   │   ├── Protocol.h/cpp    # 协议版本号 + MessageType + 消息结构体 + 手牌脱敏 redactCardList
│   │   ├── MessageSerializer.h/cpp # QDataStream 序列化 + 帧封装/解码
│   │   ├── GameServer.h/cpp  # QTcpServer：连接管理/握手/选将/广播转发（零 Model 依赖）
│   │   └── GameClient.h/cpp  # QTcpSocket："网络化 ViewModel"，零 Model 依赖、零规则判断
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
│       ├── SGSApp.h/cpp      # 本地模式（View + ViewModel 直连）
│       ├── ServerApp.h/cpp   # 网络服务器模式（headless；持有 GameServer，
│       │                     #   VM 信号 ↔ 客户端命令的双向接线见 connection.md §7）
│       └── ClientApp.h/cpp   # 网络客户端组合根（View + GameClient 直连，
│                             #   连接形状与 SGSApp 对称，见 connection.md §7.6）
└── tests/
    ├── smoke_test.cpp          # 无框架 Model 冒烟测试
    ├── model_test.cpp          # Model Qt Test
    ├── viewmodel_test.cpp      # ViewModel Qt Test
    ├── view_test.cpp           # View/App Qt Widgets Test
    └── network_test.cpp        # Network Qt Test（序列化/帧解码/ServerApp headless/GameServer 握手/命令分发/
                                 #   GameClient 网络化 ViewModel/ClientApp 组合根，真实 QWidget 点击驱动全链路）
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

产物：`SanguoshaQt.exe`、`libSanguoshaModel.a`、`libSanguoshaNetwork.a`、`ModelSmokeTest.exe`、`ModelTest.exe`、`ViewModelTest.exe`、`ViewTest.exe`、`NetworkTest.exe`

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

当前注册 5 个测试：`ModelSmokeTest`、`ModelTest`、`ViewModelTest`、`ViewTest` 和 `NetworkTest`。`ViewTest` 和 `NetworkTest` 通过 CTest 自动使用 `QT_QPA_PLATFORM=offscreen`，不需要桌面显示服务器（`NetworkTest` 自 Step 7 起会创建真实 `GameBoardWidget`，因此也需要 offscreen 平台插件）。

直接运行单个 Qt Test 时，可以使用对应的构建产物：

```powershell
.\build\ModelTest.exe
.\build\ViewModelTest.exe
$env:QT_QPA_PLATFORM = "offscreen"; .\build\ViewTest.exe
$env:QT_QPA_PLATFORM = "offscreen"; .\build\NetworkTest.exe
```

当前完整套件通过，5 个测试目标均为正常断言通过；覆盖卡牌规则、响应权限、濒死救援、目标选择、主要 QWidget、App 生命周期、网络协议序列化/帧解码（半包/粘包）、ServerApp headless 启动路径（无 QApplication 环境下完整回合循环）、ServerApp↔GameServer 的双向接线与三轮对抗性审查加固（VM 广播顺序、7 条命令分发、跨连接身份伪造防护、越权推进阶段防护、出牌/弃牌回合归属校验、待定动作重入保护）、Step 5 手牌脱敏（对手侧牌面字段占位、cardId 结构信息仍保留、己方视角不受影响）、Step 6 GameClient（对接真实 GameServer/ServerApp：playerId 分配与超员拒绝、7 个发送方法 payload 正确性、gameStarted/phaseChanged/playerDataUpdated/logMessage/脱敏后 handCardsUpdated 转发、双客户端出杀→待定响应往返、断线信号）、Step 7 ClientApp（真实 `GameBoardWidget` + `GameClient` 组合根，从真实 `QTest::mouseClick` 点击手牌到网络命令再到服务器结算的完整链路，验证 `GameBoardWidget` 零改动），以及 Step 8 心跳保活（无响应客户端在短超时参数下被判定失联并踢出、正常客户端跨多个心跳周期靠自动 `Pong` 保持连接）。NetworkTest 共 60 个用例。
