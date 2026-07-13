# 三国杀 2.0 — 拓展计划

> 基于当前已完成的 MVVM 架构（Model + ViewModel + Qt View），对游戏内容、网络对战、界面体验和稳定性进行全面升级。

---

## 0. 当前状态

```
Model 层 (纯 C++17)       ✅ 完整 — 数据、规则、武将技能
Core 层 (纯 C++17)        ✅ 完整 — CommonTypes, Event, RandomUtils
ViewModel 层 (纯 C++17)   ✅ 完整 — GameVM, PlayerVM, CardVM, ActionVM
View 层 (Qt Widgets)      ✅ 完整 — 7 个控件，完整双人本地对战
冒烟测试                  ✅ 95/95 通过
```

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

```cpp
// Player.h 新增
class Player {
    // 装备管理
    void equipCard(EquipmentCard* card);              // 装备
    void unequipSlot(EquipSlot slot);                 // 卸下
    EquipmentCard* equippedAt(EquipSlot slot) const;  // 查询
    int attackRange() const;                          // 计算攻击距离
    bool hasArmor() const;                            // 是否有防具

    // 事件
    EventListener<EquipSlot> equipmentChanged;         // 装备变化

private:
    EquipmentCard* m_equipSlots[4] = {nullptr};       // 装备槽
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

- `PlayerViewModel` 新增 `equipCards()` 返回装备列表
- `PlayerInfoWidget` 新增装备区显示（在体力下方或右侧）
- `CardViewModel` 新增 `isEquipment()`, `equipSlotString()`
- 装备牌的手绘渲染增加"装备"分类标签和特殊边框

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

```cpp
class Character {
    // 新增技能触发点
    virtual int onDrawPhaseBonus() const;          // 摸牌阶段额外摸牌数（周瑜）
    virtual bool requireExtraDodge() const;        // 是否需要两张闪（吕布）
    virtual bool canRedirectKill() const;          // 是否可以转移杀（大乔）
    virtual Player* getRedirectTarget(GameState* state, Player* self,
                                       Player* attacker) const;

    // 新增事件触发
    EventListener<const std::string&> skillDisplay;  // 技能发动文字提示
};
```

---

## 2. 局域网双人对战（核心拓展）

### 2.1 架构设计

#### 网络边界位置

```
┌───────── 服务器（Server）─────────┐     ┌───────── 客户端（Client）─────────┐
│                                   │     │                                   │
│  ┌────────────┐                   │     │                    ┌────────────┐ │
│  │ ViewModel  │◄─── LAN 协议 ────│─────│─── LAN 协议 ────► │ ViewModel  │ │
│  │ (纯 C++17) │                   │     │                    │ (纯 C++17) │ │
│  └──────┬─────┘                   │     │                    └──────┬─────┘ │
│         │                         │     │                           │       │
│  ┌──────▼──────┐                  │     │                    ┌──────▼──────┐│
│  │   Model     │                  │     │                    │   View      ││
│  │ GameState   │                  │     │                    │ (Qt Widgets)││
│  │ CardManager │                  │     │                    └─────────────┘│
│  │ GameRule    │                  │     │                                   │
│  └─────────────┘                  │     │                                   │
└───────────────────────────────────┘     └───────────────────────────────────┘

         上层（Server ViewModel ←→ Client ViewModel）
              ↓
          ┌──────────┐
          │ 网络传输层 │
          │ Socket    │
          │ 序列化     │
          │ 协议解析   │
          └──────────┘
```

**核心决策**：网络边界位于 **ViewModel 之间**。

- **服务器**拥有完整的 Model + ViewModel，执行**全部游戏逻辑**
- **客户端**运行 ViewModel 的薄壳 + View，所有游戏操作通过 LAN 协议转发到服务器执行
- 服务器 ViewModel 将状态变化编码为**网络协议消息**发送给客户端
- 客户端 ViewModel 解码消息，更新本地状态，通过 `EventListener` 通知 View

#### 为什么选择这个边界

| 边界 | 优点 | 缺点 |
|------|------|------|
| Model → VM（本方案） | ViewModel 接口对 View 不变；View 不感知网络 | 协议需覆盖所有 VM 事件 |
| VM → View | 协议简单（只传值类型） | View 必须修改为 async |
| 直接同步 Model | 逻辑简单 | 暴露 Model 指针，破坏 MVVM |

### 2.2 网络协议设计

#### 协议层架构

```
src/Network/
├── Protocol.h              # 消息类型枚举 + 消息结构体
├── MessageSerializer.h/cpp # 序列化/反序列化（JSON 或二进制）
├── GameServer.h/cpp        # 服务器端（TCP + 游戏逻辑桥接）
├── GameClient.h/cpp        # 客户端（TCP + VM 状态同步）
└── RemoteGameVM.h/cpp      # 客户端的 GameViewModel 替代品
```

#### 消息类型定义

```cpp
// Network/Protocol.h
enum class MessageType : uint8_t {
    // 握手
    Handshake,              // 客户端 → 服务器：请求连接
    HandshakeAck,           // 服务器 → 客户端：连接确认

    // 选将阶段
    SelectCharacter,        // 客户端 → 服务器：选择武将
    CharacterConfirmed,     // 服务器 → 客户端：选将确认（含对手信息）

    // 游戏操作
    PlayCard,               // 客户端 → 服务器：出牌
    RespondCard,            // 客户端 → 服务器：响应出牌
    SkipResponse,           // 客户端 → 服务器：跳过响应
    EndPlayPhase,           // 客户端 → 服务器：结束出牌
    DiscardCard,            // 客户端 → 服务器：弃牌
    ConfirmDiscard,         // 客户端 → 服务器：确认弃牌

    // 状态同步（服务器 → 客户端）
    StateSync,              // 完整状态同步（重连/初始化）
    PhaseChanged,           // 阶段变化
    PlayerStateChanged,     // 玩家状态变化（体力、手牌数）
    HandCardsRevealed,      // 手牌内容更新（本方玩家看到具体牌）
    OpponentCardCount,      // 对手手牌张数
    PendingAction,          // 待定动作信息
    LogMessage,             // 日志消息
    GameOver,               // 游戏结束

    // 保活
    Ping,
    Pong,
};
```

#### 序列化格式

使用轻量级二进制序列化（非 JSON，减少解析开销和包大小）：

```cpp
// Network/MessageSerializer.h
class MessageSerializer {
public:
    // 序列化操作 → 字节流
    std::vector<uint8_t> serialize(const PlayCardMsg& msg);
    std::vector<uint8_t> serialize(const StateSyncMsg& msg);
    // ...

    // 反序列化字节流 → 消息
    template<typename T>
    T deserialize(const std::vector<uint8_t>& data);

private:
    // 基础类型读写
    void writeUint8(std::vector<uint8_t>& buf, uint8_t val);
    void writeInt32(std::vector<uint8_t>& buf, int32_t val);
    void writeString(std::vector<uint8_t>& buf, const std::string& str);
    // ...
};
```

**消息结构体示例**：

```cpp
struct PlayCardMsg {          // 客户端 → 服务器
    MessageType type = MessageType::PlayCard;
    int32_t cardId;
    int32_t playerId;
    std::vector<int32_t> targetIds;
};

struct StateSyncMsg {         // 服务器 → 客户端
    MessageType type = MessageType::StateSync;
    PhaseType phase;
    int32_t currentPlayerId;
    int32_t turnCount;
    struct PlayerState {
        int32_t playerId;
        int32_t hp;
        int32_t maxHp;
        int32_t handCardCount;
        bool isDying;
        bool isAlive;
        std::string characterName;
        std::string skillName;
        std::string skillDesc;
        // 本方手牌内容（只有本方能看）
        std::vector<CardInfo> handCards;      // 含 cardId, cardType, suit, number
        // 装备区
        std::vector<CardInfo> equipCards;
    };
    std::vector<PlayerState> players;
};

struct CardInfo {
    int32_t cardId;
    CardType cardType;
    CardSuit suit;
    int32_t number;
    std::string cardName;
};
```

#### 帧格式

```
┌─────────────────────────────────────────────┐
│ Frame Header (8 bytes)                       │
│  ┌──────┬──────────┬──────────┬──────────┐  │
│  │ Sync │ MsgType  │ Payload  │ Checksum │  │
│  │ 0xAB │ 1 byte   │ Length   │ 2 bytes  │  │
│  │ 0xCD │          │ 4 bytes  │          │  │
│  └──────┴──────────┴──────────┴──────────┘  │
├─────────────────────────────────────────────┤
│ Payload (变长)                               │
│  序列化后的消息结构体                          │
└─────────────────────────────────────────────┘
```

### 2.3 服务器设计（GameServer）

```
GameServer (QTcpServer)
├── 监听端口（默认 9527）
├── 管理最多 2 个客户端连接
├── 拥有完整的 GameViewModel + Model
├── 消息循环
│   ├── 接收客户端操作消息
│   ├── 调用 GameViewModel 对应的操作方法
│   ├── 等待 Model 状态变更
│   └── 广播同步消息给所有客户端
└── 状态同步定时器（用于保活和重连）
```

```cpp
// Network/GameServer.h
class GameServer : public QObject {
    Q_OBJECT
public:
    explicit GameServer(QObject* parent = nullptr);
    ~GameServer();

    bool start(quint16 port = 9527);
    void stop();

signals:
    void serverStarted();
    void serverStopped();
    void clientConnected(int clientId);
    void clientDisconnected(int clientId);
    void gameStarted();
    void gameEnded(int winnerId);

private slots:
    void onNewConnection();
    void onClientMessage();
    void onClientDisconnected();
    void onGameStateChanged();

private:
    struct ClientInfo {
        QTcpSocket* socket;
        int playerId;           // -1 = 未分配
        bool ready = false;     // 选将已确认
    };

    void processMessage(ClientInfo* client, const std::vector<uint8_t>& data);
    void broadcast(const std::vector<uint8_t>& data, int excludeId = -1);
    void sendTo(ClientInfo* client, const std::vector<uint8_t>& data);

    void handleHandshake(ClientInfo* client);
    void handleSelectCharacter(ClientInfo* client, const SelectCharMsg& msg);
    void handlePlayCard(ClientInfo* client, const PlayCardMsg& msg);
    // ...

    void syncFullState(ClientInfo* client);     // 发送完整状态
    void syncPhaseChange(PhaseType phase);      // 发送阶段变化
    void syncPlayerState(int playerId);        // 发送玩家状态变化
    void syncPendingAction(const PendingActionVM& info);  // 发送待定动作

    QTcpServer* m_server;
    std::vector<ClientInfo> m_clients;
    GameViewModel* m_gvm;        // 服务器持有完整 GameVM

    // 连接 ID 管理（用于断开 VM 事件）
    std::vector<size_t> m_vmConnections;
};
```

### 2.4 客户端设计（GameClient + RemoteGameVM）

```
GameClient (QObject)
├── 连接到服务器
├── 消息接收与解析
├── 控制 RemoteGameVM 的状态更新

RemoteGameVM (充当 GameViewModel 的替代品)
├── 实现与 GameViewModel 相同的公共接口
│   ├── currentPlayerId(), currentPhase(), etc.
│   ├── 本地快速响应（手牌缓存）
│   └── 操作转发到服务器
├── 不包含 Model，不执行游戏逻辑
├── 数据由服务器同步消息驱动
└── 提供相同的 EventListener 接口
```

```cpp
// Network/GameClient.h
class GameClient : public QObject {
    Q_OBJECT
public:
    explicit GameClient(QObject* parent = nullptr);
    ~GameClient();

    void connectToServer(const QString& host, quint16 port = 9527);
    void disconnect();

    // 发送操作
    void sendSelectCharacter(int characterId);
    void sendPlayCard(int cardId, const std::vector<int>& targetIds);
    void sendRespondCard(int cardId);
    void sendSkipResponse();
    void sendEndPlayPhase();
    void sendDiscardCard(int cardId);
    void sendConfirmDiscard();

    // 获取远程 VM
    RemoteGameVM* remoteVM() const;

signals:
    void connected();
    void disconnected();
    void connectionError(const QString& error);
    void playerIdAssigned(int playerId);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);

private:
    void processMessage(const std::vector<uint8_t>& data);
    void handleStateSync(const StateSyncMsg& msg);
    void handlePhaseChange(PhaseType phase);
    void handlePlayerState(const PlayerStateMsg& msg);
    void handlePendingAction(const PendingActionVM& info);
    void handleGameOver(int winnerId);

    QTcpSocket* m_socket;
    std::unique_ptr<RemoteGameVM> m_remoteVM;

    // 接收缓冲区
    std::vector<uint8_t> m_recvBuffer;
};
```

```cpp
// Network/RemoteGameVM.h
/// 远程 GameViewModel — 客户端侧的 VM 替代品
/// 不包含 Model，所有数据由服务器同步驱动
/// 公共接口与 GameViewModel 一致，View 层无需修改即可使用
class RemoteGameVM {
public:
    RemoteGameVM();
    ~RemoteGameVM();

    // ==================== 生命周期（由 GameClient 驱动）====================
    void applyStateSync(const StateSyncMsg& msg);
    void applyPhaseChange(PhaseType phase);
    void applyPlayerStateChange(int playerId, int hp, int handCount, ...);
    void applyHandCards(int playerId, const std::vector<CardInfo>& cards);
    void applyPendingAction(const PendingActionVM& info);
    void applyPendingActionCleared();
    void applyLog(const std::string& msg);
    void applyGameOver(int winnerId);

    // ==================== 状态查询（与 GameViewModel 相同的接口）====================
    int currentPlayerId() const;
    int opponentPlayerId() const;
    PhaseType currentPhase() const;
    int turnCount() const;
    bool isGameOver() const;
    int winnerId() const;
    bool hasPendingAction() const;
    PendingActionVM pendingActionVM() const;
    int playerHp(int playerId) const;
    int playerMaxHp(int playerId) const;
    int playerHandCount(int playerId) const;
    std::string playerDisplayName(int playerId) const;
    std::string playerCharacterName(int playerId) const;

    // 手牌（只有本方玩家能看到具体内容）
    const std::vector<CardInfo>& handCardInfos(int playerId) const;

    // 子 VM 访问（客户端侧适配）
    PlayerVMAdapter* playerVM(int index) const;

    // ==================== 事件（与 GameViewModel 相同的接口）====================
    EventListener<PhaseType> phaseChanged;
    EventListener<int> currentPlayerChanged;
    EventListener<int> gameOver;
    EventListener<const std::string&> logMessage;
    EventListener<const PendingActionVM&> pendingActionCreated;
    EventListener<> pendingActionCleared;
    EventListener<> stateChanged;

private:
    struct PlayerState {
        int playerId;
        int hp;
        int maxHp;
        int handCardCount;
        bool isAlive;
        bool isDying;
        std::string displayName;
        std::string characterName;
        std::string skillName;
        std::string skillDesc;
        std::vector<CardInfo> handCards;     // 内容（仅本方可见）
    };

    PhaseType m_phase;
    int m_currentPlayerId;
    int m_turnCount;
    bool m_gameOver;
    int m_winnerId;

    bool m_hasPendingAction;
    PendingActionVM m_pendingAction;

    std::vector<PlayerState> m_players;
    std::vector<std::unique_ptr<PlayerVMAdapter>> m_playerVMAdapters;
};
```

```cpp
// Network/PlayerVMAdapter.h
/// PlayerViewModel 适配器 — 替代 PlayerViewModel，由 RemoteGameVM 驱动
class PlayerVMAdapter {
public:
    // 与 PlayerViewModel 相同的只读接口
    int playerId() const;
    int hp() const;
    int maxHp() const;
    int handCardCount() const;
    bool isAlive() const;
    bool isDying() const;
    std::string displayName() const;
    std::string characterName() const;
    std::string skillName() const;
    std::string skillDescription() const;

    // 相同的事件接口
    EventListener<int> hpChanged;
    EventListener<int> maxHpChanged;
    EventListener<> dying;
    EventListener<> died;
    EventListener<> handCardsChanged;
    EventListener<> stateChanged;

    // 由 RemoteGameVM 调用更新
    void applyHpChange(int hp, int maxHp);
    void applyDying(bool dying);
    void applyHandCountChange(int count);
};
```

### 2.5 View 层适配

#### 原有 GameBoardWidget 的修改

```cpp
// View/GameBoardWidget.h
class GameBoardWidget : public QWidget {
public:
    // 原有构造函数不变（本地模式）
    explicit GameBoardWidget(GameViewModel* gvm, QWidget* parent = nullptr);

    // 新增：远程模式构造函数
    explicit GameBoardWidget(RemoteGameVM* rvm, GameClient* client,
                              int localPlayerId, QWidget* parent = nullptr);

private:
    // 统一接口类型（无论是本地还是远程，View 看到的是一样的）
    // 使用 variant 或抽象接口？
    // 更简单的方案：GameBoardWidget 内部抽象出 GameController 接口
    GameController* m_controller;    // 本地/远程的统一抽象
};
```

#### GameController 抽象接口

```cpp
// View/GameController.h
/// View 层统一的控制器抽象 — 屏蔽本地 VS 远程的差异
class GameController {
public:
    virtual ~GameController() = default;

    // 状态查询
    virtual int currentPlayerId() const = 0;
    virtual int opponentPlayerId() const = 0;
    virtual PhaseType currentPhase() const = 0;
    virtual int turnCount() const = 0;
    virtual bool isGameOver() const = 0;
    virtual int winnerId() const = 0;
    virtual bool hasPendingAction() const = 0;
    virtual PendingActionVM pendingActionVM() const = 0;

    // 操作
    virtual void advancePhase() = 0;
    virtual void endPlayPhase() = 0;
    virtual void advanceToInteractivePhase() = 0;
    virtual ActionResult playCard(int cardId, int playerId,
                                   const std::vector<int>& targetIds) = 0;
    virtual void respondCard(int cardId, int playerId) = 0;
    virtual void skipResponse(int playerId, bool forceNoCard) = 0;
    virtual void discardCard(int cardId, int playerId) = 0;
    virtual int getDiscardCount(int playerId) const = 0;
    virtual bool isOwnCard(int cardId, int playerId) const = 0;
    virtual bool canPlayCard(int cardId, int playerId) const = 0;
    virtual std::vector<int> getPlayableCardIds(int playerId) const = 0;
    virtual std::vector<int> getValidTargetIds(int cardId, int playerId) const = 0;
    virtual std::vector<int> getResponseCardIds(int playerId,
                                                 CardType requiredType) const = 0;

    // 子 ViewModel
    virtual PlayerViewModel* playerVM(int index) const = 0;

    // 卡牌显示数据
    virtual std::string cardNameById(int cardId) const = 0;
    virtual CardType cardTypeById(int cardId) const = 0;

    // 手牌
    virtual std::vector<std::unique_ptr<CardViewModel>>
        getCurrentPlayerCardVMs() const = 0;
    virtual std::vector<std::unique_ptr<CardViewModel>>
        getPlayerCardVMs(int playerIndex) const = 0;

    // 事件
    virtual EventListener<PhaseType>* phaseChanged() = 0;
    virtual EventListener<int>* currentPlayerChanged() = 0;
    virtual EventListener<int>* gameOver() = 0;
    virtual EventListener<const std::string&>* logMessage() = 0;
    virtual EventListener<const PendingActionVM&>* pendingActionCreated() = 0;
    virtual EventListener<>* pendingActionCleared() = 0;
    virtual EventListener<>* stateChanged() = 0;
};
```

```cpp
// View/LocalController.h
/// 本地模式适配器 — 包装 GameViewModel
class LocalController : public GameController {
    GameViewModel* m_gvm;
    // 所有方法直接委托给 m_gvm
};
```

```cpp
// View/RemoteController.h
/// 远程模式适配器 — 包装 RemoteGameVM + GameClient
class RemoteController : public GameController {
    RemoteGameVM* m_rvm;
    GameClient* m_client;
    int m_localPlayerId;
    // 读操作委托给 m_rvm
    // 写操作通过 m_client 发给服务器
    // 手牌 VM 本地构造（用 CardInfo 缓存）
};
```

#### View 层修改汇总

| 文件 | 修改 |
|------|------|
| `GameBoardWidget.h` | 接受 `GameController*` 而非 `GameViewModel*` |
| `GameBoardWidget.cpp` | 所有 `m_gvm->xxx()` 改为 `m_controller->xxx()` |
| `MainWindow.h/cpp` | 新增"局域网对战"按钮 + 连接界面 |
| 新增 `GameController.h` | 抽象控制器接口 |
| 新增 `LocalController.h/cpp` | 本地模式实现 |
| 新增 `RemoteController.h/cpp` | 远程模式实现 |
| 新增 `NetworkConfigDialog.h/cpp` | 联网配置对话框（IP/端口） |

### 2.6 双人局域网流程

```
服务器端（房主）                          客户端（加入者）
─────────────────                        ─────────────────
1. 启动游戏，选择「创建房间」
   服务器开始监听端口 9527
   
2. 看到等待界面                    →    3. 输入服务器 IP + 端口
                                        点击「加入房间」
                                        
4. 接受连接，分配 PlayerId=0      ←    5. 收到 PlayerId=1 确认
   发送初始状态
   
6. 选将界面：选择武将               →    7. 选将界面：选择武将
   发送 SelectCharacter(曹操)             发送 SelectCharacter(关羽)
   
8. 服务器执行 startGame(0,1)
   开始游戏
   广播 StateSync 给双方
   
9. 广播 PhaseChanged(Play)         ←    10. 收到状态，渲染游戏界面

11. P1 双击「杀」
    发送 PlayCard(killId, 0, {1})  →
    
12. 服务器执行 playCard
    广播 PendingAction(Dodge, P2)  →   13. 收到待定动作，显示响应界面
    
14.                             ←    15. P2 点击「跳过」
                                        发送 SkipResponse
    
16. 服务器执行 skipResponse → damage
    广播 PlayerStateChanged(P2, hp-1)  → 17. 收到状态更新
    
18. P1 出牌结束                     →    19. ...
    ...
```

### 2.7 可靠性设计

```cpp
// 心跳保活
class GameServer {
    void startHeartbeat() {
        m_heartbeatTimer = new QTimer(this);
        connect(m_heartbeatTimer, &QTimer::timeout, [this]() {
            broadcast(serialize(PingMsg{}));
            // 检查上次接收时间，超时 10s 断开
            for (auto& client : m_clients) {
                if (client.socket->state() == QAbstractSocket::ConnectedState
                    && client.lastRecvTime.elapsed() > 10000) {
                    client.socket->disconnectFromHost();
                }
            }
        });
        m_heartbeatTimer->start(3000);  // 每 3s 一次
    }
};
```

```cpp
// 重连支持（预留）
class GameClient {
    void startReconnectTimer() {
        m_reconnectTimer = new QTimer(this);
        connect(m_reconnectTimer, &QTimer::timeout, [this]() {
            if (!m_connected) {
                m_socket->connectToHost(m_host, m_port);  // 尝试重连
            }
        });
        m_reconnectTimer->start(5000);
    }
};
```

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

### 4.1 程序运行一段时间后无响应退出

**现象**：长时间运行后程序崩溃或卡死。

**可能原因与修复**：

| 可能原因 | 验证方法 | 修复 |
|---------|---------|------|
| EventListener 回调中调用 delete 或 deleteLater 导致悬垂指针 | 添加 AddressSanitizer 编译 | 队列化所有 UI 更新，使用 QMetaObject::invokeMethod |
| CardViewModel 生命周期问题（旧控件访问已销毁的 VM） | 在 clearWidgets 中打印日志 | 已部分修复，需验证全部路径 |
| 弃牌堆回收时牌对象生命周期 | 在 CardManager 析构时添加检查 | 确保没有外部指针指向已回收的牌 |
| Qt 定时器在对象销毁后仍然触发 | 添加 isDestroyed 标记 | 在析构函数中停止所有定时器 |

**系统性修复**：

```cpp
// 对所有 QTimer 添加生命周期保护
class GameBoardWidget {
    // 在析构函数中停止所有定时器
    ~GameBoardWidget() {
        m_autoAdvanceTimer->stop();
        // 或使用 QTimer::singleShot 替代成员定时器
    }
};

// 对所有 EventListener 添加生命期检查
// 在 Qt 回调中使用 QPointer 保护
void GameBoardWidget::onPhaseChanged(PhaseType phase) {
    if (!m_gvm) return;  // 对象已被销毁
    // ...
}
```

### 4.2 武将技能不能正常使用

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
std::vector<Card*> ActionViewModel::getResponseCards(Player* player, CardType requiredType) {
    std::vector<Card*> result;
    for (Card* card : player->handCards()) {
        if (card->cardType() == requiredType) {
            result.push_back(card);
            continue;
        }
        // 武将技能转化
        if (player->character()) {
            CardType transformed = player->character()->skillTransformCard(card);
            if (transformed == requiredType) {
                result.push_back(card);
            }
        }
    }
    return result;
}
```

### 4.3 有可响应牌时必须响应不可跳过

**问题**：对于某些待定动作（如南蛮入侵、桃园结义），玩家即使有可响应的牌也应该可以选择放弃。

**当前行为**：
- `updateForPendingAction(info)` 只在 `canSkip==true` 时显示「跳过」按钮
- 某些待定动作的 `canSkip` 设置为 `false`（如杀），强制响应

**期望行为**：
- 杀：必须有闪或必须扣血（`canSkip=false`，当前逻辑正确）
- 南蛮入侵/万箭齐发：可以选择不出杀/闪直接扣血（`canSkip=true`，当前逻辑正确）
- 桃园结义：所有玩家自动生效（不需要响应）
- 濒死求桃：可以放弃（`canSkip=true`，当前逻辑正确）

**修复**：检查每种待定动作的 `canSkip` 设置是否符合游戏规则。

```cpp
// GameRule.cpp 中验证每个 PendingAction 的 canSkip：
// 杀: canSkip=false ✓（不能无条件跳过）
// 南蛮入侵: canSkip=true ✓（可以选择扣血）
// 万箭齐发: canSkip=true ✓
// 濒死求桃: canSkip=true ✓（可以放弃）
// AOE 链式推进: canSkip=true ✓

// 补充：当 responseCards 为空时自动推进（已在 1.0 中实现）
void GameBoardWidget::onPendingActionCreated(const PendingActionVM& info) {
    std::vector<int> responseIds = avm->getResponseCardIds(info.targetId, info.requiredCardType);
    if (responseIds.empty()) {
        // 无响应牌，自动跳过（承担后果）
        avm->skipResponse(info.targetId, true);  // forceNoCard=true
        return;
    }
    // 正常显示响应界面...
}
```

---

## 5. 实施路线图

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
| 任务 | 文件 | 优先级 |
|------|------|--------|
| Protocol.h 消息定义 | `Network/Protocol.h` | P0 |
| MessageSerializer | `Network/MessageSerializer.h/cpp` | P0 |
| GameServer | `Network/GameServer.h/cpp` | P0 |
| GameClient | `Network/GameClient.h/cpp` | P0 |
| RemoteGameVM | `Network/RemoteGameVM.h/cpp` | P0 |
| GameController 抽象 | `View/GameController.h` | P0 |
| LocalController | `View/LocalController.h/cpp` | P0 |
| RemoteController | `View/RemoteController.h/cpp` | P0 |
| GameBoardWidget 改造 | `View/GameBoardWidget.h/cpp` | P0 |
| MainWindow 联网选将 | `View/MainWindow.h/cpp` | P1 |
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
| 冒烟测试补充（95→150+） | P0 |

---

## 6. 架构兼容性检查

### 6.1 当前 MVVM 边界是否被突破？

| 改动 | 是否突破边界 | 理由 |
|------|------------|------|
| EquipmentCard 新类 | ❌ 否 | Model 层纯数据，无 UI 依赖 |
| 装备槽在 Player 中 | ❌ 否 | 纯数据成员 + 事件 |
| GameController 抽象 | ❌ 否 | View 层内部抽象，不涉及 Model/VM |
| RemoteGameVM | ❌ 否 | ViewModel 层替代品，接口兼容 |
| RemoteController | ❌ 否 | View 层适配器，不直接访问 Model |
| GameServer 持有 GameVM | ❌ 否 | 服务器端拥有完整 VM，这是对的 |
| 网络协议序列化 | ❌ 否 | 值类型（int/string/enum），不涉及裸指针 |

### 6.2 需要特别注意的边界

```
View 层通过 RemoteController 访问 RemoteGameVM
  └── RemoteGameVM 内部只存储值类型
    └── 值类型通过序列化从服务器同步得到
      └── 服务器拥有真正的 Model
        └── Model 不感知网络的存在
```

**关键约束**：RemoteGameVM 永远不能持有 `Model::Player*` 或 `Model::Card*` 指针。所有数据必须通过值类型（`int`, `std::string`, `CardInfo` 结构体）存储和传递。
