# 三国杀

- 实现语言：C++
- 包和工具：vcpkg, Ninja, Qt, git, Cmake等

---

## 架构总览：MVVM 架构

```
┌──────────────────────────────────────────┐
│  View 层 (Qt Widgets)                    │
│  界面绘制、用户交互、动画效果            │
│  → 人员C负责                               │
├──────────────────────────────────────────┤
│  ViewModel 层 (QObject派生)              │
│  状态管理、命令转发、View←→Model桥梁     │
│  → 人员B负责                               │
├──────────────────────────────────────────┤
│  Model 层 (纯C++类 / QObject派生)        │
│  数据结构、游戏规则、卡牌/武将数据       │
│  → 人员A负责                               │
└──────────────────────────────────────────┘
```

**数据流方向**：
- 用户操作 → View(信号) → ViewModel(槽/命令) → Model(修改数据)
- Model(状态变化信号) → ViewModel(属性更新) → View(刷新UI)

---

## 目录结构

```
Sanguosha/
├── CMakeLists.txt          # 顶层构建文件（A/B/C协作编写）
├── src/
│   ├── main.cpp            # 入口（C负责）
│   ├── Model/
│   │   ├── Card.h/cpp           # 卡牌基类及具体卡牌
│   │   ├── CardManager.h/cpp    # 牌堆管理（洗牌、摸牌、弃牌）
│   │   ├── Character.h/cpp      # 武将基类及具体武将技能
│   │   ├── Player.h/cpp         # 玩家对象（手牌、血量、武将）
│   │   └── GameRule.h/cpp       # 规则引擎（杀闪判定、伤害结算）
│   ├── ViewModel/
│   │   ├── CardViewModel.h/cpp      # 卡牌VM（暴露卡牌展示状态）
│   │   ├── PlayerViewModel.h/cpp    # 玩家VM（暴露血量/手牌数等）
│   │   ├── ActionViewModel.h/cpp    # 操作VM（出牌/响应/目标选择）
│   │   └── GameViewModel.h/cpp      # 游戏VM（回合管理、阶段调度）
│   └── View/
│       ├── MainWindow.h/cpp         # 主窗口（开始界面→游戏界面）
│       ├── GameBoardWidget.h/cpp    # 游戏面板（桌面布局）
│       ├── CardWidget.h/cpp         # 单张卡牌控件（显示+点击）
│       ├── PlayerInfoWidget.h/cpp   # 玩家信息面板（头像/血量/ID）
│       ├── HandCardAreaWidget.h/cpp # 手牌区域（排列+交互）
│       └── ActionPanelWidget.h/cpp  # 操作按钮面板
└── resources/
    └── (图片资源、样式表等)
```

---

## 初版需要实现的功能

- 包含卡牌：3-4个简单的武将（曹操、关羽、张飞、赵云）、基本牌（杀、闪、桃、酒）、简单锦囊牌（过河拆桥、顺手牵羊、无中生有、南蛮入侵、万箭齐发、桃园结义）
- 对战形式：双人本地对战

---

## 三国杀基本规则（双人版简化）

### 游戏流程
1. **选将阶段**：每人选择一名武将
2. **初始手牌**：每人摸4张牌
3. **回合制**：双方轮流执行自己的回合
4. **胜利条件**：对方体力降至0且无人能救时，己方获胜

### 回合内阶段（依次执行）
| 阶段 | 说明 |
|------|------|
| ① 准备阶段 | 部分武将技能在此触发（如"洛神"），简单版通常跳过 |
| ② 判定阶段 | 若有延时锦囊（如"乐不思蜀"），进行判定；简单版先不实现 |
| ③ 摸牌阶段 | 从牌堆摸2张牌 |
| ④ 出牌阶段 | 可使用任意张牌（一般限制出1次"杀"） |
| ⑤ 弃牌阶段 | 手牌超出当前体力值则弃至体力值 |
| ⑥ 结束阶段 | 部分武将技能在此触发，简单版通常跳过 |

### 基本牌规则
| 卡牌 | 效果 | 使用时机 |
|------|------|---------|
| **杀** | 对对方造成1点伤害 | 出牌阶段（每回合限1次） |
| **闪** | 抵消一次"杀"的效果 | 被"杀"指定目标时响应 |
| **桃** | 回复1点体力 | 出牌阶段（体力不满时）/ 濒死时 |
| **酒** | 使下一张"杀"伤害+1 / 濒死时回复1体力 | 出牌阶段（出杀前）/ 濒死时 |

### 锦囊牌规则（简化版）
| 卡牌 | 效果 |
|------|------|
| **过河拆桥** | 弃掉对方一张牌（手牌或装备） |
| **顺手牵羊** | 获得对方一张牌（手牌或装备），需距离限制（双人版忽略距离） |
| **无中生有** | 摸2张牌 |
| **南蛮入侵** | 对方需打出一张"杀"，否则掉1血（需群体判定，双人版简化） |
| **万箭齐发** | 对方需打出一张"闪"，否则掉1血 |
| **桃园结义** | 双方回复至满血 |

---

## 三人分工计划

### 人员A — Model 层（核心数据与规则）

**负责模块**：所有Model层类 + 接口定义

| 文件 | 职责 | 工作量预估 |
|------|------|-----------|
| `Card.h/cpp` | 卡牌基类 + 7种子类实现（杀/闪/桃/酒/过河拆桥/顺手牵羊/无中生有/南蛮入侵/万箭齐发/桃园结义） | ★★★ |
| `Character.h/cpp` | 武将基类 + 4个武将（曹操、关羽、张飞、赵云，各有1个技能） | ★★☆ |
| `Player.h/cpp` | 玩家数据（手牌容器、血量、武将、装备区、判定区） | ★★☆ |
| `CardManager.h/cpp` | 牌堆初始化、洗牌、摸牌、弃牌管理 | ★★☆ |
| `GameRule.h/cpp` | 规则引擎（能否出杀、伤害结算、濒死结算、锦囊效果实现） | ★★★ |

**交付物要求**：
- 所有Model类需定义清晰的**公开接口**（方法签名 + 信号），作为与ViewModel的契约
- 不依赖View层任何代码，可独立单元测试
- 用 `QObject` 作为基类并定义信号，方便ViewModel监听状态变化
- 定义枚举类型：`CardType`, `Suit`, `PhaseType`, `GameEvent`

**关键接口契约（需先定稿给B/C）**：
```cpp
// Player
class Player : public QObject {
    Q_OBJECT
signals:
    void hpChanged(int newHp);
    void maxHpChanged(int newMaxHp);
    void handCardAdded(Card* card);
    void handCardRemoved(Card* card);
    void dying(Player* self);  // 濒死状态
    void died(Player* self);   // 死亡
public:
    const Character* character() const;
    int hp() const;
    int maxHp() const;
    const QList<Card*>& handCards() const;
    void drawCard(Card* card);
    void discardCard(Card* card);
    void damage(int value);
    void heal(int value);
    bool isDying() const;
    bool isAlive() const;
};

// Card
class Card : public QObject {
    Q_OBJECT
public:
    CardType type() const;
    QString name() const;
    int id() const;
    virtual bool canUse(const GameState* state, Player* user) const;
    virtual bool canTarget(const GameState* state, Player* user, Player* target) const;
    virtual void execute(GameState* state, Player* user, Player* target);
    bool isBasic() const;
    bool isStrategy() const;
};

// GameRule : 纯规则函数
namespace GameRule {
    bool canPlayKill(const Player* player);  // 是否已出过杀
    void resolveKill(GameState* state, Player* from, Player* to);  // 杀结算
    void resolveDying(GameState* state, Player* player);  // 濒死结算
    // ...
};
```

---

### 人员B — ViewModel 层（流程控制与状态管理）

**负责模块**：所有ViewModel层类 + 连接Model与View的桥梁

| 文件 | 职责 | 工作量预估 |
|------|------|-----------|
| `GameViewModel.h/cpp` | 游戏生命周期管理：开始游戏→选将→回合调度→阶段切换→胜负判定 | ★★★★ |
| `PlayerViewModel.h/cpp` | 包装Player数据为View可绑定的属性（血量百分比、手牌数、当前提示等） | ★★☆ |
| `CardViewModel.h/cpp` | 包装Card为View可用的展示状态（是否可选中、是否高亮、位置等） | ★★☆ |
| `ActionViewModel.h/cpp` | 操作逻辑：出牌合法性检查、响应流程（出杀→出闪→结算）、目标选择管理 | ★★★ |

**核心职责**：
- 通过信号/槽与Model层通信，监听Model事件
- 通过Qt属性或信号向View层暴露状态
- 响应View层的用户操作（按钮点击、卡牌点击），调用Model层方法
- 管理回合流程的状态机（阶段切换、等待用户操作、超时等）

**关键接口契约（需定稿给C）**：
```cpp
// GameViewModel : 公开给View的接口
class GameViewModel : public QObject {
    Q_OBJECT
signals:
    void phaseChanged(PhaseType newPhase);     // View据此刷新提示
    void currentPlayerChanged(Player* player); // 轮到谁了
    void gameOver(Player* winner);             // 游戏结束
    void actionRequired(QString prompt);       // 需要某玩家响应
    void cardPlayableChanged(QList<Card*> playableCards);  // 当前可用的牌
public slots:
    void startGame(Character* char1, Character* char2);  // 开始游戏
    void playCard(Card* card, Player* target);  // 用户出牌
    void respondCard(Card* card);               // 用户响应（如出闪）
    void skipAction();                         // 跳过操作
    void selectTarget(Player* target);          // 选择目标
};
```

---

### 人员C — View 层（界面展示与交互）

**负责模块**：所有View层类 + 主入口 + 资源文件

| 文件 | 职责 | 工作量预估 |
|------|------|-----------|
| `MainWindow.h/cpp` | 主窗口框架：标题、大小、开始界面（选将）→ 游戏界面的切换 | ★★☆ |
| `GameBoardWidget.h/cpp` | 游戏桌面布局：双方手牌区、桌面中央、玩家信息区、操作区 | ★★★ |
| `CardWidget.h/cpp` | 单张卡牌控件：花色/点数显示、卡牌名、可点击状态、选中高亮 | ★★★ |
| `PlayerInfoWidget.h/cpp` | 玩家信息：姓名、体力（血条/桃心）、手牌张数 | ★★☆ |
| `HandCardAreaWidget.h/cpp` | 手牌排列区：扇形/线性排列、选中/悬停效果、出牌动画 | ★★★ |
| `ActionPanelWidget.h/cpp` | 操作按钮面板：出牌、响应（出闪/出杀/使用桃）、跳过、取消 | ★★☆ |

**核心职责**：
- 使用Qt Designer或手写布局完成界面
- 绑定ViewModel的信号来更新UI
- 将用户交互（点击、选择）转化为对ViewModel的调用
- 负责视觉呈现：卡牌样式、血条、提示文字、简单动画

**界面布局参考**：
```
┌──────────────────────────────────────────┐
│ [对手信息]  ❤❤❤❤  手牌×4               │
│ ┌────────────────────────────────────┐  │
│ │    对手手牌区（背面朝上）           │  │
│ └────────────────────────────────────┘  │
│                                          │
│        桌 面 区 域（出牌显示）           │
│                                          │
│ ┌────────────────────────────────────┐  │
│ │    己方手牌区（正面朝上）           │  │
│ └────────────────────────────────────┘  │
│ [提示信息]  操作按钮                  │
│ [己方信息]  ❤❤❤❤  手牌×4            │
└──────────────────────────────────────────┘
```

**启动方式**：`/run` 技能启动查看效果

---

## 协作流程与契约

### 阶段一：接口对齐（A主导，B/C参与）

1. A 先定义 Model 层所有公开接口（头文件），提交 PR
2. B 和 C  review 接口，确认可用
3. B 基于 Model 接口设计 ViewModel 接口
4. C 基于 ViewModel 接口设计 View 接口
5. 三方确认整体数据流

### 阶段二：并行开发

| 人员 | 开发内容 | 可独立开始条件 |
|------|---------|--------------|
| **A** | Model 层全部实现 | 接口定稿后立即开始 |
| **B** | ViewModel 层实现 | Model 头文件可用后开始（可先mock Model） |
| **C** | View 层 + MainWindow + 资源 | ViewModel 头文件可用后开始（可先mock ViewModel） |

### 阶段三：集成联调

1. A → B：Model 实现完成后，B 替换 mock 为真实 Model
2. B → C：ViewModel 实现完成后，C 替换 mock 为真实 ViewModel
3. 三方联调，修复接口适配问题
4. 测试完整游戏流程

### Git 分支策略

- `main` — 稳定版本
- `feat/model-xxx` — A 的功能分支
- `feat/viewmodel-xxx` — B 的功能分支
- `feat/view-xxx` — C 的功能分支
- `feat/integration` — 集成联调分支

### 接口定义文件（契约文档）

在 `docs/interfaces/` 中维护三方接口定义文档，供随时查阅。

---

## 风险与注意事项

1. **接口变更**：任何接口变更需通知另外两人，更新契约文档
2. **View层无业务逻辑**：C 不能把游戏规则写在 View 里，必须通过 ViewModel
3. **Model层无UI依赖**：A 不能 include 任何 Qt GUI 头文件（`QWidget`等）
4. **信号连接时机**：在 GameViewModel 中集中管理信号连接，避免混乱
5. **C++内存管理**：明确 Card/Character 对象的生命周期和所有权（建议用 `QObject` 父子关系管理）
