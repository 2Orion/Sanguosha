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
    ([孙一斐], [32401xxxxx], [计算机科学与技术]),
    ([刘耘希], [32401xxxxx], [计算机科学与技术]),
    ([徐常喻], [32401xxxxx], [计算机科学与技术]),
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

== 智能体配置细节

本项目全程使用 *Claude Code* 作为主要开发助手。底层模型主要通过 DeepSeek 提供的 Anthropic 兼容 API 接入，并按任务复杂度映射不同的推理模型：
- *代码生成、简单重构*：`deepseek-v4-flash`
- *复杂逻辑处理、高难度 Debug*：`deepseek-v4-pro`

本项目没有使用过`MCP Server`。

== 规则文档与技能文档

*文档工程*：项目根目录的 `CLAUDE.md` 记录了 MVVM 严格分层约束、构建命令、已知 Bug 模式等。智能体每次对话自动加载该文档，确保输出始终遵循项目既定架构。此外还编写了 `plan.md`（开发计划书）、`interface.md`（Model 层接口契约文档）、`connection.md`（层间信号映射表）等辅助文档，三人各自基于同一份接口文档生成代码，避免各层理解不一致导致的返工。

*演进记录*：`tobefixed.md`作为临时bug记录清单，记录需要修复的bug；`CHANGELOG.md` 记录了每次关键设计决策和修复记录。后续对话中智能体通过读取这些记录了解项目演进历史，避免重复踩坑。

*权限白名单*：`.claude/settings.local.json` 配置了智能体的命令执行权限，将编译（`cmake`、`make`）、测试（`ctest`）、运行等高频操作加入白名单，减少开发过程中的确认中断。

== 智能体内置工具与技能

Claude Code 提供了丰富的内置工具和技能来辅助开发：
- *代码搜索*：`Glob`（文件路径匹配）、`Grep`（代码内容搜索）快速定位代码
- *文件操作*：`Read`、`Write`、`Edit` 直接读写文件
- *命令执行*：`Bash` 执行编译、测试、运行命令
- *Git 集成*：直接在智能体对话中完成 `git add`、`git commit`、`git push` 等操作
- *技能系统*：`/archify`（架构分析）、`/code-review`（代码审查）、`/verify`（行为验证）、`/simplify`（代码简化）等内置技能，一键执行特定开发任务
- *计划模式*：`EnterPlanMode` 进入计划模式，探索代码库后设计方案，经用户确认后再实施

== 团队成员与智能体之间的分工协作机制

团队成员的智能体共享各种规则和限制文档，保证代码架构、风格、进度统一。每位成员在开发过程中都可以直接与自己的智能体对话，提出问题、请求代码生成或修改。智能体会根据项目文档和历史记录提供建议和代码实现，团队成员则负责审查、测试和整合这些代码。

== 实际应用效果及当前存在的问题
=== 实际应用效果

*标准化代码批量化产出*：Model 层的十个卡牌子类需要各自覆盖 `canUse`/`canTarget`/`getValidTargets`/`execute` 四个虚接口，结构相同但逻辑不同——手写每个子类需重复头文件声明、cpp 文件定义、构造函数链式调用等大量模板代码，智能体一次就能把这些类全部生成到位，开发者只需逐类检查编译和边界条件。同理 GameRule 中多个 AOE 响应函数（南蛮入侵和万箭齐发结构对称、区别仅为响应牌型）以及四个武将子类的技能框架也由智能体按基类接口快速完成。据估算，纯模板代码的编码量因此减少了 60% 以上。

*重构辅助*：在中期架构整改阶段（View 和 ViewModel 从直接耦合重构为纯信号/槽通信），智能体能够理解"View 不得 include ViewModel/Model 头文件"等约束，按开发者列的 todo list 逐条执行重构操作，并在每次重构后协助进行全流程回归测试，确保重构不引入新 Bug。

*多轮调试闭环*：测试中发现的逻辑缺陷（如装备替换时旧装备未进弃牌堆、回合外技能转化判断遗漏等）可以描述给智能体，由它分析根因、给出修改方案后执行修复。每轮修改后开发者手动跑一次游戏验证，形成"发现→分析→修复→验证"的快速闭环。

=== 当前存在的问题

尽管智能体大幅提升了编码效率，但在以下方面仍存在明显局限：

*非标准流程容易遗漏*：AOE 响应链需要依次让每个存活玩家响应杀/闪，中间若有人进入濒死状态，需暂停 AOE、处理濒死救援、再恢复后续目标。智能体生成的版本只在 `PendingActionInfo` 中保留了 `remainingTargets` 队列，没有保存完整的断点信息（当前 AOE 类型、发起者是谁），导致濒死救援完成后 AOE 链无法正确恢复。这个逻辑并不复杂，但对于"只看到当前函数"的智能体来说，很难意识到 AOE 响应和濒死救援是两个独立状态机、需要在数据层面做桥接。

*跨模块约束覆盖不全*：关羽武圣和赵云龙胆的技能转化需要同时接入响应路径（判断转化牌能否用于响应南蛮入侵等）和出牌路径（判断不可用的牌能否当杀主动打出）。智能体只接入了响应路径（入口集中），遗漏了出牌路径中 `getPlayableCards`、`canPlayCard`、`getValidTargetIds`、`playCard` 四个函数入口。这类"多入口、多路径"覆盖问题在整个开发过程中反复出现——只要需求描述中没有逐条列举受影响的所有代码路径，智能体就会遗漏。

*边界条件与 Qt 陷阱*：空指针保护、状态一致性判断、信号触发时机等点智能体经常遗漏。特别是 Qt 信号在旧值等于新值时不会触发，智能体生成的代码中多次出现"更新同一值后依赖信号刷新的逻辑无法执行"的问题，每次都需要人工审查补全。

*需求描述的精确度决定产出质量*：向智能体描述需求时留下的任何模糊地带，它都会在那个模糊地带里做出一个看起来合理但未必正确的实现。提示词越精确，智能体的产出偏差越小——但到达一定精细度后，编写提示词本身的工作量已接近手写代码。如何在这两者之间找到平衡点，是实践中最需要经验积累的环节。

== 人机对话的关键记录

*注：使用了/export导出的对话记录的pdf*
#figure(image("image/talk01.png", width: 80%), caption: "重构1")
#figure(image("image/talk02.png", width: 70%), caption: "重构2")
#figure(image("image/talk03.png", width: 70%), caption: "重构3")
#figure(image("image/智能体vm.jpg", width: 80%), caption: "智能体ViewModel")

= 学习回顾与个人总结

- *孙一斐*：

*学习回顾：*本次课程中我主要学到了一些现代C++项目工程的架构知识，其中最主要的就是MVVM架构，包括三个模块的职责和交互方式以及降低各个模块耦合度的方法等等。项目实现过程中，我的项目设计，代码实现等等能力都得到了很大提升，也学习到了很多工具的使用。比如说如何使用Qt来进行界面设计，如何使用Github Actions进行CI/CD流程的搭建等等。另外，我对git的使用也更加得心应手，更加熟练使用git进行多人协作开发，逐渐探索出了属于自己的项目开发workflow。团队协作上，我发现即使git以及github提供了非常方便的多人协作模式，团队成员之间的沟通交流依然很有必要。当一个团队成员发现一个新的bug，可以在群里提出。大家可以对此作出分析和评价，而准备修改的成员进行提前准备，防止做大量不必要的工作，大大提高团队的开发效率。

*个人总结：*

本次项目中，我主要负责前期的View、Common、App模块的实现、后期卡牌拓展的View、Common、App模块实现，游戏测试和部分bug修改、部分UI优化、CI/CD流程搭建等。

本项目遇到的最大困难就是前期的架构设计。由于前期对MVVM架构理解不是很深入（而且AI也不是很靠谱），导致前期的架构设计不够合理，View和ViewModel之间耦合过紧，后期不得不进行重构。我仔细研究了老师上课时样例代码的结构，对智能体提出了新的约束，让它进行了代码重构。重构后，View和ViewModel之间通过信号/槽通信，View完全不依赖ViewModel的类定义，同时在App层做信号、命令、数据绑定，实现了两部分的解耦。但解决这个问题时，App层的设计又出现了不合理的情况：App层在三大绑定和生命周期控制之外，参杂了许多应该属于Viewmodel层的设计。比方说，App层没有直接把View和Viewmodel用信号和槽机制来绑定，而是利用自身的信号和槽进行信号中转，增添了一些是否将信号转发到下一层的逻辑判断代码。但实际上，这个工作应该下放到Viewmodel层。后面也是做了重构，让代码符合MVVM架构的要求。

另外，另一大问题是三国杀逻辑设计的复杂性。三国杀的规则非常复杂，涉及到许多不同的卡牌和技能的交互，导致在实现过程中需要处理大量的边界情况和特殊规则。而纯靠智能体来设计实际上导致了很多逻辑问题。由于代码量很大，而且代码基本是由智能体编写的，加上我不是特别熟悉qt的类和用法，我基本没办法再去手动调整逻辑。最后的解决方案是把需要修改的逻辑分解成一个个atomic的问题，梳理好逻辑之后整理成文档，再交给智能体去修改，并且每次修改之后我都手动跑一遍程序来确保修改正确。这样智能体代码的正确率就大大提高了。

最后，大部分的问题其实都源自于智能体的代码质量不高，而且很难完全让智能体get到自己的意思（而且智能体确实容易产出shi山代码）。我想这一方面是由于搭载的模型还不够先进，另一方面也由于我对智能体的使用还比较青涩。智能体的使用是一个需要不断摸索和实践的过程，只有在不断的实践中才能更好地掌握智能体的使用方法和技巧。

本次项目中我确实学到了很多知识、工具、技能。在与智能体的写作中，我也在不断探索如何更好地使用智能体来辅助开发。我认为在模型越来越强大、智能体越来越智能的未来，智能体将会成为我们开发中不可或缺的工具，但这仍然不意味着我们可以完全依赖它。我们依然需要学习更多知识，了解更多技术，掌握更多技能，否则有时智能体产出了表面上能跑但实际上有问题的代码我们也无从得知。机师的操作水平，依然对AI的发挥有着至关重要的作用。

- *刘耘希*：

我在项目前期主要负责ViewModel层的实现和部分debug，后期负责Model和ViewModel层的卡牌扩展部分。

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

我在项目中主要负责网络层（局域网双人对战协议栈）的完整实现、全模块自动化测试的编写与维护，以及部分UI美化和debug修复工作。

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

建议课程在智能体工具使用上提供更具体的引导。课程大纲中提到了智能体作为开发工具之一，但实际使用中我们发现，AI生成的代码经常偏离项目既有的设计模式，如果直接信任输出反而会引入问题。如果课程能补充一些智能体协作的最佳实践——比如如何编写有效的项目上下文文档、如何审查AI生成的代码、如何将任务拆解到适合AI处理的粒度——学生能更高效地利用这个工具，代码实操效果会更好。