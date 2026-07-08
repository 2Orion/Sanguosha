# Model 层公开接口定义

> 本文档定义 Model 层所有类的公开接口，作为与 ViewModel 层的契约。
> 人员 B（ViewModel）和人员 C（View）以此为准进行开发，可先基于此 mock 数据。

---

## 目录

1. [公共枚举与类型](#1-公共枚举与类型)
2. [游戏状态 GameState](#2-游戏状态-gamestate)
3. [待定动作 PendingActionInfo](#3-待定动作-pendingactioninfo)
4. [卡牌基类 Card](#4-卡牌基类-card)
5. [基本牌子类](#5-基本牌子类)
6. [锦囊牌子类](#6-锦囊牌子类)
7. [武将基类 Character](#7-武将基类-character)
8. [具体武将](#8-具体武将)
9. [玩家 Player](#9-玩家-player)
10. [牌堆管理 CardManager](#10-牌堆管理-cardmanager)
11. [规则引擎 GameRule](#11-规则引擎-gamerule)
12. [数据流示例](#12-数据流示例)
13. [接口依赖关系与关键设计](#13-接口依赖关系与关键设计)

---

## 1. 公共枚举与类型

```cpp
#ifndef COMMONTYPES_H
#define COMMONTYPES_H

#include <QObject>

// ==================== 卡牌枚举 ====================

/// 卡牌花色
enum class CardSuit {
    Spade,   // 黑桃 ♠
    Heart,   // 红桃 ♥
    Club,    // 梅花 ♣
    Diamond  // 方块 ♦
};

/// 卡牌颜色（根据花色派生）
enum class CardColor {
    Red,   // ♥♦
    Black  // ♠♣
};

/// 卡牌大类
enum class CardCategory {
    Basic,     // 基本牌
    Strategy,  // 锦囊牌
    Equipment  // 装备牌（暂不实现）
};

/// 卡牌类型
enum class CardType {
    // 基本牌
    Kill,       // 杀
    Dodge,      // 闪
    Peach,      // 桃
    Wine,       // 酒

    // 锦囊牌
    Dismantle,          // 过河拆桥
    Steal,              // 顺手牵羊
    Bountiful,          // 无中生有
    BarbarianInvasion,  // 南蛮入侵
    Volley,             // 万箭齐发
    PeachGarden,        // 桃园结义
};

// ==================== 游戏流程枚举 ====================

/// 回合阶段
enum class PhaseType {
    Prepare,  // 准备阶段
    Judge,    // 判定阶段
    Draw,     // 摸牌阶段
    Play,     // 出牌阶段
    Discard,  // 弃牌阶段
    End,      // 结束阶段
};

/// 卡牌所在区域
enum class CardArea {
    Hand,      // 手牌区
    Equipment, // 装备区（预留）
    Judgment,  // 判定区（预留）
    Pending    // 处理中
};

/// 游戏事件枚举（用于武将技能触发判断）
enum class GameEvent {
    OnDamage,          // 受到伤害后
    OnDamageCaused,    // 造成伤害后
    OnCardPlayed,      // 使用牌后
    OnCardResponded,   // 打出牌响应后
    OnDrawPhase,       // 摸牌阶段
    OnTurnStart,       // 回合开始
    OnDying,           // 濒死时
};

/// 动作结果（ViewModel 据此判断下一步）
enum class ActionResult {
    Completed,         // 动作已完成，无后续
    RequiresDodge,     // 目标需要出闪
    RequiresKill,      // 目标需要出杀
    RequiresPeach,     // 濒死需要出桃
    RequiresDiscard,   // 需要弃牌
};

// ==================== 转发声明 ====================

class Player;
class Card;
class GameState;
class CardManager;
class Character;

#endif // COMMONTYPES_H
```

---

## 2. 游戏状态 GameState

> 游戏全局状态的快照。ViewModel 通过监听 GameState 的信号来驱动 UI 刷新。
> 由 GameViewModel 创建并持有，每个回合更新。

```cpp
#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <QObject>
#include <QList>
#include "CommonTypes.h"

class Player;
class CardManager;

/// 待定动作信息（当需要玩家响应时填充此结构）
struct PendingActionInfo {
    Player* source;             // 动作来源（使用牌的玩家）
    Player* target;             // 需要响应的玩家
    Card*   sourceCard;         // 触发动作的卡牌
    CardType requiredCardType;  // 需要打出的卡牌类型
    QString description;        // 界面提示文字（如"请打出一张【闪】"）
    bool    canSkip;            // 是否可以跳过响应
};

class GameState : public QObject {
    Q_OBJECT
public:
    explicit GameState(QObject* parent = nullptr);
    ~GameState() override = default;

    // ---- 阶段管理 ----
    PhaseType currentPhase() const;
    void setCurrentPhase(PhaseType phase);

    // ---- 玩家管理 ----
    int currentPlayerIndex() const;
    void setCurrentPlayerIndex(int index);
    Player* currentPlayer() const;
    Player* player(int index) const;
    int playerCount() const;
    void addPlayer(Player* player);
    QList<Player*> alivePlayers() const;
    QList<Player*> allPlayers() const;

    // ---- 回合追踪 ----
    int turnCount() const;
    void incrementTurn();

    // ---- 待定动作 ----
    bool hasPendingAction() const;
    const PendingActionInfo& pendingActionInfo() const;
    void setPendingAction(const PendingActionInfo& info);
    void clearPendingAction();

    // ---- 牌堆管理器 ----
    CardManager* cardManager() const;
    void setCardManager(CardManager* mgr);

    // ---- 游戏结束 ----
    bool isGameOver() const;
    Player* winner() const;
    void setGameOver(Player* winnerPlayer);

signals:
    void phaseChanged(PhaseType newPhase);
    void currentPlayerChanged(int playerIndex);
    void pendingActionCreated(const PendingActionInfo& info);
    void pendingActionCleared();
    void gameOver(Player* winner);
    void stateRefreshed();  // 通用刷新信号

private:
    PhaseType m_currentPhase = PhaseType::Prepare;
    int m_currentPlayerIndex = 0;
    int m_turnCount = 0;
    QList<Player*> m_players;
    CardManager* m_cardManager = nullptr;
    bool m_hasPendingAction = false;
    PendingActionInfo m_pendingAction;
    bool m_gameOver = false;
    Player* m_winner = nullptr;
};

#endif // GAMESTATE_H
```

---

## 3. 卡牌基类 Card

> 所有卡牌的基类。通过虚方法实现多态行为。
> 每张卡牌在游戏中有唯一 ID。

```cpp
#ifndef CARD_H
#define CARD_H

#include <QObject>
#include <QString>
#include <QList>
#include "CommonTypes.h"

class GameState;
class Player;

class Card : public QObject {
    Q_OBJECT
public:
    /// @param type   卡牌类型
    /// @param suit   花色
    /// @param number 点数（1-13，A-K）
    Card(CardType type, CardSuit suit, int number, QObject* parent = nullptr);
    ~Card() override = default;

    // ==================== 只读属性 ====================

    /// 全局唯一卡牌 ID
    int id() const;

    /// 卡牌类型
    CardType cardType() const;

    /// 花色
    CardSuit suit() const;

    /// 点数（1=A, 2-10, 11=J, 12=Q, 13=K）
    int number() const;

    /// 卡牌颜色（红/黑）
    CardColor color() const;

    /// 卡牌名称（中文，如"杀"、"顺手牵羊"）
    QString cardName() const;

    /// 卡牌说明文字
    QString description() const;

    // ==================== 分类判断 ====================

    CardCategory category() const;
    bool isBasic() const;
    bool isStrategy() const;
    bool isEquipment() const;  // 装备牌（预留）
    bool isRed() const;
    bool isBlack() const;

    // ==================== 游戏逻辑接口（子类重写）====================

    /// 判断玩家在当前状态下是否能使用此牌
    virtual bool canUse(const GameState* state, const Player* user) const;

    /// 判断某玩家是否可以作为此牌的合法目标
    virtual bool canTarget(const GameState* state,
                           const Player* user,
                           const Player* target) const;

    /// 获取所有合法目标列表
    virtual QList<Player*> getValidTargets(const GameState* state,
                                            const Player* user) const;

    /// 执行卡牌效果
    /// @return 动作结果，指示是否需要玩家进一步响应
    virtual ActionResult execute(GameState* state,
                                  Player* user,
                                  const QList<Player*>& targets);

    // ==================== 工具方法 ====================

    /// 根据 CardType 获取中文名称
    static QString cardTypeName(CardType type);

    /// 花色符号字符串
    QString suitSymbol() const;

    /// 点数显示字符串
    QString numberString() const;

protected:
    /// 子类在构造函数中调用以设置自定义名称/描述
    void setCardName(const QString& name);
    void setDescription(const QString& desc);

private:
    int m_id;
    CardType m_type;
    CardSuit m_suit;
    int m_number;
    QString m_cardName;
    QString m_description;

    static int s_nextCardId;  // 全局自增 ID
};

#endif // CARD_H
```

---

## 4. 基本牌子类

### 4.1 杀 (Kill)

```cpp
class KillCard : public Card {
    Q_OBJECT
public:
    KillCard(CardSuit suit, int number, QObject* parent = nullptr);
    ~KillCard() override = default;

    bool canUse(const GameState* state, const Player* user) const override;
    bool canTarget(const GameState* state,
                   const Player* user,
                   const Player* target) const override;
    QList<Player*> getValidTargets(const GameState* state,
                                    const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const QList<Player*>& targets) override;
};
```

**规则**：出牌阶段每回合限一次（张飞咆哮除外），目标为对手且存活，执行后对手需出闪或受伤。

---

### 4.2 闪 (Dodge)

```cpp
class DodgeCard : public Card {
    Q_OBJECT
public:
    DodgeCard(CardSuit suit, int number, QObject* parent = nullptr);
    ~DodgeCard() override = default;

    /// 闪不能在出牌阶段主动使用，始终返回 false
    bool canUse(const GameState* state, const Player* user) const override;
};
```

**规则**：只作为响应打出，不由 `canUse` 判定。ViewModel 在响应流程中直接调用 `GameRule`。

---

### 4.3 桃 (Peach)

```cpp
class PeachCard : public Card {
    Q_OBJECT
public:
    PeachCard(CardSuit suit, int number, QObject* parent = nullptr);
    ~PeachCard() override = default;

    bool canUse(const GameState* state, const Player* user) const override;
    bool canTarget(const GameState* state,
                   const Player* user,
                   const Player* target) const override;
    QList<Player*> getValidTargets(const GameState* state,
                                    const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const QList<Player*>& targets) override;
};
```

**规则**：
- 出牌阶段：仅对自己使用，体力不满时可用，回复 1 体力
- 濒死时：可由任意玩家对濒死角色使用（由 `GameRule::startDyingProcess` 管理）

---

### 4.4 酒 (Wine)

```cpp
class WineCard : public Card {
    Q_OBJECT
public:
    WineCard(CardSuit suit, int number, QObject* parent = nullptr);
    ~WineCard() override = default;

    bool canUse(const GameState* state, const Player* user) const override;
    QList<Player*> getValidTargets(const GameState* state,
                                    const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const QList<Player*>& targets) override;
};
```

**规则**：
- 出牌阶段：对自己使用，下张杀伤害 +1，不可叠加
- 濒死时：可当桃自救（由 ViewModel 决定使用方式）

---

## 5. 锦囊牌子类

### 5.1 锦囊牌基类

```cpp
class StrategyCard : public Card {
    Q_OBJECT
public:
    StrategyCard(CardType type, CardSuit suit, int number, QObject* parent = nullptr);
    ~StrategyCard() override = default;

    bool canUse(const GameState* state, const Player* user) const override;
};
```

---

### 5.2 过河拆桥 (Dismantle)

```cpp
class DismantleCard : public StrategyCard {
    Q_OBJECT
public:
    DismantleCard(CardSuit suit, int number, QObject* parent = nullptr);

    bool canTarget(const GameState* state,
                   const Player* user,
                   const Player* target) const override;
    QList<Player*> getValidTargets(const GameState* state,
                                    const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const QList<Player*>& targets) override;
};
```

---

### 5.3 顺手牵羊 (Steal)

```cpp
class StealCard : public StrategyCard {
    Q_OBJECT
public:
    StealCard(CardSuit suit, int number, QObject* parent = nullptr);

    bool canTarget(const GameState* state,
                   const Player* user,
                   const Player* target) const override;
    QList<Player*> getValidTargets(const GameState* state,
                                    const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const QList<Player*>& targets) override;
};
```

---

### 5.4 无中生有 (Bountiful)

```cpp
class BountifulCard : public StrategyCard {
    Q_OBJECT
public:
    BountifulCard(CardSuit suit, int number, QObject* parent = nullptr);

    QList<Player*> getValidTargets(const GameState* state,
                                    const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const QList<Player*>& targets) override;
};
```

---

### 5.5 南蛮入侵 (Barbarian Invasion)

```cpp
class BarbarianCard : public StrategyCard {
    Q_OBJECT
public:
    BarbarianCard(CardSuit suit, int number, QObject* parent = nullptr);

    bool canTarget(const GameState* state,
                   const Player* user,
                   const Player* target) const override;
    QList<Player*> getValidTargets(const GameState* state,
                                    const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const QList<Player*>& targets) override;
};
```

---

### 5.6 万箭齐发 (Volley)

```cpp
class VolleyCard : public StrategyCard {
    Q_OBJECT
public:
    VolleyCard(CardSuit suit, int number, QObject* parent = nullptr);

    bool canTarget(const GameState* state,
                   const Player* user,
                   const Player* target) const override;
    QList<Player*> getValidTargets(const GameState* state,
                                    const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const QList<Player*>& targets) override;
};
```

---

### 5.7 桃园结义 (Peach Garden)

```cpp
class PeachGardenCard : public StrategyCard {
    Q_OBJECT
public:
    PeachGardenCard(CardSuit suit, int number, QObject* parent = nullptr);

    QList<Player*> getValidTargets(const GameState* state,
                                    const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const QList<Player*>& targets) override;
};
```

---

## 6. 武将基类 Character

```cpp
#ifndef CHARACTER_H
#define CHARACTER_H

#include <QObject>
#include <QString>
#include "CommonTypes.h"

class GameState;
class Player;
class Card;

class Character : public QObject {
    Q_OBJECT
public:
    explicit Character(const QString& name, int maxHp,
                       const QString& skillName, const QString& skillDesc,
                       QObject* parent = nullptr);
    ~Character() override = default;

    // ==================== 只读属性 ====================

    QString characterName() const;
    int maxHp() const;
    QString skillName() const;
    QString skillDescription() const;

    // ==================== 技能系统（子类重写）====================

    /// 是否拥有技能（默认 true）
    virtual bool hasSkill() const;

    /// 判断技能触发条件是否满足
    virtual bool triggerCondition(GameEvent event,
                                  const GameState* state,
                                  const Player* self) const;

    /// 执行技能效果
    virtual void triggerSkill(GameState* state, Player* self);

    /// 是否可将某张牌转化为另一种牌使用
    /// 关羽：红牌→杀；赵云：杀↔闪
    /// @return 可转化成的 CardType，若不能转化则返回 card 的原始类型
    virtual CardType skillTransformCard(const Card* card) const;

signals:
    /// 技能发动信号（ViewModel 用来显示"XX发动了技能"）
    void skillTriggered(const QString& skillName);

protected:
    QString m_name;
    int m_maxHp;
    QString m_skillName;
    QString m_skillDescription;
};

#endif // CHARACTER_H
```

---

## 7. 具体武将

### 7.1 曹操 — 奸雄

```cpp
class CaoCao : public Character {
    Q_OBJECT
public:
    explicit CaoCao(QObject* parent = nullptr);

    bool hasSkill() const override;
    bool triggerCondition(GameEvent event,
                          const GameState* state,
                          const Player* self) const override;
    void triggerSkill(GameState* state, Player* self) override;
};
```

| 项目 | 内容 |
|------|------|
| 体力 | 4 血 |
| 技能 | **奸雄**：受到伤害后，获得造成伤害的牌（简化版：受到伤害后摸 1 张牌）|
| 触发时机 | `OnDamage`（受到伤害后）|

---

### 7.2 关羽 — 武圣

```cpp
class GuanYu : public Character {
    Q_OBJECT
public:
    explicit GuanYu(QObject* parent = nullptr);

    bool hasSkill() const override;
    CardType skillTransformCard(const Card* card) const override;
};
```

| 项目 | 内容 |
|------|------|
| 体力 | 4 血 |
| 技能 | **武圣**：红色牌可当【杀】使用或打出 |
| 实现 | `skillTransformCard`：若 card 为红色则返回 `CardType::Kill` |

---

### 7.3 张飞 — 咆哮

```cpp
class ZhangFei : public Character {
    Q_OBJECT
public:
    explicit ZhangFei(QObject* parent = nullptr);

    bool hasSkill() const override;
    bool triggerCondition(GameEvent event,
                          const GameState* state,
                          const Player* self) const override;
    void triggerSkill(GameState* state, Player* self) override;
};
```

| 项目 | 内容 |
|------|------|
| 体力 | 4 血 |
| 技能 | **咆哮**：出牌阶段可使用任意张【杀】 |
| 实现 | `GameRule::canPlayKill` 中检查武将类型，若为张飞则始终返回 true |

---

### 7.4 赵云 — 龙胆

```cpp
class ZhaoYun : public Character {
    Q_OBJECT
public:
    explicit ZhaoYun(QObject* parent = nullptr);

    bool hasSkill() const override;
    CardType skillTransformCard(const Card* card) const override;
};
```

| 项目 | 内容 |
|------|------|
| 体力 | 4 血 |
| 技能 | **龙胆**：可将【杀】当【闪】，【闪】当【杀】使用或打出 |
| 实现 | `skillTransformCard`：Kill→Dodge, Dodge→Kill |

---

## 8. 玩家 Player

```cpp
#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>
#include <QList>
#include <QString>
#include "CommonTypes.h"

class Card;
class Character;
class GameState;

class Player : public QObject {
    Q_OBJECT
public:
    explicit Player(QObject* parent = nullptr);
    ~Player() override;

    // ==================== 身份标识 ====================

    int playerId() const;
    void setPlayerId(int id);
    QString displayName() const;
    void setDisplayName(const QString& name);

    // ==================== 武将管理 ====================

    void setCharacter(Character* character);
    Character* character() const;
    QString characterName() const;
    bool hasCharacter() const;

    // ==================== 体力管理 ====================

    int hp() const;
    void setHp(int value);
    int maxHp() const;           // 委托给 Character
    void damage(int value);
    void heal(int value);
    bool isAlive() const;
    bool isDying() const;
    void setDying(bool dying);
    bool isFullHp() const;
    double hpRatio() const;      // hp / maxHp（用于血条显示）
    int handCardLimit() const;   // = 当前体力值

    // ==================== 手牌管理 ====================

    const QList<Card*>& handCards() const;
    int handCardCount() const;
    bool hasCard(const Card* card) const;
    void addHandCard(Card* card);
    void removeHandCard(Card* card);
    bool hasHandCards() const;
    Card* getRandomHandCard() const;  // 随机获取一张手牌

    // ==================== 装备区（预留） ====================

    const QList<Card*>& equipCards() const;
    bool hasEquipCards() const;
    void addEquipCard(Card* card);
    void removeEquipCard(Card* card);

    // ==================== 判定区（预留） ====================

    const QList<Card*>& judgmentCards() const;
    bool hasJudgmentCards() const;
    void addJudgmentCard(Card* card);
    void removeJudgmentCard(Card* card);

    // ==================== 回合状态 ====================

    void resetTurnState();              // 回合开始时调用
    bool hasUsedKillThisTurn() const;
    void setUsedKillThisTurn(bool used);
    bool isWineEnhanced() const;
    void setWineEnhanced(bool enhanced);

    // ==================== 工具 ====================

    /// 获取所有可被选中的牌（手牌 + 装备区）
    QList<Card*> allSelectableCards() const;

signals:
    void hpChanged(int newHp);
    void maxHpChanged(int newMaxHp);
    void dying(Player* self);
    void died(Player* self);
    void revived(Player* self);       // 从濒死救回
    void handCardAdded(Card* card);
    void handCardRemoved(Card* card);
    void handCardsChanged();          // 批量手牌变化
    void characterChanged(Character* character);
    void stateChanged();              // 通用刷新

private:
    int m_playerId = -1;
    QString m_displayName;
    Character* m_character = nullptr;

    int m_hp = 0;
    bool m_dying = false;

    QList<Card*> m_handCards;
    QList<Card*> m_equipCards;
    QList<Card*> m_judgmentCards;

    bool m_usedKillThisTurn = false;
    bool m_wineEnhanced = false;
};

#endif // PLAYER_H
```

---

## 9. 牌堆管理 CardManager

```cpp
#ifndef CARDMANAGER_H
#define CARDMANAGER_H

#include <QObject>
#include <QList>
#include "CommonTypes.h"

class Card;

class CardManager : public QObject {
    Q_OBJECT
public:
    explicit CardManager(QObject* parent = nullptr);
    ~CardManager() override;

    // ==================== 初始化与洗牌 ====================

    /// 创建所有卡牌并洗牌（简化版牌堆，共约 48 张）
    /// 杀×15 闪×10 桃×6 酒×3
    /// 过河拆桥×3 顺手牵羊×3 无中生有×3
    /// 南蛮入侵×2 万箭齐发×2 桃园结义×1
    void initialize();

    /// 洗牌（Fisher-Yates）
    void shuffle();

    // ==================== 摸牌 ====================

    /// 摸一张（牌堆空时自动回收弃牌堆）
    Card* drawCard();

    /// 摸多张
    QList<Card*> drawCards(int count);

    /// 剩余张数
    int remainingCount() const;

    /// 总张数
    int totalCardCount() const;

    // ==================== 弃牌 ====================

    void discard(Card* card);
    void discardMultiple(const QList<Card*>& cards);
    int discardPileCount() const;

    // ==================== 回收 ====================

    void reshuffleDiscardPile();

    // ==================== 查找 ====================

    Card* findCardById(int id) const;

signals:
    void drawPileEmpty();
    void reshuffled();
    void cardDiscarded(Card* card);

private:
    Card* createCard(CardType type, CardSuit suit, int number);
    CardSuit randomSuit() const;

    QList<Card*> m_drawPile;
    QList<Card*> m_discardPile;
    QList<Card*> m_allCards;  // 全量卡牌（管理生命周期）
    int m_nextCardId = 1;
};

#endif // CARDMANAGER_H
```

---

## 10. 规则引擎 GameRule

> 纯函数命名空间，不持有状态，所有状态通过 `GameState*` 传入。

```cpp
#ifndef GAMERULE_H
#define GAMERULE_H

class GameState;
class Player;
class Card;

namespace GameRule {

    // ==================== 常量 ====================

    constexpr int INITIAL_HAND_COUNT = 4;  // 初始手牌数
    constexpr int DRAW_PHASE_COUNT = 2;    // 摸牌阶段摸牌数

    // ==================== 规则判断 ====================

    /// 玩家当前是否可以使用【杀】（含张飞咆哮判定）
    bool canPlayKill(const GameState* state, const Player* player);

    /// 玩家是否可以使用某张牌（通用入口）
    bool canPlayCard(const GameState* state, const Player* player, const Card* card);

    /// 手牌上限（= 当前体力值）
    int handLimit(const Player* player);

    /// 是否有闪可响应
    bool hasDodgeToRespond(const Player* player);

    /// 是否有杀可响应（南蛮入侵）
    bool hasKillToRespond(const Player* player);

    /// 是否有桃可救濒死玩家
    bool hasPeachToSave(const Player* player, const Player* dyingPlayer);

    // ==================== 卡牌执行 ====================

    void executeKill(GameState* state, Player* user, Player* target);
    void handleKillResponse(GameState* state, Player* responder, Card* dodgeCard);
    void executePeach(GameState* state, Player* user, Player* target);
    void executeWine(GameState* state, Player* user);

    // ==================== 锦囊执行 ====================

    void executeDismantle(GameState* state, Player* user, Player* target);
    void executeSteal(GameState* state, Player* user, Player* target);
    void executeBountiful(GameState* state, Player* user);
    void executeBarbarianInvasion(GameState* state, Player* user);
    void executeVolley(GameState* state, Player* user);
    void executePeachGarden(GameState* state);

    // ==================== AOE 响应处理 ====================

    void handleAoeKillResponse(GameState* state, Player* responder, Card* killCard);
    void handleAoeDodgeResponse(GameState* state, Player* responder, Card* dodgeCard);
    void handleAoeSkipResponse(GameState* state, Player* responder);

    // ==================== 伤害与濒死 ====================

    void dealDamage(GameState* state, Player* target, int value, Player* source);
    void startDyingProcess(GameState* state, Player* dyingPlayer);
    bool handleDyingPeach(GameState* state, Player* dyingPlayer, Player* peachUser, Card* peachCard);
    void skipDyingResponse(GameState* state, Player* dyingPlayer);
    void checkDeath(GameState* state, Player* player);
    void checkGameOver(GameState* state);

    // ==================== 弃牌阶段 ====================

    /// 返回需要弃置的张数（0 则不需要）
    int getDiscardCount(const Player* player);

} // namespace GameRule

#endif // GAMERULE_H
```

---

## 11. 文件结构总览（含新增）

```
src/Model/
├── CommonTypes.h      ← 新增：所有枚举与前向声明
├── Card.h / .cpp      ← 卡牌基类 + 4基本牌 + 6锦囊牌
├── Character.h / .cpp ← 武将基类 + 4具体武将
├── Player.h / .cpp    ← 玩家
├── CardManager.h / .cpp  ← 牌堆管理
├── GameState.h / .cpp ← 新增：游戏状态（含 PendingActionInfo）
└── GameRule.h / .cpp  ← 规则引擎
```

---

## 12. 数据流示例

### 杀 → 闪 流程

```
View (C)            ViewModel (B)               Model (A)
  │                     │                          │
  │ 点击"杀"选目标      │                          │
  │────────────────────>│                          │
  │                     │ playCard(kill, B)        │
  │                     │─────────────────────────>│
  │                     │                          │ KillCard::execute
  │                     │                          │ → GameRule::executeKill
  │                     │                          │ → setPendingAction(B需出闪)
  │                     │     pendingActionCreated │
  │                     │<─────────────────────────│
  │ 提示"请出闪"        │     actionRequired       │
  │<────────────────────│                          │
  │                     │                          │
  │ 玩家B点"出闪"       │                          │
  │────────────────────>│                          │
  │                     │ respondCard(闪)          │
  │                     │─────────────────────────>│
  │                     │                          │ GameRule::handleKillResponse
  │                     │                          │ → clearPendingAction
  │                     │     pendingActionCleared │
  │                     │<─────────────────────────│
  │ 刷新UI              │     hpChanged(不变)      │
  │<────────────────────│                          │
```

### 南蛮入侵 → 不出杀 → 受伤 → 濒死流程

```
View (C)            ViewModel (B)               Model (A)
  │                     │                          │
  │ 使用南蛮入侵         │ playCard(南蛮, B)        │
  │────────────────────>│─────────────────────────>│
  │                     │                          │ → setPendingAction(B需出杀)
  │ 提示"请出杀响应"     │     pendingActionCreated │
  │<────────────────────│<─────────────────────────│
  │                     │                          │
  │ B跳过(不出杀)        │ skipAction()             │
  │────────────────────>│─────────────────────────>│
  │                     │                          │ GameRule::handleAoeSkip
  │                     │                          │ → dealDamage(B, 1, A)
  │                     │                          │ → B: hp→0, dying=true
  │                     │                          │ → startDyingProcess
  │                     │                          │ → setPendingAction(需出桃)
  │                     │     hpChanged(0) / dying │
  │                     │<─────────────────────────│
  │ 提示"请用桃救"       │     pendingActionCreated │
  │<────────────────────│<─────────────────────────│
  │                     │                          │
  │ A选择不出桃          │ skipAction()             │
  │────────────────────>│─────────────────────────>│
  │                     │                          │ skipDyingResponse
  │                     │                          │ → B死亡
  │                     │                          │ → checkGameOver
  │                     │     died / gameOver(A)   │
  │                     │<─────────────────────────│
  │ 显示"游戏结束"       │                          │
  │<────────────────────│                          │
```

---

## 13. 接口依赖关系图

```
                     ┌──────────────┐
                     │  CommonTypes │  (枚举、前向声明)
                     └──────┬───────┘
                            │
           ┌────────────────┼─────────────────┐
           │                │                 │
    ┌──────┴──────┐  ┌─────┴──────┐   ┌──────┴───────┐
    │    Card     │  │ Character  │   │   Player     │
    │  (基类+子类) │  │ (基类+子类) │   │              │
    └──────┬──────┘  └─────┬──────┘   └──────┬───────┘
           │               │                 │
           └───────────────┼─────────────────┘
                           │
                   ┌───────┴────────┐
                   │   GameState    │  (聚合所有 Model 对象)
                   └───────┬────────┘
                           │
                   ┌───────┴────────┐
                   │  CardManager   │
                   └────────────────┘

    GameRule 命名空间：无状态，所有函数接收 GameState*
```

---

## 14. 关键设计决策

### 14.1 卡牌虚方法 vs switch

每个具体卡牌类重写 `canUse` / `canTarget` / `execute`，添加新卡牌时无需修改现有代码（开闭原则）。`GameRule` 中的函数作为底层规则实现，由 `Card::execute` 内部调用。

### 14.2 GameRule 是命名空间而非类

规则函数是纯逻辑，不持有状态。所有可变状态通过 `GameState*` 传入，便于测试和复用。

### 14.3 PendingAction 响应流程

1. 卡牌 `execute` 调用 `GameRule` 函数
2. 如需响应，`GameRule` 调用 `GameState::setPendingAction()`
3. ViewModel 监听 `pendingActionCreated`，展示 UI 提示
4. 用户操作后，ViewModel 调用对应 `GameRule::handle*Response()` 函数
5. 完成后调用 `GameState::clearPendingAction()`

### 14.4 内存管理

- **Card**：`CardManager::m_allCards` 持有全部卡牌生命周期
- **Character**：由 `Player` 持有
- **Player**：由 `GameState` 持有
- **GameState**：由 `GameViewModel` 持有
- 全部通过 QObject 父子关系管理析构顺序

### 14.5 初版简化说明

1. **判定区**：第一版不使用，延迟锦囊不实现
2. **装备区**：预留接口，第一版不实现具体装备牌
3. **距离**：双人版忽略所有距离限制
4. **武将技能**：简化实现，仅保留核心效果
5. **牌堆**：标准牌堆 108 张，简化版约 48 张
