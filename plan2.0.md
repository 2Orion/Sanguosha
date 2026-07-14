# 三国杀 2.0 — 拓展计划

> 基于当前已完成的 MVVM 架构（Model + ViewModel + Qt View），对游戏内容、网络对战、界面体验和稳定性进行全面升级。

---

## 0. 当前状态

> 本节按当前代码的 MVVM 结构更新：View 层零 ViewModel 依赖，View 与 ViewModel 通过 Qt 信号/槽直连，连接由 `SGSApp::startLocalGame()` 集中建立。本文中的网络扩展将分别使用 `ServerApp` 和 `ClientApp` 作为新的组合根。

```
Common 层 (值类型，无逻辑)   ✅ 完整 — CommonTypes、CardData、PlayerData、PendingActionData
Model 层 (QObject + 信号)   ✅ 完整 — GameState、Player、Card 及子类、Character、CardManager、GameRule
ViewModel 层 (QObject)      ✅ 完整 — GameViewModel（阶段/状态推送）、ActionViewModel（出牌/响应/弃牌）
View 层 (Qt Widgets)        ✅ 完整 — MainWindow / GameBoardWidget / CardWidget / PlayerInfoWidget /
                                      HandCardAreaWidget / ActionPanelWidget，零编译依赖 ViewModel/Model
App 层 (组合根)              ✅ 完整 — SGSApp，唯一同时认识本地 View 和 ViewModel 具体类型的模块
自动化测试                  ✅ 4/4 通过（ModelSmokeTest、ModelTest、ViewModelTest、ViewTest；ViewTest 使用 offscreen）
```

当前测试已覆盖 Model、ViewModel、View 和 App 组合根；4/4 CTest 目标均为正常断言通过，濒死流程、响应权限、目标选择和 App 生命周期均有回归覆盖。

**没有** `PlayerViewModel`/`CardViewModel` 这类细粒度 VM，也没有独立的 `Core` 层或 `EventListener<T>` 观察者机制——ViewModel 直接用 Qt 信号 + Common 层值类型 DTO 对外通信。第 2 节的网络对战设计已按此更新（原稿基于 `EventListener`/`GameController` 抽象，与当前信号槽架构不符，见下）。

### 当前卡牌
- **基本牌**：杀×15、闪×10、桃×6、酒×3
- **锦囊牌**：过河拆桥×3、顺手牵羊×3、无中生有×3、南蛮入侵×2、万箭齐发×2、桃园结义×1
- **总计**：48 张

### 当前武将
曹操（奸雄）、关羽（武圣）、张飞（咆哮）、赵云（龙胆）

---

## 1. 卡牌拓展（Card Expansion）

### 1.1 装备牌系统

#### 新增基类：EquipmentCard

```cpp
// src/Model/Card.h 新增
enum class EquipSlot {
    Weapon,      // 武器
    Armor,       // 防具
    Mount,       // 马（进攻/防御）
    Treasure     // 宝物（预留）
};

class EquipmentCard : public Card {
public:
    EquipmentCard(CardType type, CardSuit suit, int number, EquipSlot slot);
    virtual ~EquipmentCard() = default;

    EquipSlot equipSlot() const;
    bool canUse(const GameState* state, const Player* user) const override;
    bool canTarget(const GameState* state, const Player* user,
                   const Player* target) const override;
    std::vector<Player*> getValidTargets(const GameState* state,
                                          const Player* user) const override;
    ActionResult execute(GameState* state, Player* user,
                          const std::vector<Player*>& targets) override;

    /// 装备效果查询
    virtual int attackRangeBonus() const;     // 攻击距离加成
    virtual int defenseBonus() const;         // 防御加成
    virtual bool canExtraKill() const;        // 是否可以额外出杀

protected:
    EquipSlot m_slot;
};
```

#### 新增装备牌

| 牌名 | 花色点数 | 类型 | 效果 |
|------|---------|------|------|
| 诸葛连弩 | ♠A | 武器 | 出牌阶段无限制出杀（`canExtraKill() = true`） |
| 青龙偃月刀 | ♠5 | 武器 | 攻击距离+2 |
| 丈八蛇矛 | ♠Q | 武器 | 攻击距离+3 |
| 麒麟弓 | ♥5 | 武器 | 攻击距离+5 |
| 青釭剑 | ♠6 | 武器 | 攻击距离+2，无视防具 |
| 寒冰剑 | ♠2 | 武器 | 攻击距离+2 |
| 雌雄双股剑 | ♠2 | 武器 | 攻击距离+2 |
| 八卦阵 | ♠2 | 防具 | 需要出闪时判定：红色则视为出闪 |
| 仁王盾 | ♠2 | 防具 | 黑色杀无效 |

**装备规则**：
- 每类装备槽每位玩家只能装备一件（替换旧装备）
- 装备牌在出牌阶段使用，装备到自己装备区
- 武器提供攻击距离（默认为 1）
- 防具提供特殊防御效果

#### Model 层修改

> `Player` 目前已有 `m_equipCards`（`std::vector<Card*>`，不区分槽位）及 `addEquipCard`/`removeEquipCard`/`equipCards()`/`hasEquipCards()`。这套接口是"无限叠加装备"的占位实现，需要改造为按 `EquipSlot` 索引，而不是新增一套平行接口。事件同样要用现有的 Qt 信号（`Player` 已有 `stateChanged()` 等信号），不要引入 `EventListener<T>`——本项目所有跨层通知都是 Qt 信号槽，没有自定义观察者模板。

```cpp
// Player.h 修改（替换现有 m_equipCards 为按槽存储）
class Player : public QObject {
    Q_OBJECT
public:
    // 装备管理
    void equipCard(EquipmentCard* card);              // 装备（替换同槽旧装备，旧装备进弃牌堆）
    void unequipSlot(EquipSlot slot);                 // 卸下
    EquipmentCard* equippedAt(EquipSlot slot) const;  // 查询
    int attackRange() const;                          // 计算攻击距离
    bool hasArmor() const;                            // 是否有防具

signals:
    void equipmentChanged(EquipSlot slot);             // 装备变化（新增信号）

private:
    EquipmentCard* m_equipSlots[4] = {nullptr};       // 替换原 m_equipCards
};
```

```cpp
// GameRule.h 新增
namespace GameRule {
    // 攻击距离计算（含武器 + 坐骑）
    int getAttackRange(const GameState* state, const Player* attacker);

    // 是否可被攻击到（距离判定）
    bool isInAttackRange(const GameState* state,
                         const Player* attacker, const Player* target);

    // 防具效果判定
    bool armorEffectCheck(const GameState* state,
                          const Player* defender, Card* attackCard);
}
```

#### ViewModel + View 层修改

- `PlayerData`（`src/Common/PlayerData.h`）新增装备区字段。**实际落地为按槽的两个 `CardData`：`weaponCard` / `armorCard`（`cardId == -1` 表示空槽），Mount 槽预留**——不是本节早稿设想的 `equipCards`（`QVector<CardData>`）。`GameViewModel::pushPlayerData` 里补充赋值
- `PlayerInfoWidget` 新增装备区显示（在体力下方或右侧），从 `onPlayerDataUpdated` 收到的 `PlayerData` 里读取
- `CardData`（`src/Common/CardData.h`）新增 `isEquipment` / `equipSlot` / `attackRange` 字段（**已落地**，比早稿多一个 `attackRange`：武器攻击距离加成），`GameViewModel` 在构造 `CardData` 时填充
- 装备牌的手绘渲染（`CardWidget::drawCardFront`）增加"装备"分类标签和特殊边框

### 1.2 锦囊牌补充

| 牌名 | 花色点数 | 效果 |
|------|---------|------|
| 决斗 | ♠A, ♦A, ♣A | 目标需打出杀，否则受到伤害 |
| 闪电 | ♠A | 判定区判定：黑桃2-9则受到3点雷电伤害 |
| 无懈可击 | ♠K, ♣Q, ♣K | 抵消一张锦囊牌对一名角色的效果 |
| 借刀杀人 | ♠Q, ♣K | 令装备武器的角色使用杀 |
| 五谷丰登 | ♥3, ♥4 | 展示牌堆顶至存活人数张牌，依次选择一张 |
| 乐不思蜀 | ♥6, ♥6, ♠6 | 判定区判定：非红桃则跳过出牌阶段 |
| 兵粮寸断 | ♠10, ♣10 | 判定区判定：非梅花则跳过摸牌阶段 |

**新增 GameRule 方法**：

```cpp
namespace GameRule {
    void executeDuel(GameState* state, Player* user, Player* target);
    void executeLightning(GameState* state, Player* user, Player* target);
    void executeNullify(GameState* state, Player* user);
    void executeBorrow(GameState* state, Player* user, Player* target);
    void executeHarvest(GameState* state, Player* user);
    void executeHappy(GameState* state, Player* user, Player* target);
    void executeFamine(GameState* state, Player* user, Player* target);

    // 无懈可击连锁处理
    bool checkNullifyChain(GameState* state, Card* targetCard,
                           Player* targetPlayer, Player* sourcePlayer);
}
```

### 1.3 CardManager 牌堆调整

```cpp
// 新牌堆构成（约 112 张）
// 基本牌：杀×30 闪×15 桃×8 酒×5
// 锦囊牌：过河拆桥×3 顺手牵羊×3 无中生有×3 南蛮入侵×3 万箭齐发×3
//         桃园结义×1 决斗×3 借刀杀人×2 五谷丰登×2 无懈可击×3
// 延时锦囊：乐不思蜀×3 兵粮寸断×2 闪电×1
// 装备牌：诸葛连弩×2 青龙偃月刀×1 丈八蛇矛×1 麒麟弓×1 青釭剑×1
//         寒冰剑×1 雌雄双股剑×1 八卦阵×2 仁王盾×1
```

### 1.4 武将拓展

| 武将 | 体力 | 技能 | 类型 |
|------|------|------|------|
| 孙权 | 4 | 制衡：出牌阶段可弃任意张牌并摸等量的牌 | 主动技能 |
| 周瑜 | 3 | 英姿：摸牌阶段多摸一张 | 被动技能 |
| 吕布 | 4 | 无双：杀需两张闪才能抵消 | 锁定技能 |
| 大乔 | 3 | 流离：杀可转移给其他玩家 | 主动技能 |
| 司马懿 | 3 | 反馈：受到伤害后可获得伤害来源一张牌 | 被动技能 |

**技能系统拓展**：

> `Character` 已经是 `QObject`，已有 `skillTriggered(const QString&)` 信号（见 `src/Model/Character.h`），新增技能提示直接复用它，不需要引入 `EventListener`。

```cpp
class Character : public QObject {
    Q_OBJECT
public:
    // 新增技能触发点
    virtual int onDrawPhaseBonus() const;          // 摸牌阶段额外摸牌数（周瑜）
    virtual bool requireExtraDodge() const;        // 是否需要两张闪（吕布）
    virtual bool canRedirectKill() const;          // 是否可以转移杀（大乔）
    virtual Player* getRedirectTarget(GameState* state, Player* self,
                                       Player* attacker) const;

    // skillTriggered 信号已存在，直接复用，无需新增
};
```

---

## 2. 局域网双人对战（核心拓展）

> 当前本地代码使用 `SGSApp` 作为组合根：View 与 ViewModel 通过 Qt 信号/槽直连，出牌、响应和阶段推进的校验分别位于 `ActionViewModel` 和 `GameViewModel` 的 public slots 中；自动跳过逻辑位于 ViewModel 内部。网络扩展沿用当前信号/槽形状，并新增两个组合根：服务器侧 `ServerApp`，客户端侧 `ClientApp`。Common 值类型使用 `CardData`、`PlayerData`、`PendingActionData` 和 `CardList`。
>
> - `ServerApp` 不创建 View，持有 `GameViewModel`、`ActionViewModel` 和 `GameServer`；VM 信号连接到 `GameServer` 的广播槽，`GameServer` 收到客户端消息后调用 VM 的 public slots。
> - `ClientApp` 持有 `GameBoardWidget` 和 `GameClient`；View 信号连接到 `GameClient` 的发送方法，`GameClient` 的同形状信号连接到 View 槽。
> - 当前 ViewModel 的路由槽已经是 public，无需修改现有 `SGSApp`；网络侧只需新增无 UI 的 `ServerApp` 和客户端 `ClientApp`。
> - 自动跳过逻辑已经在 ViewModel 发出信号前完成，服务器侧复用 ViewModel 时自动获得同样的行为。
>
> 当前信号/槽的权威映射见 [`connection.md`](connection.md)。
>
> 本节不再沿用原稿的 `EventListener<T>` 观察者模式和 `GameController` 虚接口设计。当前代码采用 Qt 信号/槽，View 不持有 ViewModel/Model 指针；网络层复用现有信号槽形状，通过 `ServerApp`/`ClientApp` 做组装，不给 View 层新增 Controller 抽象。

### 2.1 架构设计

#### 关键洞察：复用本地信号槽连接形状

当前本地对战的连接关系（见 `connection.md`）已经是：

```
GameViewModel/ActionViewModel  ──信号直连──►  GameBoardWidget（View）
GameBoardWidget（View）        ──信号直连──►  ViewModel public slots
                         连接由 SGSApp::startLocalGame() 建立
```

当前 ViewModel 的命令槽（`ActionViewModel::onPlayCardRequested`、`onRespondCardRequested`、`onTargetSelected`，以及 `GameViewModel` 的阶段/弃牌槽）和对外信号都使用纯值参数，天然可以映射成网络消息字段，不需要额外设计一套 VM 接口。具体信号形状以 `connection.md` 和当前头文件为准。

**因此网络边界不需要新增 `RemoteGameVM`/`GameController` 这类"仿制 ViewModel 接口"的适配层**，只需要：

- **服务器**：本地已有的 `GameViewModel`/`ActionViewModel`/`Model` 原样保留。新增 `GameServer` 和无 UI 的 `ServerApp`；`ServerApp` 把 VM 信号连接到 `GameServer` 的广播槽，`GameServer` 将客户端消息分发到 VM 的 public slots。
- **客户端**：`GameBoardWidget`（View）**完全不改**，因为它本就零依赖 ViewModel。新增 `GameClient` 和 `ClientApp`；`ClientApp` 把 View 信号连接到 `GameClient` 的发送方法，把 `GameClient` 的信号连接到 View 槽。`GameClient` 只负责网络收发，不在本地执行游戏规则。

```
┌───────── 服务器（Server）──────────┐          ┌───────── 客户端（Client）──────────┐
│                                    │          │                                     │
│  GameViewModel + ActionViewModel   │          │  GameBoardWidget（View，不改）      │
│  （本地权威，执行全部游戏逻辑）      │          │           ▲例信号槽形状复用          │
│           ▲│ 信号 / 槽调用          │          │           │                        │
│  ServerApp（服务器组合根）          │          │  ClientApp（客户端组合根）          │
│  · VM 信号  → 序列化 → 广播         │  TCP     │  · GameClient 收到消息 → 转发给 View 槽│
│  · 收到客户端消息 → 调用 VM public slot│◄───────►│  · View 信号 → GameClient 发送给服务器│
│           ▲                        │  协议     │           ▲                        │
│  GameServer（QTcpServer）           │          │  GameClient（QTcpSocket）           │
│  只做转发，不含游戏逻辑              │          │  只做转发，不含游戏逻辑，无 Model     │
└────────────────────────────────────┘          └─────────────────────────────────────┘
```

**核心决策**：网络边界仍在 View ↔ ViewModel 之间。服务器端由 `ServerApp` 把 GameServer 接到本地 VM；客户端由 `ClientApp` 把 GameClient 接到 `GameBoardWidget`。两端的 `GameBoardWidget` 代码保持一致，无需 `GameController`/`LocalController`/`RemoteController` 之类的运行时多态。

#### 为什么选择这个边界

| 边界 | 优点 | 缺点 |
|------|------|------|
| View ↔ VM 之间，复用现有信号槽形状（本方案） | 不新增 View 层抽象；协议字段与现有信号/槽参数一一对应，容易维护一致性 | 服务器/客户端各需要一个组合根，需要小心避免把两端的连接逻辑写重复 |
| VM → View 之间（原方案的另一选项） | 协议简单 | View 必须改造为 async，且需要新增 Controller 抽象 |
| 直接同步 Model | 逻辑简单 | 暴露 Model 指针，彻底破坏 MVVM |

### 2.2 网络协议设计

#### 协议层架构

```
src/Network/
├── Protocol.h              # 消息类型枚举 + 消息结构体（字段对齐现有信号/槽参数）
├── MessageSerializer.h/cpp # 序列化/反序列化（二进制）
├── GameServer.h/cpp        # 服务器端：QTcpServer + "网络化 View"逻辑
└── GameClient.h/cpp        # 客户端：QTcpSocket + "网络化 ViewModel"逻辑
```

不再需要 `RemoteGameVM.h/cpp`。客户端不需要一个独立文件去镜像 `GameViewModel` 的接口——`GameClient` 本身就直接暴露与 `GameViewModel`/`ActionViewModel` 相同形状的信号和方法（见 2.4），角色定位更清楚：GameServer 是"View 的网络化身"，GameClient 是"ViewModel 的网络化身"。

#### 消息类型定义

消息字段直接对应 `GameViewModel`/`ActionViewModel` 现有 public slots 和信号的参数列表（见对应头文件和 `connection.md` 的信号表），不再单独设计一套字段。

```cpp
// Network/Protocol.h
enum class MessageType : uint8_t {
    // 握手 / 选将
    Handshake,               // 客户端 → 服务器：请求连接
    HandshakeAck,             // 服务器 → 客户端：连接确认，含分配的 playerId
    SelectCharacter,          // 客户端 → 服务器：选择武将（对应 MainWindow::startGameRequested 的参数）
    GameStarted,              // 服务器 → 客户端：游戏开始，含双方角色信息

    // View 命令的网络化（ClientApp 收到 View 信号后打包发给服务器）
    PlayCardRequested,        // ~ ActionViewModel::onPlayCardRequested(cardId, playerId)
    RespondCardRequested,     // ~ ActionViewModel::onRespondCardRequested(cardId, responderId)
    DiscardCardRequested,     // ~ GameViewModel::onDiscardCardRequested(cardId, playerId)
    TargetSelected,           // ~ ActionViewModel::onTargetSelected(playerIndex)
    EndPlayRequested,         // ~ GameViewModel::onEndPlayRequested()
    SkipRequested,            // ~ GameViewModel::onSkipRequested()

    // GameViewModel 信号的网络化（ServerApp 收到 VM 信号后，
    // 不再调用本地 m_board 的槽，而是序列化广播给客户端）
    PhaseChanged,              // ~ GameViewModel::phaseChanged(PhaseType)
    PlayerDataUpdated,         // ~ GameViewModel::playerDataUpdated(int, PlayerData)
    HandCardsUpdated,          // ~ GameViewModel::handCardsUpdated(int, CardList)（对手手牌需脱敏，见下）
    PendingActionCreated,      // ~ GameViewModel::pendingActionCreated(PendingActionData)
    PendingActionCleared,      // ~ GameViewModel::pendingActionCleared()
    GameOver,                  // ~ GameViewModel::gameOver(int)
    LogMessage,                // ~ GameViewModel::logMessage(QString)

    // 保活
    Ping,
    Pong,
};
```

**手牌脱敏**：`HandCardsUpdated` 广播给两个客户端时不能发送同一份数据——`CardData` 含 `cardType`/`suit`/`number`/`cardName` 等牌面信息。服务器 `ServerApp`/`GameServer` 需要对每个客户端单独构造消息：发给牌的所有者时带完整 `CardList`；发给对手时只保留 `cardId`（用于后续弃牌/失去牌的动画匹配）和数量，牌面字段清空或替换为占位值。这一层脱敏必须在服务器广播边界完成，否则对手能看到手牌内容。

#### 序列化格式

沿用原方案的二进制序列化思路（值类型字段，Qt 类型如 `QString`/`QVector` 需要显式读写函数）：

```cpp
// Network/MessageSerializer.h
class MessageSerializer {
public:
    QByteArray serialize(const PlayCardMsg& msg);
    QByteArray serialize(const PlayerDataMsg& msg);
    // ... 每种消息一个重载

    template<typename T>
    T deserialize(const QByteArray& data);

private:
    void writeInt32(QByteArray& buf, qint32 val);
    void writeString(QByteArray& buf, const QString& str);
    void writeCardData(QByteArray& buf, const CardData& data);
    void writePlayerData(QByteArray& buf, const PlayerData& data);
    // ...
};
```

> 直接用 `QDataStream` 配合 `QByteArray` 会更省事（Qt 内置支持大多数值类型的 `<<`/`>>`），除非有明确的跨平台/跨语言互操作需求，否则不需要手写帧头校验和这类底层协议——这是原方案里唯一可以简化的部分。

**消息结构体示例**（字段对齐 ViewModel public slots 和 Common 层值类型）：

```cpp
struct PlayCardMsg {          // 客户端 → 服务器，对应 onPlayCardRequested(cardId, playerId)
    qint32 cardId;
    qint32 playerId;
};

struct TargetSelectedMsg {    // 客户端 → 服务器，对应 onTargetSelected(playerIndex)
    qint32 playerIndex;
};

struct PlayerDataMsg {        // 服务器 → 客户端，对应 playerDataUpdated(playerId, PlayerData)
    qint32 playerId;
    PlayerData data;          // 复用 Common 层结构体，不新造 PlayerState
};

struct HandCardsMsg {         // 服务器 → 客户端，对应 handCardsUpdated(playerId, CardList)
    qint32 playerId;
    CardList cards;           // 复用 Common 层结构体；发给非所有者时按上面脱敏规则裁剪字段
};

struct PendingActionMsg {     // 服务器 → 客户端，对应 pendingActionCreated(PendingActionData)
    PendingActionData info;   // 直接复用 Common 层结构体
};
```

**关键点**：消息 payload 尽量直接复用 `src/Common/` 里已有的 `CardData`/`PlayerData`/`PendingActionData`，而不是另建一套 `CardInfo`/`StateSyncMsg::PlayerState`。这样服务器端打包消息时可以直接拿 `GameViewModel` 信号槽的入参去序列化，只在按玩家脱敏时做必要的字段裁剪。

### 2.3 服务器设计（ServerApp + GameServer）— "网络化的 View"

```
GameServer (QTcpServer)
├── 监听端口（默认 9527）
├── 管理最多 2 个客户端连接（分配 playerId 0/1）
├── ServerApp 持有完整的 GameViewModel + ActionViewModel + Model（和本地模式一样，直接复用）
├── 扮演 GameBoardWidget 在信号槽图里的位置：
│   ├── 把 GameViewModel/ActionViewModel 的信号连接到自己的槽，序列化后按 playerId 定向广播
│   └── 收到客户端消息后，调用 ViewModel 的 public slots（onPlayCardRequested 等）
│       —— 这部分校验/路由逻辑不需要重写，服务器只是把"谁触发了这个槽"从 View 信号换成网络消息
└── 心跳定时器（保活 + 超时踢出）
```

具体做法：`ServerApp` 创建 `GameViewModel`、`ActionViewModel` 和 `GameServer`，但**不创建 `GameBoardWidget`**。它把 VM 的信号连接到 `GameServer` 的广播槽；`GameServer` 收到客户端消息后，直接调用 `ActionViewModel`/`GameViewModel` 的 public slots。现有本地 `SGSApp` 不需要改造，也不存在把私有槽改为 public 的步骤。

```cpp
// Network/GameServer.h
class GameServer : public QObject {
    Q_OBJECT
public:
    explicit GameServer(QObject* parent = nullptr);

    bool listen(quint16 port = 9527);
    void startGameWhenReady(int charId1, int charId2);  // 双方都连接后由房主触发

signals:
    void clientConnected(int playerId);
    void clientDisconnected(int playerId);

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();

    // 接收 ServerApp 连接的 VM 信号并广播（连接方式见下）
    void broadcastPhaseChanged(PhaseType phase);
    void broadcastPlayerData(int playerId, const PlayerData& data);
    void broadcastHandCards(int playerId, const CardList& cards);  // 内部按 playerId 脱敏后分别发
    void broadcastPendingAction(const PendingActionData& info);
    void broadcastGameOver(int winnerId);
    void broadcastLog(const QString& msg);

private:
    struct ClientSlot {
        QTcpSocket* socket = nullptr;
        int playerId = -1;
        QByteArray recvBuffer;
    };

    void dispatchClientMessage(ClientSlot& client, const QByteArray& payload);
    void sendTo(int playerId, MessageType type, const QByteArray& payload);
    void broadcastTo(MessageType type, const QByteArray& payload, int excludePlayerId = -1);

    QTcpServer* m_server;
    std::array<ClientSlot, 2> m_clients;
    GameViewModel* m_gameViewModel = nullptr;
    ActionViewModel* m_actionViewModel = nullptr;
};
```

**ServerApp 组装点**：

1. `ServerApp` 提供 `startHeadlessGame(charId1, charId2)`，创建并启动 `GameViewModel`，不创建窗口或牌桌。
2. 将 `GameViewModel`/`ActionViewModel` 的信号连接到 `GameServer` 的广播槽，将网络命令分发到对应的 ViewModel public slots。
3. 自动跳过逻辑继续使用 `GameViewModel::setNextPhase()` 和 `GameViewModel::onModelPendingActionCreated()`，不在网络层重复实现。

### 2.4 客户端设计（GameClient）— "网络化的 ViewModel"

```
GameClient (QObject)
├── 连接到服务器
├── 消息收发（socket 层）
├── 对外暴露与 GameViewModel + ActionViewModel 相同形状的信号 + 方法：
│   ├── 信号：phaseChanged / playerDataUpdated / handCardsUpdated /
│   │        pendingActionCreated / pendingActionCleared / gameOver / logMessage
│   │        —— 内容由收到的网络消息反序列化后 emit，不经过任何本地 Model 计算
│   └── 方法：playCard / respondCard / discardCard / skipResponse / endPlayPhase / advancePhase
│              —— 调用后只是把参数打包发给服务器，不在本地执行判定
└── 不包含 Model，不做任何规则判断（校验全部信任服务器结果）
```

客户端使用不含 Model 的 `ClientApp` 作为组合根：`GameBoardWidget` 的信号连接到 `GameClient` 的发送方法，`GameClient` 收到服务器广播后发出的信号直接连接到 `GameBoardWidget` 的槽。连接关系与本地 `SGSApp::startLocalGame()` 对齐，`GameBoardWidget` 本身不用感知运行模式差异。

```cpp
// Network/GameClient.h
class GameClient : public QObject {
    Q_OBJECT
public:
    explicit GameClient(QObject* parent = nullptr);

    void connectToServer(const QString& host, quint16 port = 9527);
    int localPlayerId() const { return m_localPlayerId; }

    // 与 ActionViewModel/GameViewModel 相同形状的方法，供 ClientApp 调用
    void playCard(int cardId, int playerId, const std::vector<int>& targetIds);
    void respondCard(int cardId, int responderId);
    void discardCard(int cardId, int playerId);
    void skipResponse();
    void endPlayPhase();

signals:
    void connected();
    void disconnected();
    void connectionError(const QString& error);
    void playerIdAssigned(int playerId);
    void gameStarted();

    // 与 GameViewModel 信号完全相同的形状，GameBoardWidget 槽可以直接连
    void phaseChanged(PhaseType phase);
    void playerDataUpdated(int playerId, const PlayerData& data);
    void handCardsUpdated(int playerId, const CardList& cards);
    void pendingActionCreated(const PendingActionData& info);
    void pendingActionCleared();
    void gameOver(int winnerId);
    void logMessage(const QString& msg);

private slots:
    void onSocketReadyRead();
    void onSocketDisconnected();

private:
    void dispatchServerMessage(MessageType type, const QByteArray& payload);

    QTcpSocket* m_socket;
    QByteArray m_recvBuffer;
    int m_localPlayerId = -1;
};
```

**不再需要** `RemoteGameVM`/`PlayerVMAdapter` 这两个类——原方案让它们去镜像 ViewModel 的完整只读查询接口，是因为原方案假设 View 会主动调用 VM 方法拉取状态。但当前 View（`GameBoardWidget`）从不主动查询 VM，全部状态都是通过槽被动接收（见 `connection.md` 的信号表），所以 `GameClient` 只需要转发信号即可，不需要维护一份完整的本地状态镜像和只读查询接口。

### 2.5 View 层适配

`GameBoardWidget`、`CardWidget`、`PlayerInfoWidget`、`HandCardAreaWidget`、`ActionPanelWidget` **完全不需要修改**——这是复用信号槽形状这个设计的核心收益。原方案里的 `GameController`/`LocalController`/`RemoteController` 三个类全部不需要，`GameBoardWidget` 也不需要接受构造参数上的差异（它本来就不持有 VM/Controller 指针，见 `src/View/GameBoardWidget.h`）。

需要修改/新增的只有 `App` 层和 `View/MainWindow`：

| 文件 | 修改 |
|------|------|
| `MainWindow.h/cpp` | 新增"创建房间"/"加入房间"入口，新增联网配置对话框 |
| 新增 `View/NetworkConfigDialog.h/cpp` | 联网配置对话框（IP/端口输入），纯 UI，不依赖 Network 层类型（通过信号把 host/port 传出去） |
| 新增 `App/ServerApp.h/cpp` | 组装无 UI 的 `GameViewModel`/`ActionViewModel` + `GameServer`，提供服务器侧启动路径 |
| 新增 `App/ClientApp.h/cpp` | 组装 `GameBoardWidget` + `GameClient`，按 2.4 描述的方式建立信号连接 |
| 新增 `Network/GameServer.h/cpp` | 服务器网络层 |
| 新增 `Network/GameClient.h/cpp` | 客户端网络层 |
| 新增 `Network/Protocol.h`、`MessageSerializer.h/cpp` | 消息定义与序列化 |

### 2.6 双人局域网流程

```
服务器端（房主）                          客户端（加入者）
─────────────────                        ─────────────────
1. 启动游戏，选择「创建房间」
   GameServer::listen(9527)

2. 看到等待界面                    →    3. 输入服务器 IP + 端口
                                        GameClient::connectToServer(...)

4. accept 连接，分配 playerId=0    ←    5. 收到 HandshakeAck(playerId=1)
   （若已有一个客户端则分配 =1）
   房主选将界面：选择武将               →    6. 对方选将界面：选择武将
   SelectCharacter(曹操)                    SelectCharacter(关羽)

7. 双方都 ready 后，服务器调用
   ServerApp::startHeadlessGame(0,1)
   （内部即 GameViewModel::startGame，逻辑不变）

8. GameViewModel 发出的每个信号
    经 ServerApp 连接 → GameServer
   序列化后广播（HandCardsUpdated 按 playerId 脱敏）  →  9. GameClient 收到，emit 同形状信号
                                                          → GameBoardWidget 槽接收，UI 更新（与本地模式路径一致）

10. P1 双击「杀」
    GameBoardWidget::playCardRequested → ClientApp
    → GameClient::playCard → 发送 PlayCardRequested        →

11. 服务器 GameServer 收到消息
    → 调用 ActionViewModel::onPlayCardRequested(cardId, playerId)
      （与本地模式完全同一份校验/路由代码）
    → ActionViewModel::playCard(...) 执行
    → GameViewModel 发出 pendingActionCreated
    → GameViewModel::onModelPendingActionCreated 在发信号前处理自动跳过
      （无牌可响应时不广播待定动作）
    → GameServer 广播 PendingActionCreated(Dodge, P2)      →  12. P2 客户端收到，显示响应界面

13.                                                    ←    14. P2 点击「跳过」
                                                              GameClient::skipResponse() → 发送 SkipRequested

15. 服务器收到，调用 GameViewModel::onSkipRequested
    → ActionViewModel::skipResponse(P2, true) → dealDamage
    → GameViewModel 发出 playerDataUpdated(P2, hp-1)
    → GameServer 广播                                   →  16. 双方收到状态更新

17. ...（后续流程与本地模式完全一致，只是每一步信号都多绕一次网络）
```

**这个流程和原方案（2.6 旧版）描述的步骤基本一致，区别在于：服务器直接复用 `GameViewModel`/`ActionViewModel` 的 public slots 和内部自动跳过逻辑，不在网络层重新实现游戏逻辑分发。**

### 2.7 可靠性设计

保活和重连的设计思路不变，只是宿主类型从原方案的 `GameServer`/`GameClient` 改为上面重新设计的版本，逻辑本身不受信号槽复用方案的影响：

```cpp
// GameServer 心跳保活
void GameServer::startHeartbeat()
{
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        broadcastTo(MessageType::Ping, {});
        for (auto& client : m_clients) {
            if (client.socket && client.socket->state() == QAbstractSocket::ConnectedState
                && client.lastRecvElapsed() > 10000) {
                client.socket->disconnectFromHost();
            }
        }
    });
    timer->start(3000);
}
```

```cpp
// GameClient 重连（预留，首个可用版本可以先不做，断线直接提示重新加入房间）
void GameClient::startReconnectTimer()
{
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        if (m_socket->state() != QAbstractSocket::ConnectedState) {
            m_socket->connectToHost(m_host, m_port);
        }
    });
    timer->start(5000);
}
```

重连要真正生效，服务器需要保留断线玩家的 `playerId` 一段时间并支持"重新绑定"，客户端重连成功后需要请求一次全量状态同步（例如新增 `MessageType::RequestFullSync`，服务器把当前 `GameState` 通过现有的 `pushAllData()`（`GameViewModel` 已有此方法，见 `src/ViewModel/GameViewModel.h:81`）重新推送一遍）——这部分原方案没有细化，留到阶段二实现时再落地。

---

## 3. View 模块显示效果优化

### 3.1 动画效果

```cpp
// CardWidget 新增动画
class CardWidget : public QFrame {
    // 入场动画（发牌时从牌堆飞出）
    void playDealAnimation(const QPoint& from, const QPoint& to,
                            int duration = 300);

    // 选中动画（弹性上弹）
    void playSelectAnimation();

    // 高亮脉冲（可打出时绿色呼吸光效）
    void startPulseAnimation();
    void stopPulseAnimation();

    // 禁用动画
    void playDisableAnimation();

    // 卡牌移动
    void playMoveToDiscardAnimation(const QPoint& discardPilePos);
};
```

使用 `QPropertyAnimation` 实现平滑过渡：

```cpp
void CardWidget::playDealAnimation(const QPoint& from, const QPoint& to, int duration) {
    auto* anim = new QPropertyAnimation(this, "pos");
    anim->setDuration(duration);
    anim->setStartValue(from);
    anim->setEndValue(to);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}
```

### 3.2 特效提示

```
// 伤害闪烁：目标玩家面板红色闪烁 3 次
void PlayerInfoWidget::playDamageEffect() {
    // 使用 QTimer 交替变换背景色
    // 红 → 白 → 红 → 白 → 恢复
}

// 濒死警告：面板红色脉冲 + 文字闪烁
void PlayerInfoWidget::playDyingEffect() {
    // 持续红色渐变脉冲，直到被救或死亡
}

// 桃园结义/五谷丰登等全局效果
void GameBoardWidget::playGlobalEffect(const QString& effectName) {
    // 中央显示大号特效文字（如"桃园结义"）
    // 渐进出现 → 停留 1s → 渐隐消失
}

// 技能发动提示
void GameBoardWidget::playSkillEffect(const QString& skillName) {
    // 在发动者面板上显示技能名动画
}
```

### 3.3 牌局背景美化

```cpp
// GameBoardWidget 设置背景
void GameBoardWidget::setupLayout() {
    // 背景色：深绿色牌桌 (#2E7D32)
    setStyleSheet("GameBoardWidget { background-color: #2E7D32; }");

    // 牌桌纹理（可选：使用 QPixmap 平铺）
    // QPixmap texture(":/images/table_texture.png");
    // QPalette pal;
    // pal.setBrush(backgroundRole(), QBrush(texture));
    // setPalette(pal);
}

// PlayerInfoWidget 圆角 + 阴影
void PlayerInfoWidget::setupUi() {
    // 阴影效果
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(12);
    shadow->setOffset(0, 2);
    shadow->setColor(QColor(0, 0, 0, 80));
    setGraphicsEffect(shadow);
}
```

### 3.4 布局优化

```cpp
// 手牌区域：扇形排列（替代纯水平重叠）
void HandCardAreaWidget::arrangeCards() {
    if (m_cardWidgets.size() <= 5) {
        // 少量手牌：扇形排列
        // 中心为轴，每张牌旋转 ±N 度
        int center = count / 2;
        for (int i = 0; i < count; ++i) {
            int angle = (i - center) * 3;  // 每张偏转 3°
            QTransform transform;
            transform.translate(x + CARD_WIDTH/2, y + CARD_HEIGHT);
            transform.rotate(angle);
            transform.translate(-CARD_WIDTH/2, -CARD_HEIGHT);
            m_cardWidgets[i]->setTransform(transform);
        }
    } else {
        // 大量手牌：水平重叠排列（当前方案）
    }
}

// 卡片宽度自适应
void CardWidget::resizeEvent(QResizeEvent* event) {
    // 根据窗口宽度调整卡牌大小
    // 最小 60x84，最大 100x140
    int baseWidth = parentWidget()->width() / 10;
    int cardW = std::clamp(baseWidth, 60, 100);
    int cardH = cardW * 1.4;
    setFixedSize(cardW, cardH);
}
```

### 3.5 日志与提示优化

```cpp
// GameBoardWidget 日志增强
class GameBoardWidget {
    // 使用 QScrollArea 显示滚动日志（替代单行 QLabel）
    QTextEdit* m_logArea;

    // 日志分级
    void appendLog(const QString& msg, LogLevel level = Normal) {
        switch (level) {
        case Normal:  // 黑色：常规操作
        case Damage:  // 红色：伤害
        case Heal:    // 绿色：回复
        case Skill:   // 金色：技能发动
        case System:  // 灰色：系统信息
        }
    }
};
```

---

## 4. Bug 修复与优化

> ⚠️ 原稿的 4.1（运行一段时间后无响应退出）和 4.3（有可响应牌时必须响应不可跳过）中的部分问题已经在实际代码里修复，且根因和原稿的推测不一样。原稿是问题发现前基于猜测写的排查清单，真正的根因、修复方式和涉及文件见 [`CHANGELOG.md`](CHANGELOG.md)。ViewModel 的响应校验、濒死链、目标选择和 App 生命周期已经加入自动化回归测试；网络非法命令仍需等网络层落地后补充网络回归测试。

> 当前回归测试入口为 `ctest --test-dir build --output-on-failure`，测试源文件分别位于 `tests/model_test.cpp`、`tests/viewmodel_test.cpp` 和 `tests/view_test.cpp`。当前 4 个测试目标全部通过，详见测试运行结果和 `CHANGELOG.md`。
>
> - **运行一段时间后无响应卡退**：根因不是 `EventListener`（本项目从未使用这个机制）或定时器生命周期，而是 `HandCardAreaWidget::clearWidgets()` 用 `delete w` 同步销毁了正在处理 `mousePressEvent` 调用栈上的 `CardWidget` 自身，导致 use-after-free。修复是把 `delete w` 换成 `w->deleteLater()`。
> - **有可响应牌时必须响应不可跳过 / 无响应牌时卡死**：根因是 `ActionPanelWidget::updateForPendingAction` 在 `canSkip=false` 时隐藏「跳过」按钮，且 `ActionViewModel::skipResponse` 在非强制模式下会因 `canSkip` 检查直接 return。当前修复是始终显示「跳过」按钮，由 `GameViewModel::onSkipRequested` 统一以 `forceNoCard=true` 调用，并由 `GameViewModel::onModelPendingActionCreated` 在无响应牌时自动跳过，详见 `src/ViewModel/GameViewModel.cpp`。
>
> 后续再遇到类似的"卡死"、"崩溃"问题，优先去读 `CHANGELOG.md` 里已记录的坑（信号在新旧值相同时不触发、`delete` vs `deleteLater()`、响应面板状态未恢复），大多数疑难杂症已经在这几类模式里出现过。

### 4.1 武将技能不能正常使用

> ⚠️ 原编号为 4.2，因上方两节被删除而重新编号。当前自动化测试已经覆盖曹操受伤触发、张飞杀次数规则、关羽/赵云的基础转化和响应路径；关羽/赵云主动转化出杀的完整端到端路径仍需补充专门断言，不能仅凭静态阅读或手动启动结论。

| 技能 | 当前问题 | 修复 |
|------|---------|------|
| 曹操 - 奸雄 | 受伤后不触发/不能正确摸牌 | 检查 `GameEvent::OnDamage` 触发链，确保 `dealDamage` 后正确检测事件 |
| 关羽 - 武圣 | 红牌当杀不能正常响应南蛮入侵 | 检查 `skillTransformCard` 在 `getResponseCards` 中的调用路径 |
| 张飞 - 咆哮 | 出牌阶段出杀次数限制未解除 | 检查 `canPlayKill` 中张飞特殊逻辑的执行 |
| 赵云 - 龙胆 | 杀↔闪转化不生效 | 检查 `skillTransformCard` 在响应路径中的调用 |

**修复方案**：

```cpp
// 统一技能触发点
void GameRule::dealDamage(GameState* state, Player* target, int value, Player* source) {
    // ... 伤害结算 ...
    // 触发所有玩家的 OnDamage 事件
    for (Player* p : state->allPlayers()) {
        if (p->character() && p->character()->triggerCondition(GameEvent::OnDamage, state, p)) {
            p->character()->triggerSkill(state, p);
        }
    }
    // 触发所有玩家的 OnDamageCaused 事件
    if (source && source->character()) {
        // ...
    }
}

// 统一响应牌收集（含技能转化）
std::vector<int> ActionViewModel::getResponseCardIds(int playerId, CardType requiredType) const {
    std::vector<int> result;
    Player* player = findPlayer(playerId);
    if (!player) return result;
    for (Card* card : player->handCards()) {
        if (card->cardType() == requiredType ||
            (requiredType == CardType::Peach && card->cardType() == CardType::Wine)) {
            result.push_back(card->id());
            continue;
        }
        // 武将技能转化
        if (player->character()) {
            CardType transformed = player->character()->skillTransformCard(card);
            if (transformed == requiredType) {
                result.push_back(card->id());
            }
        }
    }
    return result;
}
```

---

## 5. 实施路线图

> 三人分工（按内容顺序切分：人员职责、契约前置、时间线错位、联调里程碑、协调规则）见第 7 节。任务归属：阶段一 → 甲；阶段二 → 乙（其中 MainWindow 联网入口、NetworkConfigDialog 两项纯 View 任务划给丙）；阶段三 → 丙。

### 阶段一：基础拓展（约 2 周）
| 任务 | 文件 | 优先级 |
|------|------|--------|
| 装备牌系统（Card 子类 + Player 装备槽） | `Card.h/cpp`, `Player.h/cpp` | P0 |
| 新增锦囊牌（决斗、无懈、乐不思蜀等） | `Card.h/cpp`, `GameRule.h/cpp` | P0 |
| 牌堆调整（112 张） | `CardManager.cpp` | P0 |
| 武将拓展（孙权、周瑜、吕布等） | `Character.h/cpp` | P1 |
| View 装备区显示 | `PlayerInfoWidget`, `CardWidget` | P1 |
| 技能 Bug 修复 | `GameRule.cpp`, `Character.cpp` | P0 |

### 阶段二：局域网对战（约 4 周）

> 按 2.1-2.7 的重新设计更新，去掉了 `RemoteGameVM`/`GameController`/`LocalController`/`RemoteController` 四项（不再需要），`GameBoardWidget` 从"改造"变为"零改动"，新增 `ServerApp`/`ClientApp` 两个组合根和网络层对象。

| 任务 | 文件 | 优先级 |
|------|------|--------|
| Protocol.h 消息定义 | `Network/Protocol.h` | P0 |
| MessageSerializer（建议直接用 QDataStream） | `Network/MessageSerializer.h/cpp` | P0 |
| ServerApp 无 UI 启动路径 | `App/ServerApp.h/cpp` | P0 |
| GameServer（服务器侧"网络化 View"） | `Network/GameServer.h/cpp` | P0 |
| GameClient（客户端侧"网络化 ViewModel"） | `Network/GameClient.h/cpp` | P0 |
| 客户端信号翻译 | `App/ClientApp.h/cpp` | P0 |
| 手牌脱敏逻辑（对手手牌广播前裁剪牌面字段） | `Network/GameServer.cpp` | P0 |
| MainWindow 联网入口（创建/加入房间） | `View/MainWindow.h/cpp` | P1 |
| 网络配置对话框 | `View/NetworkConfigDialog.h/cpp` | P1 |
| 双人联调测试 | — | P0 |

### 阶段三：UI 美化 + 稳定性（约 2 周）
| 任务 | 优先级 |
|------|--------|
| 卡牌入场/移动动画 | P1 |
| 伤害/濒死/技能特效 | P1 |
| 牌桌背景 + 控件美化 | P2 |
| 日志区域增强 | P2 |
| 运行时稳定性和内存泄漏修复 | P0 |
| `QPointer` / `isDestroyed` 保护所有异步回调 | P0 |
| 分层自动化测试（Model/ViewModel/View/App） | ✅ 已完成（4/4 CTest 目标通过；无预期失败） |

---

## 6. 架构兼容性检查

### 6.1 当前 MVVM 边界是否被突破？

| 改动 | 是否突破边界 | 理由 |
|------|------------|------|
| EquipmentCard 新类 | ❌ 否 | Model 层纯数据，无 UI 依赖 |
| 装备槽在 Player 中 | ❌ 否 | 纯数据成员 + 信号（复用 Qt 信号，非新概念） |
| GameServer 持有完整 GameViewModel/Model | ❌ 否 | 服务器端本就该拥有完整 VM+Model，和本地单机模式一致 |
| GameClient 暴露 GameViewModel 同形状信号/方法 | ❌ 否 | 只是转发网络消息，不持有、不访问 Model 指针 |
| GameBoardWidget 零改动，同时被本地/服务器/客户端复用 | ❌ 否，且是收益 | View 本来就零依赖 ViewModel，天然可以被三种运行模式复用，不需要新增 Controller 抽象层 |
| ServerApp/ClientApp 组合根 | ❌ 否 | 只负责创建对象和建立连接，不向 View 暴露 Model 指针；现有 `SGSApp` 本地路径保持不变 |

原方案里 `GameController`/`LocalController`/`RemoteController`/`RemoteGameVM`/`PlayerVMAdapter` 五个类已从设计中移除（见 2.3-2.5），因为它们解决的问题（"让 View 用同一套接口访问本地/远程数据"）当前架构已经用"View 只认 Common 值类型 + 信号槽"的方式解决了，不需要再加一层运行时多态。

### 6.2 需要特别注意的边界

```
服务器：GameServer 转发 GameViewModel/ActionViewModel 的信号（含值类型脱敏）
  └── GameViewModel/Model 是唯一的游戏规则权威，不感知网络存在
客户端：GameClient 转发网络消息为信号，直接连到 GameBoardWidget 的槽
  └── GameClient 不持有、不构造任何 Model::Player*/Model::Card* 指针
  └── GameBoardWidget 全程只接触 Common 层值类型，不知道数据来自本地 VM 还是网络
```

**关键约束**：`GameClient` 和服务器侧的网络转发代码永远不能持有或传递 `Model::Player*`/`Model::Card*` 指针。所有跨网络的数据必须是 `src/Common/` 里已有的值类型（`CardData`/`PlayerData`/`PendingActionData`/`CardList`）或更基础的 `int`/`QString`/枚举——这和本地模式 View 对 Model 的约束完全一致，只是把"跨层"换成了"跨网络"。

---

## 7. 三人分工方案（按内容顺序切分）

> 本节按当前代码命名：本地模式使用 `SGSApp`，网络模式新增 `ServerApp`/`ClientApp` 两个组合根；ViewModel 路由槽已经是 public slots，`SGSApp` 不需要为网络模式改造。值类型统一使用 `CardData`/`PlayerData`/`PendingActionData`。
>
> 与第 5 节路线图配套。按文档内容章节切分：甲负责 §1+§4（卡牌与规则），乙负责 §2 全部（网络对战），丙负责 §3+阶段三（UI 与稳定性）。这个切法的特点是**乙的网络主线 + 甲丙两条支线定期汇入**——网络的服务器/客户端两端由一人完成，信号翻译逻辑高度对称，一人写不容易两边对不上；代价是工作量不均（§2 是 4 周，§1/§3 各 2 周）和两处串行依赖，必须用 7.2 的契约前置和 7.5 的时间线错位来消化。

### 7.1 总体划分

| 成员 | 内容 | 对应章节 | 涉及文件 |
|------|------|---------|---------|
| **甲：卡牌与规则** | 装备牌、锦囊、牌堆、武将、技能 Bug 验证 | §1、§4、阶段一 | `Model/*`、`Common/*`（加字段）、`PlayerInfoWidget`（装备区显示） |
| **乙：网络对战** | 协议、序列化、GameServer、GameClient、ServerApp、ClientApp | §2 全部、阶段二 | `Network/*`（全部新建）、`App/*` |
| **丙：UI 与稳定性** | 动画、特效、布局、日志、联网入口 UI、View/App 测试 | §3、阶段三 | `View/*`、`tests/*` |

其中 §2.5 表格里的两个纯 View 文件（`MainWindow` 联网入口、`NetworkConfigDialog`）划给丙而不是乙——它们不依赖 Network 层类型（通过信号传出 host/port），本质是 UI 内容，同时给乙减量。

### 7.2 第 0 步：契约前置（约 2 天，不能省）

乙的序列化依赖甲给 Common 值类型加的新字段，这是本切法最大的串行依赖，处理方式：

1. **甲先提交 Common 新字段的结构定义**——✅ 已完成：`PlayerData::weaponCard`/`armorCard`（按槽的 `CardData`，非早稿的 `equipCards` 列表）、`CardData::isEquipment`/`equipSlot`/`attackRange`、`CommonTypes.h` 的 `EquipSlot` 枚举与 9 种装备 `CardType`。坐骑（Mount）槽位已预留枚举、暂无对应牌。乙的序列化以这些实际字段为准。
2. **乙随后冻结 `Network/Protocol.h`**：消息枚举 + 消息结构体，字段直接对齐 `GameViewModel`/`ActionViewModel` 的 slots/signals（§2.2 已列全）；并写 Common 值类型的 `QDataStream <<`/`>>` 运算符（甲评审）。建议协议加版本号字段，字段变更时递增。
3. **乙的 `ServerApp`/`ClientApp` 组装最先合入**：先完成服务器 headless 启动和客户端信号翻译，再接入完整网络流程。

### 7.3 乙的网络主线（约 4 周）

| 顺序 | 任务 | 说明 |
|------|------|------|
| 1 | `Protocol.h` + `MessageSerializer` | 用 QDataStream，不手写帧校验（§2.2 建议） |
| 2 | `ServerApp` headless 路径 | 不创建 MainWindow/GameBoardWidget 的启动路径 |
| 3 | `GameServer` | QTcpServer，连接管理、playerId 分配、握手/选将流程 |
| 4 | **手牌脱敏** | 对手手牌只发 cardId + 数量，牌面字段清空（§2.2 强调的安全点，P0） |
| 5 | `GameClient` | 暴露与 GameViewModel/ActionViewModel 同形状的信号和方法，纯转发，零 Model |
| 6 | `ClientApp` | GameBoardWidget 信号 → GameClient 发送方法、GameClient 信号 → View 槽，照抄 `SGSApp::startLocalGame()` 的连接形状 |
| 7 | 心跳保活 | Ping/Pong + 超时踢出（§2.7）；断线重连留到联调后，首版可不做 |

### 7.4 甲、丙的支线与汇入点

**甲（第 1-2 周做 §1+§4，第 3-4 周支援联调）**：按阶段一优先级——技能 Bug 验证（先实跑一局确认 §4.1 四个武将，不要假设问题存在）→ 装备牌系统 → 锦囊补充 → 牌堆调整 → 武将拓展。第 3 周起加入联调，扮演第二个客户端做双人联调测试（M2 的脱敏互看必须两个真实客户端）。

**丙（第 1-2 周做 §3 动画 + 联网 UI，第 3-4 周做测试）**：卡牌动画、伤害/濒死特效、`MainWindow` 联网入口、`NetworkConfigDialog`；第一轮 Model/ViewModel/View/App 自动化测试已经完成，后续应补关羽/赵云主动转化的端到端断言，并在网络层落地后编写网络对局回归清单。丙后半段的测试工作依赖甲的卡牌落地和乙的联调进度，如出现空档优先消化阶段三的 P2 项（牌桌美化、日志增强）。

### 7.5 联调里程碑（乙主导，甲丙第 3 周起加入，约 1 周）

1. **M1 握手**：连接 → HandshakeAck → 双方选将 → GameStarted
2. **M2 单向同步**：服务器跑一局，客户端只收不发，验证 UI 渲染 + 脱敏正确（用两个客户端互看对方手牌是否为牌背——需要甲扮演第二客户端）
3. **M3 完整对局**：出杀 → 响应闪 → 伤害结算 → 游戏结束
4. **M4 异常**：中途拔线、心跳超时、非法消息（如非当前回合出牌）

### 7.6 风险与协调规则

1. **工作量不均是本切法的固有代价**：乙 4 周网络主线无人分担。缓解手段已内置在 7.1（View 侧网络 UI 划给丙）和 7.4（甲第 3 周汇入联调）——如果乙进度落后，优先砍掉的是心跳保活和断线重连（首版可不做，见 §2.7），而不是手牌脱敏（P0）。
2. **Common 值类型字段变更必须走三人同步**：甲加字段 → 乙补序列化 → 丙补 View 渲染（§1.1 和 §2.2 都依赖这条链）。甲在阶段一中途如需再加字段，必须先通知乙升协议版本号。
3. **`App/ServerApp.h/cpp` 和 `App/ClientApp.h/cpp` 是网络组装热点**。规则：先冻结 Common DTO 和网络协议，再分别合入 ServerApp、ClientApp；现有 `SGSApp` 只保留本地模式连接，不把网络逻辑回填到本地组合根。
4. **View 层零改动是验收标准**：如果联调中发现 `GameBoardWidget` 需要为网络模式改代码，说明设计跑偏了——先回头查 `ClientApp` 的信号连接，而不是改 View。丙的动画改动只加新方法，不改现有槽的签名，否则会波及乙的连接代码。
