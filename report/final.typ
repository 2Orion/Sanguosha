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

== 徐常喻

=== 智能体配置细节

本项目在开发过程中借助了 AI 智能体（Claude Code）辅助编码和调试。智能体运行在 Windows 11 环境下，通过项目根目录的 `CLAUDE.md` 文件获取项目上下文——该文档记录了架构说明（MVVM 分层约束、信号/槽通信机制）、构建命令（CMake 配置和工具链注意事项）以及项目开发中反复出现的历史 Bug 模式。智能体通过内置工具（Read/Edit/Write/Glob/Grep/Bash/PowerShell）进行文件操作和代码生成。

=== 规则文档与技能文档

项目维护的 `CLAUDE.md` 规则文档是智能体协作的核心参考。其中*架构约束*明确了 View 对 ViewModel/Model 零编译依赖，网络层 `GameClient` 不持有 Model 指针；*已知坑模式*记录了大量高频 Bug，如 `HandCardAreaWidget::clearWidgets()` 必须用 `deleteLater()`、Qt 属性变化信号在新旧值相同时不触发等。开发过程中智能体持续参考这些规则来保证代码风格一致、避免已记录的历史 Bug，例如编写网络端到端测试时自动检索了关于测试手牌构造非确定性的记录，使用了显式 `addHandCard` 而非依赖随机发牌。

=== 团队成员与智能体之间的分工协作机制

我使用智能体的方式是"小步迭代+每步验证"：先明确需要实现的功能（如"实现手牌脱敏，对手只能看到张数看不到牌面"），智能体生成代码框架后进行审查，确认符合项目现有架构和风格后再集成。每次只让智能体完成一个独立的小功能，随即编译运行对应测试，通过后再进入下一步。以网络层 Step 4（ServerApp 接线 GameServer）为例，该轮次拆分为协议消息分发 → 身份校验 → 对抗性审查 → 测试补充四个子步骤，每个子步骤都经历了"需求描述→代码生成→编译测试→审查确认"的循环。

=== 每轮人机对话的关键记录

在智能体辅助下完成网络层的典型开发循环如下：

1. *需求描述*：向智能体描述需要实现的功能
2. *探索理解*：智能体读取相关文件理解现有架构，参考 `CLAUDE.md` 中的已知坑记录
3. *方案生成*：智能体提出实现方案并生成代码，遵守架构约束
4. *人工审查*：审查生成的代码，提出修改意见后调整
5. *测试验证*：编写测试用例并编译运行，失败则分析日志、修改代码后重新验证
6. *文档同步*：实现通过后同步更新 `connection.md`、`CHANGELOG.md` 等文档

网络层在智能体辅助下经历了约 9 个主要轮次（Step 1-9）：协议定义 → 序列化 → 服务器连接管理 → 命令分发与安全加固 → 手牌脱敏 → 客户端网络化 ViewModel → 客户端组合根 → 心跳保活 → 端到端对局。

=== 实际应用效果及当前存在的问题

*成效：* 网络层开发效率显著提升，Step 1-9 在约2天内完成；测试 flaky 排查辅助——网络 e2e 测试曾出现约 50% 概率的间歇性失败，智能体通过系统化分析找到了根因（测试手牌构造的非确定性导致 `onModelPendingActionCreated` 的自动跳过分支触发），而非最初以为的 TCP 资源紧张；每次代码变更后智能体同步更新项目文档。

*当前存在的问题：* AI 生成的代码有时偏离项目既有的设计模式，需人工仔细审查——如网络层初期方案曾建议引入 `RemoteGameVM`/`GameController` 适配层，经人工判断后否定了这一设计；多文件修改时偶尔出现头文件与实现不一致；智能体对 Qt 元类型系统和 `QSignalSpy` 的限制理解不够深入，生成的测试代码有时需要手动调整。

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

- *徐常喻*：我在项目中主要负责网络层（局域网双人对战协议栈）的完整实现、全模块自动化测试的编写与维护，以及部分UI美化和debug修复工作。

网络层从零搭建：设计了一套二进制协议（19种消息类型、长度前缀的帧格式），实现了GameServer/GameClient/ServerApp/ClientApp四个核心类，以及序列化和手牌脱敏的逻辑。同时给各模块写了自动化测试（NetworkTest约65个用例，加上Model/ViewModel/View的补充测试），也参与了UI美化（深色主题、卡牌重绘、手牌扇形排列）和一些bug修复。前后新建约15个源文件。

第一个是身份认证。本地模式下View发命令时playerId就是操作者自己，不存在问题。但网络模式TCP消息里的playerId客户端可以任意填写，直接信任的话玩家就能冒充对手操作。解决办法是所有命令分发时用TCP连接分配的playerId覆盖消息体里的字段，再加了currentPlayerId和pendingResponderId两道校验。做了三轮对抗性审查（越权命令、重复选将、非法characterId）确认没有遗漏。

第二个是View不能改。网络模式要求GameBoardWidget一行代码不动，意味着GameClient的信号签名必须和GameViewModel/ActionViewModel完全对齐。做法是对着connection.md的信号映射表逐条建立ClientApp的连接，网络信号收到后直接emit不做任何本地计算。最后用QTest::mouseClick真实点击CardWidget验证整条链路通畅。

第三个是网络e2e测试不稳定。大约一半概率失败，一开始以为是TCP连接数太多导致资源竞争，后来系统排查发现根因是onModelPendingActionCreated在响应者没牌时会跳过pending、不广播信号，而测试依赖"一定会有pending广播"但随机发牌导致有时不发。改成显式addHandCard确定性地构造手牌，再补了cleanup冲刷deleteLater对象，之后连续12次全部通过。

之前对TCP协议和MVVM的理解停留在理论层面，做完这个项目才算真正知道这些概念在实际里怎么落地。比如协议设计要考虑半包粘包处理、QDataStream版本不一致会导致序列化失败、Qt自定义类型需要用qRegisterMetaType注册才能被QSignalSpy捕获。MVVM也是，以前理解是"View和Model分开"，现在知道边界选在哪比分开本身更关键——网络层放View和ViewModel之间最自然，因为View只认值类型信号，换本地还是网络对它没有区别。

技能方面主要提升了Qt网络编程（QTcpServer/QTcpSocket异步通信、连接状态机管理）、CMake组织多目标项目（静态库拆分、test目标链接）、Git多人协作（rebase保持历史清晰、对抗性code review）。测试方面学习了Qt Test在offscreen模式下测Widget、进程内模拟网络e2e的方法。

网络层高度依赖其他成员的工作。序列化直接复用孙一斐在Common层定义的CardData/PlayerData等结构体，所以他每次加字段我得同步更新序列化代码，刘耘希那边也要调整View渲染。View不改的目标要求跟刘耘希对齐信号槽签名。ServerApp调用刘耘希写的ViewModel public slot时，需要理解每个参数的语义和权限边界。

最深的体会是架构边界决定了网络层有多复杂。最开始想过做一套RemoteGameVM/GameController适配层让网络和本地共用接口，但这样每加一个本地功能就要同步改两个适配器，维护成本太高。最终决定直接复用现有信号槽、GameClient只转发不计算——GameBoardWidget真的一行没改就支持了网络。这件事让我确信"如果本地和网络需要不同逻辑，那就是边界选错了"。

测试方面也有值得反思的地方：测试的确定性比覆盖率重要得多。一个测试用例写得再全面，如果依赖随机状态，它通过的概率就只是个概率，不是保障。把网络e2e从约50%成功率提升到100%稳定，比多写20个测试用例更有实际价值。


= 课程改进建议

建议课程在智能体工具使用上提供更具体的引导。课程大纲中提到了智能体作为开发工具之一，但实际使用中我们发现，AI生成的代码经常偏离项目既有的设计模式，如果直接信任输出反而会引入问题。如果课程能补充一些智能体协作的最佳实践——比如如何编写有效的项目上下文文档、如何审查AI生成的代码、如何将任务拆解到适合AI处理的粒度——学生能更高效地利用这个工具，而不是自己摸索试错。