#import "0.3.0/lib.typ": *
#show: chap-num

#let ymd = "2026-07-18"
#let course = "C++项目管理和工程实践"
#let teacher = [袁昕]
#let proj-name = [三国杀]


#zju-cover(
  course: course,
  teacher: teacher,
  student-count: 3,
  leader: [孙一斐],
  students: ([刘耘希, 徐常喻]),
  ymd: ymd,
)

#show: BL
#show: RP.with(
  course: course,
  proj-name: proj-name,
)

#exp-info-chart(
  teacher: teacher,
  course: course,
  student-count: 3,
  students: (
    ([孙一斐], [3240102099], [计算机科学与技术]),
    ([刘耘希], [3240106156], [计算机科学与技术]),
    ([徐常喻], [3240103369], [计算机科学与技术]),
  ),
  classmate-count: 0,
  ymd: ymd,
  exp-cate: [],
  exp-name: "三国杀",
  where:[],
)

#outline()

#pagebreak()

= 项目简介

本项目复刻了简化版的经典卡牌游戏《三国杀》，规则遵循标准三国杀规则。项目在实现基本规则约束之外，主要实现了以下卡牌：
- *4种基本牌*：杀、闪、酒、桃
- *8种武将牌*（部分技能做了调整）：赵云、关羽、张飞、曹操、孙权、吕布、周瑜、司马懿
- *12种锦囊牌*：万箭齐发、顺手牵羊、过河拆桥、南蛮入侵、桃园结义、无中生有、借刀杀人、决斗、无懈可击、乐不思蜀、兵粮寸断、闪电
- *6种装备牌*（部分效果做了调整）：诸葛连弩、青缸剑、寒冰剑、雌雄双股剑、青龙偃月刀、仁王盾

包含本地双人对战（主要用于测试）和局域网联机对战功能，局域网联机可以用IP:127.0.0.1进行测试。

== 项目架构

```text
Sanguosha/
├── CMakeLists.txt
├── src/
│   ├── main.cpp              # 应用程序入口
│   ├── Common/               # 值类型结构体（跨层共享）
│   │   ├── CommonTypes.h     # 枚举
│   │   ├── CardData.h        # 卡牌展示数据
│   │   ├── PlayerData.h      # 玩家展示数据
│   │   └── PendingActionData.h # 待定动作值类型
│   ├── Model/                # QObject + 信号
│   ├── Network/              # 网络层（局域网对战，已接入主程序）
│   │   ├── Protocol.h/cpp    # 协议 v3 + MessageType + 消息结构体 + 手牌脱敏
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
│   │   └── Theme.h           # 深色古风主题：颜色常量 + QSS 片段工具
│   └── App/                  # 组合根
│       ├── SGSApp.h/cpp      # 本地模式（View + ViewModel 直连）
│       ├── ServerApp.h/cpp   # 网络服务器模式
│       └── ClientApp.h/cpp   # 网络客户端组合根
└── tests/
    ├── smoke_test.cpp          # 无框架 Model 冒烟测试
    ├── model_test.cpp          # Model Qt Test
    ├── viewmodel_test.cpp      # ViewModel Qt Test
    ├── view_test.cpp           # View/App Qt Widgets Test
    └── network_test.cpp        # Network Qt Test
                                # GameClient 网络化 ViewModel/ClientApp 组合根
```

== 技术选型和工具使用

- 编程语言：C++17
- 图形界面：Qt 6.11（Qt6以上都支持）
- 构建方式：CMake + MinGW-w64
- 版本控制：Git + GitHub
- CI/CD：GitHub Actions

== 第二阶段分工
- *孙一斐*：负责卡牌拓展View, Common, App层的实现和CI/CD流程搭建，负责实际游戏测试以及武将图片的收集和处理。
- *刘耘希*：负责卡牌拓展Model，ViewModel层的实现和部分debug。
- *徐常喻*：负责网络层的实现和全模块测试文件编写、部分UI美化以及部分debug。

= 成果展示

== 游戏展示
=== 本地游戏展示
#figure(image("image/Game (2).png", width: 80%), caption: "选将界面")

双击`SanguoshaQt.exe`后进入选将界面，默认模式为双人本地对战。由于双方卡牌都可见，主要用作测试用途。双方武将选定后点击“开始游戏”进入游戏，共有8个武将可供选择。界面下方为联机对战选项，点击对应按钮后进入联机对战。

#figure(image("image/Game (3).png", width: 80%), caption: "对战界面")

进入游戏后，界面包含玩家信息区、手牌区、游戏日志区等，玩家可以通过点击手牌进行出牌操作，游戏日志区会显示游戏的进程和操作记录。

#figure(image("image/Game (4).png", width: 80%), caption: "对战界面2")

这里玩家一对玩家二出【杀】，玩家二手牌中有【闪】，进入响应环节，日志区显示响应日志。玩家二点击【闪】后，游戏日志区显示玩家二使用了【闪】，成功抵消了玩家一的【杀】。

#figure(image("image/Game (5).png", width: 80%), caption: "获胜展示")

这里玩家二血量归零，玩家一成功击败了玩家二，获得了游戏的胜利。

== 联机游戏展示

#figure(image("image/Game (6).png", width: 80%), caption: "创建房间")

联机游戏需要两个玩家处于相同的局域网下（校园网可能比较特殊，推荐使用热点），游戏默认运行在9527端口，玩家一点击“创建房间”后，游戏会在局域网内广播房间信息，玩家二点击“加入房间”后，输入玩家一的IP地址和端口号即可加入房间。

当然，也可以在同一台电脑上开两个进程来运行，IP地址默认为127.0.0.1。

#figure(image("image/Game (7).png", width: 80%), caption: "加入房间")

玩家二点击”加入房间“按钮后，输入玩家一的IP地址和端口号即可加入房间。

#figure(image("image/Game (8).png", width: 80%), caption: "玩家一（房主）对战界面")

这里是玩家一（房主）的对战界面，玩家一可以看到玩家二的手牌数量，但无法看到具体的手牌内容。

#figure(image("image/Game (1).png", width: 80%), caption: "玩家二对战界面")

同理，这里是玩家二的对战界面，玩家二可以看到玩家一的手牌数量，但无法看到具体的手牌内容。

*注：需要注意的是，可能出现界面大小和屏幕大小不匹配，导致界面显示不全的情况，解决办法是先取消全屏，然后拖动窗口到显示器上方，windows会自动调整窗口大小并设置成全屏。*

== 相关工具展示
=== Git/GitHub
#figure(image("image/git-history.png", width: 80%), caption: "部分Git提交历史")

本项目中我们主要使用git进行版本控制，使用GitHub进行代码托管和协作开发。通过上面的部分git提交历史可以看到我们在项目开发过程中每次提交的内容和时间。

=== GitHub Actions CI/CD流程
#figure(image("image/CICD.png", width: 80%), caption: "GitHub Actions CI/CD部分截图")

我们使用GitHub Actions搭建了CI/CD流程，相关脚本配置位于`.github/workflows`目录下，每次提交代码后都会自动编译和测试。测试文件位于`test`目录，涵盖了Model、ViewModel、View和Network模块的测试。通过CI/CD流程，我们可以及时发现代码中的问题，提高了开发效率和代码质量。
= 智能体使用说明
#figure(image("image/智能体配置.png", width: 80%), caption: "智能体配置")
#figure(image("image/智能体vm.jpg", width: 80%), caption: "智能体ViewModel")

= 学习回顾与个人总结

- *孙一斐*：

- *刘耘希*：我在项目前期主要负责ViewModel层的实现和部分debug，后期负责Model和ViewModel层的卡牌扩展部分。

*ViewModel层的工作概括：*

ViewModel层是本项目MVVM架构的中央枢纽，由`GameViewModel`和`ActionViewModel`两个QObject子类组成，承担四重核心职责：

1. *Model的拥有者*：通过`std::unique_ptr`持有`GameState`、`CardManager`等Model对象，全权管理游戏数据的生命周期，View完全不知晓Model的存在。
2. *数据翻译器*：将Model层的内部指针结构（如`PendingActionInfo*`）转换为View可安全消费的值类型结构体（`CardData`、`PlayerData`、`PendingActionData`），View层只include `Common/`头文件，零编译依赖Model和ViewModel。
3. *逻辑协调器*：管理游戏阶段推进（准备→判定→摸牌→出牌→弃牌→结束）、判定牌展示与定时器控制、自动跳过机制（乐不思蜀/兵粮寸断跳阶段、无响应牌时自动跳过pending action）、多目标选择状态机、武将技能转化（关羽红牌当杀、赵云闪当杀、孙权制衡）等全部流程控制。
4. *安全边界*：所有玩家操作（出牌、响应、弃牌、技能）必须经过ViewModel的权限校验——验证是否为当前回合玩家、是否有未决响应阻塞、目标是否合法。网络模式下使用连接层分配的可信`playerId`而忽略消息体内字段，防止作弊。

*核心理念：*

- *编译期隔离*：View只依赖`Common/`中的值类型，View与ViewModel之间通过Qt信号/槽通信，不直接在代码中引用对方的头文件。这使得View可以在本地模式（直连GameViewModel）和网络模式（直连GameClient）之间无差别切换。
- *显式组合根*：所有信号/槽的`connect()`集中在`SGSApp::startLocalGame()`中完成，依赖关系一目了然，避免了依赖注入框架的引入。
- *类型安全优先*：相比于经典MVVM中的`ICommand::exec(std::any)`模式，本项目采用Qt类型安全信号/槽——每个信号参数类型在编译期确定，避免了`std::any_cast`的运行时开销和类型错误风险。对于卡牌游戏这种操作类型可穷举、不需要undo/redo的场景，这是更务实的选择。
- *逻辑上移*：View层只负责”画什么”，ViewModel层决定”能做什么”——卡牌可玩性判断（`isPlayable`）、目标合法性、技能可用性等全部由ViewModel计算后通过值类型推送，View仅根据收到的数据渲染UI，不包含任何游戏逻辑。

在前期ViewModel实现中，由于对架构的理解不足，View曾直接依赖ViewModel类，导致耦合过紧。中期验收后我们重构为纯信号/槽通信，将View中的游戏逻辑全部上移至ViewModel，使各层职责清晰、可独立测试。在后期扩展中，我负责为Model层新增武将技能（借刀杀人、仁王盾等复杂交互）并在ViewModel层实现对应的仲裁逻辑和UI交互流程。整个过程中，我深刻体会到分层架构的核心不在于”用了什么模式”，而在于”每一层只做自己的事，对跨层的东西一无所知”——这是本课程要求的MVVM架构中最严格的准则。

- *徐常喻*：

= 课程改进建议