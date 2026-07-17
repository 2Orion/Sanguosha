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

- 牌堆共 **95** 张（基本 + 锦囊 + 保留装备）。牌堆中已暂时移除五谷丰登×2、丈八蛇矛、麒麟弓、八卦阵×2（类与逻辑保留，仅不入堆）。
- 可选武将 8 名展示：曹操、关羽、张飞、赵云、孙权、周瑜、吕布、司马懿。大乔（流离）在两人局无法转移，选将页 `hidden` 隐藏，charId=7 与 Model 仍保留。
- 本地模式与局域网模式共用同一套服务器规则；网络模式已实现握手、选将、手牌脱敏、心跳和完整对局链路。
- 【决斗】已实现双方交替出【杀】，并可在目标放弃【无懈可击】后继续进入决斗响应链。
- 【闪电】、【乐不思蜀】和【兵粮寸断】已进入判定区并在判定阶段结算；【无懈可击】已接入六种单目标锦囊，但反无懈连锁尚未实现。
- 孙权【制衡】：出牌阶段「发动技能」弃任意手牌摸等量，每回合一次。关羽【武圣】：出牌阶段直接点红牌当杀（转化路径，非技能按钮）。吕布【无双】需连续两张闪。大乔【流离】Model 分支仍在，选将暂隐。
- 【借刀杀人】两人局简化结算：目标须对使用者出【杀】，否则交出武器给使用者（响应路由已修正，早期版本曾误按南蛮语义结算）；五谷丰登逻辑保留但不入牌堆。
- 装备：按槽位装备/替换/界面展示；诸葛连弩无限出杀；**仁王盾挡黑色杀**、**青釭剑无视防具**已接入 `GameRule::executeKill`；武器攻击距离加成已计入 `Player::attackRange`。hover 看牌显示 `description`（如「黑色【杀】对你无效」）。
- 对局中间 log：结算类消息按队列逐条顺序播放（每条停留约 1.2 秒），播完后自动切回当前阶段提示；同一结算连发多条 log 不再互相覆盖，阶段切换/新响应/游戏结束时立即清空队列。

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

首次构建（或 `build/` 被删除重建）后，需将 Qt DLL 部署到 exe 同目录：

```powershell
D:\QT\6.11.1\mingw_64\bin\windeployqt.exe build\SanguoshaQt.exe
```

（仅需跑一次；后续增量编译无需重跑。）

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

### 终端运行（需先设 PATH）

```powershell
cd build
$env:PATH = "D:\QT\6.11.1\mingw_64\bin;D:\QT\Tools\mingw1310_64\bin;$env:PATH"
.\SanguoshaQt.exe
```

### 双击运行（免设 PATH，需先跑一次 windeployqt）

首次构建后执行 `D:\QT\6.11.1\mingw_64\bin\windeployqt.exe build\SanguoshaQt.exe`，把 Qt DLL 部署到 exe 同目录。之后即可直接双击 `build\SanguoshaQt.exe`，无需每次设 PATH。

主界面可直接选择本地双人对战，或通过「创建房间」/「加入房间」使用默认端口 `9527` 进行局域网对战。

---

## 测试

```powershell
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

当前注册 5 个测试：`ModelSmokeTest`、`ModelTest`、`ViewModelTest`、`ViewTest` 和 `NetworkTest`。`ViewTest` 和 `NetworkTest` 会创建真实 QWidget；Windows 使用 Qt 默认的原生平台插件，Linux 非 macOS 环境通过 CTest 自动使用 `QT_QPA_PLATFORM=offscreen`，以支持无桌面显示服务器的测试环境。

直接运行单个 Qt Test 时，可以使用对应的构建产物：

```powershell
.\build\ModelTest.exe
.\build\ViewModelTest.exe
.\build\ViewTest.exe
.\build\NetworkTest.exe
```

当前完整套件包含 5 个测试目标；`NetworkTest -functions` 当前列出 63 个测试函数。覆盖基本卡牌规则、
【决斗】交替响应与无懈恢复、吕布双闪、延时锦囊判定、孙权制衡、AOE/濒死链、响应与回合权限、
QWidget/App 组装、协议 v2 序列化/帧解码、身份伪造防护、手牌脱敏、技能与判定网络消息、
`GameClient`/`ClientApp` 往返、心跳保活和进程内端到端对局。
