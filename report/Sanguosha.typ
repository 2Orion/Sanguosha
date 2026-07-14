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

在各个阶段开发中要保证严格的MVVM约束，同时让agent每次工作完检查自己的代码。

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
- 刘耘希：三国杀作为卡牌游戏在事务环节的设计上还是有比较大的工作量的，智能体开发常会漏掉某些关键环节或者是在某些环节出现死循环等错误，遇到这类问题的解决方法是AI写出的每一版自己进行体验这时候要测试极端情况。同时我发现更加精确的提示词可以让agent更好的完成工作，在之后的工作中，我会首先尝试自己写出更精确的提示词，然后再让agent进行工作，这样可以减少agent的工作量，同时也能提高工作的效率。
- 徐常喻：