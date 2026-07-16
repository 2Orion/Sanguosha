# 三国杀（Sanguosha）

一个基于 MVVM + Qt 信号槽架构的双人三国杀简化实现，支持本地同屏和局域网对战。

---

## 架构

```
┌──────────────────────────────────────────────────────────────┐
│  App 层 — SGSApp / ServerApp / ClientApp（组合根）             │
│  创建对象、建立本地/网络信号槽连接、管理生命周期             │
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

View 层 **零编译依赖**于 ViewModel 和 Model，只包含 Common 层头文件。本地模式由
`SGSApp::startLocalGame()` 直连 View 与 ViewModel；网络模式由 `ServerApp` 运行权威
ViewModel/Model，`ClientApp` 直连 `GameBoardWidget` 与 `GameClient`。

---

## 当前实现

- 牌堆共 101 张：58 张基本牌、26 张普通锦囊、6 张延时锦囊、11 张装备牌。
- 可选武将共 9 名：曹操、关羽、张飞、赵云、孙权、周瑜、吕布、大乔、司马懿。
- 本地模式与局域网模式共用同一套服务器规则；网络模式已实现握手、选将、手牌脱敏、心跳和完整对局链路。
- 【决斗】已实现双方交替出【杀】，并可在目标放弃【无懈可击】后继续进入决斗响应链。
- 【闪电】、【乐不思蜀】和【兵粮寸断】已进入判定区并在判定阶段结算；【无懈可击】已接入六种单目标锦囊，但反无懈连锁尚未实现。
- 孙权【制衡】已支持选择任意张手牌、每回合限一次，并接通本地/网络命令链；吕布【无双】已要求连续打出两张【闪】。大乔【流离】仍只有规则查询接口。
- 【借刀杀人】和【五谷丰登】当前为简化结算。
- 装备牌已支持按槽位装备、替换和界面展示，诸葛连弩的无限出杀限制已接入；其余武器/防具的专属触发效果多数仍是属性或规则接口。

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
│   ├── Network/              # 网络层（局域网对战，已接入主程序）
│   │   ├── Protocol.h/cpp    # 协议 v2 + MessageType + 消息结构体 + 手牌脱敏 redactCardList
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
│   │   ├── ActionPanelWidget.h/cpp
│   │   ├── NetworkConfigDialog.h/cpp
│   │   └── Theme.h           # 深色古风主题：颜色常量 + QSS 片段工具（仅 View 层使用，无逻辑）
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
  GameViewModel::judgmentPerformed(CardData, QString, bool)
    → GameBoardWidget::onJudgmentPerformed

View → ViewModel (信号直连 public slots)
  GameBoardWidget::playCardRequested(cardId, playerId)
    → ActionViewModel::onPlayCardRequested
      → 校验 + 目标选择 + playCard(…)
  GameBoardWidget::skillRequested(cardIds, playerId)
    → ActionViewModel::onSkillRequested
      → 身份/阶段/牌权/使用次数校验 + 制衡结算
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

运行/测试前需把 Qt DLL 和配套 MinGW 运行库加入 PATH（每个新终端执行一次）：

```powershell
$env:PATH = "D:\QT\6.11.1\mingw_64\bin;D:\QT\Tools\mingw1310_64\bin;$env:PATH"
```

> 本机 `C:\mingw64\bin` 中的 GCC 8 运行库与 Qt 6.11.1/MinGW 13.1 不兼容。若它在 PATH 中排在 Qt 之前，
> 测试程序会因旧 `libstdc++-6.dll` 缺少符号而以 `0xc0000139` 退出。

> 判断该用哪个版本：先跑 `g++ --version` 和 `make --version`，若报错或版本 < 11，用版本二；若系统只有 `mingw32-make` 没有 `make`，版本一的 `make -j` 也要相应换成 `mingw32-make -j`。

产物：`SanguoshaQt.exe`、`libSanguoshaModel.a`、`libSanguoshaNetwork.a`、`ModelSmokeTest.exe`、`ModelTest.exe`、`ViewModelTest.exe`、`ViewTest.exe`、`NetworkTest.exe`

---

## 运行

```powershell
cd build
.\SanguoshaQt.exe   # 终端运行需先将 Qt 与 MinGW 的 bin 目录加入 PATH
```

主界面可直接选择本地双人对战，或通过「创建房间」/「加入房间」使用默认端口 `9527` 进行局域网对战。

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

当前完整套件包含 5 个测试目标；`NetworkTest -functions` 当前列出 63 个测试函数。覆盖基本卡牌规则、
【决斗】交替响应与无懈恢复、吕布双闪、延时锦囊判定、孙权制衡、AOE/濒死链、响应与回合权限、
QWidget/App 组装、协议 v2 序列化/帧解码、身份伪造防护、手牌脱敏、技能与判定网络消息、
`GameClient`/`ClientApp` 往返、心跳保活和进程内端到端对局。
