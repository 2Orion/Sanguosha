#import "0.3.0/lib.typ": *
#show: chap-num

#let ymd = "2026-07-13"
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

// 示例：
// = 一级标题
// == 二级标题
// #figure(image("path/to/image.png", width: 80%), caption: "图片说明")
// *加粗*

= 项目简介

本项目以智能体为主，完成了一个简化版的三国杀游戏，规则这里不再介绍，基本遵循三国杀的标准规则。目前已经实现的部分：
- 4种武将牌：赵云、关羽、张飞、曹操
- 4种基本牌：杀、闪、酒、桃
- 6种锦囊牌：万箭齐发、顺手牵羊、过河拆桥、南蛮入侵、桃园结义、无中生有
目前仅支持本地双人对战（虽然本地双人有点难绷，因为双方的牌都可见），后续计划实现局域网联机对战功能。

== 项目架构

```
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
│   │   ├── Card.h/cpp
│   │   ├── CardManager.h/cpp
│   │   ├── Character.h/cpp
│   │   ├── GameRule.h/cpp
│   │   ├── GameState.h/cpp
│   │   ├── Player.h/cpp
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
│   └── App/                  # 纯组合根
│       └── SGSApp.h/cpp
└── tests/
    └── smoke_test.cpp
```

== 智能体的使用

我们根据自己需要的功能、可能用到的依赖和技术栈、MVVM架构相关的信息，写了一个`plan.md`文档。之后使用Claude Code扩充了这个文档，生成了一个`plan.md`的完整版本，包含各部分需要实现的功能、实现步骤、具体分工等等。我们在这个计划的基础上根据我们的需求进行修改，让它最终符合我们的预期。

接下来根据`plan.md`，我们使用Claude Code生成了一个`interface.md`的接口文档。接口文档主要包含了Model层的一些主要接口，便于Viewmodel层调用。我们在这个接口文档的基础上根据我们的需求进行审核和修改，让它最终符合我们的预期。

之后，我们三人各自分工，使用智能体辅助完成了Model、Viewmodel、View层的设计与实现。

过程中，我们使用了`README.md`和`CHANGELOG.md`来记录项目的开发进度和变更情况。我们在每次提交代码时，都会更新这些文档，以便于团队成员了解项目的最新状态。



== 分工情况

- 孙一斐：负责View层的设计与实现以及App、Common模块，完成了游戏界面、玩家信息显示、牌堆显示等功能。
- 刘耘希：负责Viewmodel层的设计与实现，完成了游戏逻辑的处理、玩家操作的响应等功能。
- 徐常喻：负责Model层的设计与实现，完成了游戏数据结构的设计、牌的管理等功能。

= 成果展示
下面是游戏流程的截图展示：
#figure(image("image/屏幕截图 2026-07-14 124426.png", width: 90%), caption: "选将界面")

进入游戏后进入选将界面，玩家可以选择自己喜欢的武将进行游戏。选将界面展示了可选的武将卡牌，玩家可以点击选择自己想要的武将。双方都选定武将之后，点击开始游戏，游戏进入正式的对战阶段。

#figure(image("image/屏幕截图 2026-07-14 124449.png", width: 90%), caption: "游戏界面")

游戏分为玩家状态显示区、手牌区和游戏日志显示区。玩家状态显示区展示了当前玩家的血量、武将信息等，手牌区展示了当前玩家的手牌，游戏日志显示区记录了游戏的操作和事件。当前双方的手牌都是可见的，因为是本地双人对战，后续会改成不可见。

#figure(image("image/屏幕截图 2026-07-14 124557.png", width: 90%), caption: "游戏过程展示")

游戏右侧的“当前回合”的显示块表明了当前是哪个玩家的回合，玩家可以在自己的回合中进行出牌操作。游戏日志显示区会记录每个玩家的操作和事件，方便玩家了解游戏进程。

#figure(image("image/屏幕截图 2026-07-14 124815.png", width: 90%), caption: "游戏结束界面")

当一方体力归零时，游戏结束，游戏结束界面会显示获胜信息。玩家可以选择重新开始游戏，进入新的对战。

= 总体心得与个人感悟
- 孙一斐：
- 刘耘希：
- 徐常喻：我负责 Model 层的设计与实现，使用 Claude Code 智能体辅助编程。下面从配置、文档、使用方式和实际效果四个方面总结这次经历。

  智能体配置：我使用 Claude Code 作为开发助手，底层模型通过 DeepSeek 提供的 Anthropic 兼容 API 接入（将 `ANTHROPIC_BASE_URL` 指向 `api.deepseek.com/anthropic`，Sonnet 和 Haiku 映射到 deepseek-v4-flash，Opus 和 Fable 映射到 deepseek-v4-pro），推理强度设为 high。这个配置下智能体的代码生成速度和上下文理解都满足日常开发需求，但在处理复杂逻辑（如 AOE 响应链的濒死中断恢复）时，偶有生成不完整的情况，需要人工审查补全。

  规则文档与技能文档：为了让智能体理解项目背景和架构约束，我编写了两份关键文档。项目级 `CLAUDE.md` 记录了 MVVM 严格分层约束、构建命令、已知 Bug 模式（如 Qt 信号在新旧值相同时不触发、`delete` 与 `deleteLater()` 的坑），每次对话智能体自动加载这份文档，使得生成的代码始终遵循项目既定架构。此外，`interface.md` 作为 Model 层的接口契约文档，定义了各模块的公开接口签名和信号列表，三个人各自基于这份文档用智能体生成代码，避免了各层接口理解不一致导致的返工。

  使用方式：Model 层的开发采用分步生成策略。先从 `interface.md` 出发，让智能体按模块逐个生成实现——先完成 Card 体系和各卡牌子类，再生成 GameState 和 Player，然后是 CardManager 和 GameRule，最后是 Character 和武将子类。每生成一个模块，我编译通过并审查关键逻辑后再进入下一个。生成新代码时，我会把已有的同类实现（如已完成的 `KillCard`）贴在提示词中作为风格参考，让新代码与现有代码保持一致的命名和结构。同时，每轮对话中产生的关键设计决策和修复记录都会更新到 `CHANGELOG.md`，后续对话中智能体可以读取这些记录来了解项目演进历史，避免重复踩坑。

  实际效果与现存问题：智能体在生成标准化代码方面效率很高——十个卡牌子类的虚函数覆盖、GameRule 中多个对称的 AOE 响应函数，都能一次生成到位，大幅减少了手动编写重复代码的工作量。但存在几个明显局限。一是非标准流程容易遗漏：AOE 响应链的濒死中断与恢复逻辑（`PendingActionInfo` 的 `continuation*` 字段）智能体生成的版本缺少断点保存，需要人工补上。二是跨模块约束覆盖不全：技能转化需要同时接入响应和出牌两条路径，智能体只接了一条，导致关羽武圣和赵云龙胆的主动出牌转化在后续测试中才被队友发现。三是边界条件检查不严：空指针保护、状态一致性判断、信号触发时机这些点，智能体生成的代码经常遗漏，必须逐函数审查。总的来说，智能体是一个高效的代码生成工具，能够极大加速模板化代码的产出，但在架构设计和边界情况处理上仍需要人来把控。