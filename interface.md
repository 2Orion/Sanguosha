# Model 层公开接口定义

> 本文档描述当前代码中 Model 层及其共享类型的公开接口，作为 ViewModel 层调用 Model 的契约。
> 最后全量核对日期：2026-07-16。
> 接口代码块以仓库中的头文件为准；private 成员不在本文档中重复列出。

## 目录

1. [共享枚举与类型](#1-共享枚举与类型)
2. [游戏状态 GameState](#2-游戏状态-gamestate)
3. [卡牌 Card](#3-卡牌-card)
4. [武将 Character](#4-武将-character)
5. [玩家 Player](#5-玩家-player)
6. [牌堆管理 CardManager](#6-牌堆管理-cardmanager)
7. [规则命名空间 GameRule](#7-规则命名空间-gamerule)
8. [Model 与 ViewModel 的边界](#8-model-与-viewmodel-的边界)
9. [文件结构](#9-文件结构)
10. [所有权与生命周期](#10-所有权与生命周期)

---

## 1. 共享枚举与类型

### 1.1 规范定义位置

共享枚举的规范定义在 src/Common/CommonTypes.h：

~~~cpp
#ifndef CORE_COMMONTYPES_H
#define CORE_COMMONTYPES_H

enum class CardSuit {
    Spade,
    Heart,
    Club,
    Diamond
};

enum class CardColor {
    Red,
    Black
};

enum class CardCategory {
    Basic,
    Strategy,
    Equipment
};

enum class EquipSlot {
    Weapon,
    Armor,
    Mount
};

enum class CardType {
    Kill,
    Dodge,
    Peach,
    Wine,
    Dismantle,
    Steal,
    Bountiful,
    BarbarianInvasion,
    Volley,
    PeachGarden,
    Duel,
    Lightning,
    Nullify,
    Borrow,
    Harvest,
    Happy,
    Famine,
    Crossbow,
    QinglongBlade,
    ZhangbaSnake,
    KylinBow,
    QinggangSword,
    IceSword,
    DualSword,
    EightDiagrams,
    BenevolentShield,
};

enum class PhaseType {
    Prepare,
    Judge,
    Draw,
    Play,
    Discard,
    End,
};

enum class CardArea {
    Hand,
    Equipment,
    Judgment,
    Pending
};

enum class GameEvent {
    OnDamage,
    OnDamageCaused,
    OnCardPlayed,
    OnCardResponded,
    OnDrawPhase,
    OnTurnStart,
    OnDying,
};

enum class ActionResult {
    Completed,
    RequiresDodge,
    RequiresKill,
    RequiresPeach,
    RequiresDiscard,
};

#endif // CORE_COMMONTYPES_H
~~~

src/Model/CommonTypes.h 不是枚举的第二份定义，而是 Model 层的转发头：

~~~cpp
#ifndef MODEL_COMMONTYPES_H
#define MODEL_COMMONTYPES_H

#include "Common/CommonTypes.h"

class Player;
class Card;
class GameState;
class CardManager;
class Character;

#endif // MODEL_COMMONTYPES_H
~~~

### 1.2 枚举用途

| 类型 | 用途 |
|------|------|
| CardSuit | 卡牌花色 |
| CardColor | 卡牌颜色，由花色派生 |
| CardCategory | 基本牌、锦囊牌、装备牌 |
| EquipSlot | 装备槽位：武器、防具，坐骑预留 |
| CardType | 具体卡牌类型 |
| PhaseType | 回合阶段 |
| CardArea | 卡牌所在区域；装备区已使用，判定区结算仍为预留 |
| GameEvent | 武将技能触发事件 |
| ActionResult | 卡牌执行后是否需要后续响应 |

---

## 2. 游戏状态 GameState

### 2.1 PendingActionInfo

PendingActionInfo 用于描述当前等待玩家响应的动作。它属于 Model 内部类型，包含 Model 对象指针，不应直接传给 View。

~~~cpp
#include <string>
#include <vector>

struct PendingActionInfo {
    Player* source = nullptr;
    Player* target = nullptr;
    Card* sourceCard = nullptr;
    CardType requiredCardType = CardType::Kill;
    std::string description;
    bool canSkip = false;
    std::vector<Player*> remainingTargets;
    Player* continuationSource = nullptr;
    CardType continuationCardType = CardType::Kill;
    std::vector<Player*> continuationTargets;
};
~~~

字段含义：

| 字段 | 含义 |
|------|------|
| source | 动作来源玩家；濒死动作中是当前救援者/响应者 |
| target | 普通响应动作中的响应目标；濒死动作中是濒死玩家 |
| sourceCard | 触发动作的卡牌，可为空；决斗响应链保留原【决斗】指针，供规则/ViewModel 与其他“需要出杀”的动作区分 |
| requiredCardType | 当前要求的响应牌类型 |
| description | Model 内部使用的 UTF-8 文本 |
| canSkip | 当前响应是否允许跳过 |
| remainingTargets | AOE 动作中尚未处理的目标 |
| continuationSource | AOE 目标濒死获救后恢复响应链的来源玩家 |
| continuationCardType | 续接 AOE 所需的响应牌类型 |
| continuationTargets | AOE 目标濒死获救后仍待处理的目标 |

### 2.2 公共接口

~~~cpp
#include <QObject>
#include <string>
#include <vector>
#include "CommonTypes.h"

class Player;
class CardManager;
class Card;

class GameState : public QObject {
    Q_OBJECT
public:
    explicit GameState(QObject* parent = nullptr);
    ~GameState() override = default;

    PhaseType currentPhase() const;
    void setCurrentPhase(PhaseType phase);

    int currentPlayerIndex() const;
    void setCurrentPlayerIndex(int index);
    Player* currentPlayer() const;
    Player* player(int index) const;
    int playerCount() const;
    void addPlayer(Player* player);
    std::vector<Player*> alivePlayers() const;
    std::vector<Player*> allPlayers() const;

    int turnCount() const;
    void incrementTurn();

    bool hasPendingAction() const;
    const PendingActionInfo& pendingActionInfo() const;
    void setPendingAction(const PendingActionInfo& info);
    void clearPendingAction();

    CardManager* cardManager() const;
    void setCardManager(CardManager* mgr);

    bool isGameOver() const;
    Player* winner() const;
    void setGameOver(Player* winnerPlayer);

signals:
    void phaseChanged(PhaseType phase);
    void currentPlayerChanged(int playerIndex);
    void pendingActionCreated(const PendingActionInfo& info);
    void pendingActionCleared();
    void gameOver(int winnerId);
    void stateRefreshed();
};
~~~

行为约定：

- 默认阶段是 PhaseType::Prepare，默认当前玩家索引为 0。
- setCurrentPlayerIndex 只接受当前玩家列表范围内的索引。
- addPlayer 忽略空指针和重复指针；它只保存指针，不负责删除玩家。
- alivePlayers 返回 hp() > 0 的玩家；返回值是新的 std::vector。
- setPendingAction 会保存动作并发出 pendingActionCreated。
- clearPendingAction 会清除标志、重置动作结构并发出 pendingActionCleared。
- setGameOver 将状态设为结束，并以获胜玩家的 playerId 发出 gameOver；平局或没有获胜者时传出 -1。
- cardManager 是非拥有指针。调用方负责保证其生命周期。
- stateRefreshed 是预留的通用刷新信号；当前 GameState.cpp 不会自动发出它。

---

## 3. 卡牌 Card

### 3.1 基类

Card 是普通 C++ 多态基类，不继承 QObject，没有 Q_OBJECT，也没有 Qt 父对象参数。文本使用 std::string，目标列表使用 std::vector。

~~~cpp
#include <string>
#include <vector>
#include "CommonTypes.h"

class GameState;
class Player;

class Card {
public:
    Card(CardType type, CardSuit suit, int number);
    virtual ~Card() = default;

    int id() const;
    CardType cardType() const;
    CardSuit suit() const;
    int number() const;
    CardColor color() const;
    std::string cardName() const;
    std::string description() const;

    CardCategory category() const;
    bool isBasic() const;
    bool isStrategy() const;
    bool isEquipment() const;
    bool isRed() const;
    bool isBlack() const;

    virtual bool canUse(const GameState* state, const Player* user) const;
    virtual bool canTarget(const GameState* state,
                           const Player* user,
                           const Player* target) const;
    virtual std::vector<Player*> getValidTargets(const GameState* state,
                                                  const Player* user) const;
    virtual ActionResult execute(GameState* state,
                                  Player* user,
                                  const std::vector<Player*>& targets);

    static std::string cardTypeName(CardType type);
    std::string suitSymbol() const;
    std::string numberString() const;

protected:
    void setCardName(const std::string& name);
    void setDescription(const std::string& desc);
};
~~~

基类默认行为：

- 卡牌 ID 由 Card::s_nextCardId 全局递增生成。
- 构造时卡牌名称默认为 cardTypeName(type)。
- color() 将红桃、方块判为红色，其余花色判为黑色。
- 基类 canUse、canTarget 默认返回 false。
- 基类 getValidTargets 返回空列表。
- 基类 execute 返回 ActionResult::Completed。
- numberString 将 1/11/12/13 显示为 A/J/Q/K，其他点数显示数字。

### 3.2 代表性基本牌与锦囊牌声明

> 下方代码块展示核心类的接口形状，不重复整个 `Card.h`。当前实际子类清单见代码块后的表格。

~~~cpp
class KillCard : public Card {
public:
    KillCard(CardSuit suit, int number);
    ~KillCard() override = default;

    bool canUse(const GameState* state, const Player* user) const override;
    bool canTarget(const GameState* state,
                   const Player* user,
                   const Player* target) const override;
    std::vector<Player*> getValidTargets(const GameState* state,
                                          const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const std::vector<Player*>& targets) override;
};

class DodgeCard : public Card {
public:
    DodgeCard(CardSuit suit, int number);
    ~DodgeCard() override = default;

    bool canUse(const GameState* state, const Player* user) const override;
};

class PeachCard : public Card {
public:
    PeachCard(CardSuit suit, int number);
    ~PeachCard() override = default;

    bool canUse(const GameState* state, const Player* user) const override;
    bool canTarget(const GameState* state,
                   const Player* user,
                   const Player* target) const override;
    std::vector<Player*> getValidTargets(const GameState* state,
                                          const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const std::vector<Player*>& targets) override;
};

class WineCard : public Card {
public:
    WineCard(CardSuit suit, int number);
    ~WineCard() override = default;

    bool canUse(const GameState* state, const Player* user) const override;
    std::vector<Player*> getValidTargets(const GameState* state,
                                          const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const std::vector<Player*>& targets) override;
};

class StrategyCard : public Card {
public:
    StrategyCard(CardType type, CardSuit suit, int number);
    ~StrategyCard() override = default;

    bool canUse(const GameState* state, const Player* user) const override;
};

class DismantleCard : public StrategyCard {
public:
    DismantleCard(CardSuit suit, int number);

    bool canTarget(const GameState* state,
                   const Player* user,
                   const Player* target) const override;
    std::vector<Player*> getValidTargets(const GameState* state,
                                          const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const std::vector<Player*>& targets) override;
};

class StealCard : public StrategyCard {
public:
    StealCard(CardSuit suit, int number);

    bool canTarget(const GameState* state,
                   const Player* user,
                   const Player* target) const override;
    std::vector<Player*> getValidTargets(const GameState* state,
                                          const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const std::vector<Player*>& targets) override;
};

class BountifulCard : public StrategyCard {
public:
    BountifulCard(CardSuit suit, int number);

    std::vector<Player*> getValidTargets(const GameState* state,
                                          const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const std::vector<Player*>& targets) override;
};

class BarbarianCard : public StrategyCard {
public:
    BarbarianCard(CardSuit suit, int number);

    bool canTarget(const GameState* state,
                   const Player* user,
                   const Player* target) const override;
    std::vector<Player*> getValidTargets(const GameState* state,
                                          const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const std::vector<Player*>& targets) override;
};

class VolleyCard : public StrategyCard {
public:
    VolleyCard(CardSuit suit, int number);

    bool canTarget(const GameState* state,
                   const Player* user,
                   const Player* target) const override;
    std::vector<Player*> getValidTargets(const GameState* state,
                                          const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const std::vector<Player*>& targets) override;
};

class PeachGardenCard : public StrategyCard {
public:
    PeachGardenCard(CardSuit suit, int number);

    std::vector<Player*> getValidTargets(const GameState* state,
                                          const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const std::vector<Player*>& targets) override;
};

class DuelCard : public StrategyCard {
public:
    DuelCard(CardSuit suit, int number);

    bool canTarget(const GameState* state,
                   const Player* user,
                   const Player* target) const override;
    std::vector<Player*> getValidTargets(const GameState* state,
                                          const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const std::vector<Player*>& targets) override;
};
~~~

| 分类 | 当前子类 |
|------|------------|
| 基本牌 | KillCard、DodgeCard、PeachCard、WineCard |
| 锦囊牌 | DismantleCard、StealCard、BountifulCard、BarbarianCard、VolleyCard、PeachGardenCard、DuelCard、LightningCard、NullifyCard、BorrowCard、HarvestCard、HappyCard、FamineCard |
| 装备牌 | EquipmentCard、CrossbowCard、QinglongBladeCard、ZhangbaSnakeCard、KylinBowCard、QinggangSwordCard、IceSwordCard、DualSwordCard、EightDiagramsCard、BenevolentShieldCard |

### 3.3 卡牌行为

| 卡牌 | 使用与目标判断 | execute 结果和效果 |
|------|----------------|-------------------|
| KillCard | 仅出牌阶段、玩家存活且满足 GameRule::canPlayKill；目标必须是其他存活玩家 | 调用 GameRule::executeKill，返回 RequiresDodge |
| DodgeCard | canUse 始终返回 false | 只通过响应流程处理 |
| PeachCard | 出牌阶段可对自己使用，且自己未满体力 | 调用 executePeach，回复 1 点体力，返回 Completed |
| WineCard | 出牌阶段、玩家存活且当前未处于酒强化状态 | 调用 executeWine，强化下一次杀，返回 Completed |
| StrategyCard | 仅出牌阶段、玩家存活且至少存在一个合法目标 | 作为锦囊牌基类；无合法目标时不可使用 |
| DismantleCard | 目标是其他存活且有手牌或装备的玩家 | 弃置目标一张牌 |
| StealCard | 目标条件同过河拆桥 | 将目标一张手牌或装备牌移给使用者 |
| BountifulCard | 合法目标列表为使用者自己 | 摸两张牌 |
| BarbarianCard | 合法目标为除使用者外的所有存活玩家 | 建立依次要求出杀的响应链，返回 RequiresKill |
| VolleyCard | 合法目标为除使用者外的所有存活玩家 | 建立依次要求出闪的响应链，返回 RequiresDodge |
| PeachGardenCard | 合法目标列表为所有存活玩家 | 所有未满体力的玩家回复 1 点体力 |
| DuelCard | 目标必须是其他存活玩家 | 记录原决斗卡并要求目标先出杀，返回 RequiresKill；双方交替出杀，未出者受到对方造成的 1 点伤害 |
| LightningCard | 目标为使用者自己 | 当前仅调用判定区骨架；闪电判定、伤害和转移未实现 |
| NullifyCard | 存活玩家可进入 canUse，无显式目标 | `executeNullify`/`checkNullifyChain` 仍是空实现，不会抵消锦囊 |
| BorrowCard | 目标为其他存活且有装备的玩家 | 建立一次要求出杀的简化待定动作，未实现完整借刀目标选择/交武器规则 |
| HarvestCard | 合法目标为所有存活玩家 | 简化为使用者获得展示牌的第一张，其余弃置 |
| HappyCard / FamineCard | 目标为其他存活玩家 | 当前只有目标判断和结算骨架，未放入真实判定牌，也未跳过阶段 |
| EquipmentCard | 出牌阶段对自己使用 | 装入对应槽位并替换旧装备；诸葛连弩的无限出杀已接入，其他专属触发效果多数未接入主结算链 |

卡牌的 execute 通常只处理传入目标列表的第一个目标；AOE 卡牌的目标链由 GameRule 和 PendingActionInfo::remainingTargets 管理。

---

## 4. 武将 Character

### 4.1 基类接口

Character 仍然是 QObject，但名称、技能名称和技能描述在 Model 内部使用 std::string。只有技能信号使用 QString。

~~~cpp
#include <string>
#include <QObject>
#include "CommonTypes.h"

class GameState;
class Player;
class Card;

class Character : public QObject {
    Q_OBJECT
public:
    Character(const std::string& name, int maxHp,
              const std::string& skillName, const std::string& skillDesc,
              QObject* parent = nullptr);
    ~Character() override = default;

    std::string characterName() const;
    int maxHp() const;
    std::string skillName() const;
    std::string skillDescription() const;

    virtual bool hasSkill() const;
    virtual bool triggerCondition(GameEvent event,
                                  const GameState* state,
                                  const Player* self) const;
    virtual void triggerSkill(GameState* state, Player* self);
    virtual CardType skillTransformCard(const Card* card) const;
    virtual int onDrawPhaseBonus() const;
    virtual bool requireExtraDodge() const;
    virtual bool canRedirectKill() const;
    virtual Player* getRedirectTarget(GameState* state,
                                      Player* self,
                                      Player* attacker) const;
    virtual bool canDiscardAndDraw() const;

signals:
    void skillTriggered(const QString& skillName);
};
~~~

默认行为：

- hasSkill() 返回 true。
- triggerCondition() 返回 false。
- triggerSkill() 不执行动作。
- skillTransformCard() 返回原卡牌类型。
- onDrawPhaseBonus() 返回 0；requireExtraDodge()、canRedirectKill()、canDiscardAndDraw() 返回 false。
- getRedirectTarget() 返回 nullptr。

### 4.2 具体武将接口

~~~cpp
class CaoCao : public Character {
    Q_OBJECT
public:
    CaoCao(QObject* parent = nullptr);
    bool hasSkill() const override;
    bool triggerCondition(GameEvent event,
                          const GameState* state,
                          const Player* self) const override;
    void triggerSkill(GameState* state, Player* self) override;
};

class GuanYu : public Character {
    Q_OBJECT
public:
    GuanYu(QObject* parent = nullptr);
    bool hasSkill() const override;
    CardType skillTransformCard(const Card* card) const override;
};

class ZhangFei : public Character {
    Q_OBJECT
public:
    ZhangFei(QObject* parent = nullptr);
    bool hasSkill() const override;
    bool triggerCondition(GameEvent event,
                          const GameState* state,
                          const Player* self) const override;
    void triggerSkill(GameState* state, Player* self) override;
};

class ZhaoYun : public Character {
    Q_OBJECT
public:
    ZhaoYun(QObject* parent = nullptr);
    bool hasSkill() const override;
    CardType skillTransformCard(const Card* card) const override;
};

class SunQuan : public Character {
public:
    SunQuan(QObject* parent = nullptr);
    bool hasSkill() const override;
    bool canDiscardAndDraw() const override;
};

class ZhouYu : public Character {
public:
    ZhouYu(QObject* parent = nullptr);
    bool hasSkill() const override;
    int onDrawPhaseBonus() const override;
};

class LvBu : public Character {
public:
    LvBu(QObject* parent = nullptr);
    bool hasSkill() const override;
    bool requireExtraDodge() const override;
};

class DaQiao : public Character {
public:
    DaQiao(QObject* parent = nullptr);
    bool hasSkill() const override;
    bool canRedirectKill() const override;
    Player* getRedirectTarget(GameState* state,
                              Player* self,
                              Player* attacker) const override;
};

class SiMaYi : public Character {
public:
    SiMaYi(QObject* parent = nullptr);
    bool hasSkill() const override;
    bool triggerCondition(GameEvent event,
                          const GameState* state,
                          const Player* self) const override;
    void triggerSkill(GameState* state, Player* self) override;
};
~~~

| 武将 | 当前实现 |
|------|----------|
| CaoCao | OnDamage 满足触发条件，触发后从牌堆摸一张牌 |
| GuanYu | 红色牌通过 skillTransformCard 转为 CardType::Kill |
| ZhangFei | GameRule::canPlayKill 对张飞放宽本回合杀次数；自身 triggerCondition 当前返回 false |
| ZhaoYun | Kill 与 Dodge 互相转换，其他牌保持原类型 |
| SunQuan | canDiscardAndDraw 返回 true，但当前出牌/ViewModel 流程没有制衡命令入口 |
| ZhouYu | onDrawPhaseBonus 返回 1，GameViewModel 摸牌阶段已接入 |
| LvBu | requireExtraDodge 返回 true，但当前杀的响应结算尚未读取该值 |
| DaQiao | 流离查询接口已存在，但当前杀的目标结算尚未调用 |
| SiMaYi | OnDamage 触发框架已接入；当前从待定动作的 source 推断伤害来源，并获得其一张随机手牌 |

---

## 5. 玩家 Player

### 5.1 公共接口

~~~cpp
#include <vector>
#include <QObject>
#include "CommonTypes.h"

class Card;
class EquipmentCard;
class Character;
class GameState;

class Player : public QObject {
    Q_OBJECT
public:
    explicit Player(QObject* parent = nullptr);
    ~Player() override;

    int playerId() const;
    void setPlayerId(int id);
    QString displayName() const;
    void setDisplayName(const QString& name);

    void setCharacter(Character* character);
    Character* character() const;
    QString characterName() const;
    bool hasCharacter() const;

    int hp() const;
    void setHp(int value);
    int maxHp() const;
    void damage(int value);
    void heal(int value);
    bool isAlive() const;
    bool isDying() const;
    void setDying(bool dying);
    void markDead();
    bool isFullHp() const;
    double hpRatio() const;
    int handCardLimit() const;

    const std::vector<Card*>& handCards() const;
    int handCardCount() const;
    bool hasCard(const Card* card) const;
    void addHandCard(Card* card);
    void removeHandCard(Card* card);
    bool hasHandCards() const;
    Card* getRandomHandCard() const;

    void equipCard(EquipmentCard* card);
    void unequipSlot(EquipSlot slot);
    EquipmentCard* equippedAt(EquipSlot slot) const;
    int attackRange() const;
    bool hasArmor() const;
    bool hasCrossbow() const;

    const std::vector<Card*>& equipCards() const; // 兼容接口
    bool hasEquipCards() const;                   // 兼容接口
    void addEquipCard(Card* card);                // 兼容接口
    void removeEquipCard(Card* card);             // 兼容接口

    const std::vector<Card*>& judgmentCards() const;
    bool hasJudgmentCards() const;
    void addJudgmentCard(Card* card);
    void removeJudgmentCard(Card* card);

    void resetTurnState();
    bool hasUsedKillThisTurn() const;
    void setUsedKillThisTurn(bool used);
    bool isWineEnhanced() const;
    void setWineEnhanced(bool enhanced);

    std::vector<Card*> allSelectableCards() const;

signals:
    void hpChanged(int hp);
    void maxHpChanged(int maxHp);
    void dying(int playerId);
    void died(int playerId);
    void revived(int playerId);
    void handCardAdded(int cardId);
    void handCardRemoved(int cardId);
    void handCardsChanged();
    void characterChanged(const QString& charName);
    void stateChanged();
    void equipmentChanged(EquipSlot slot);
};
~~~

### 5.2 行为约定

- setCharacter 保存武将指针，并将当前体力初始化为武将最大体力。
- Player 的名称和字符展示字段使用 QString；Character 的内部名称字段使用 std::string。
- maxHp() 没有武将时返回 0；有武将时返回武将的最大体力。
- setHp 将体力限制为不小于 0；damage 和 heal 只处理正数。
- isAlive() 等价于 hp() > 0；isFullHp() 等价于 hp() >= maxHp()。
- handCardLimit() 等于当前体力值的非负部分。
- 手牌和判定区使用 std::vector<Card*>；装备区内部按 EquipSlot 保存 EquipmentCard*，兼容接口 equipCards() 返回重建后的缓存列表。这些指针都不拥有卡牌。
- allSelectableCards() 返回手牌和装备区的合并列表，不包含判定区。
- setDying(true) 发出 dying(playerId)；从濒死状态恢复时发出 revived(playerId)。
- handCardAdded、handCardRemoved 的参数是卡牌 ID，不是 Card*。
- `markDead()` 负责一次性发出 died(playerId)；GameRule::checkDeath 在最终死亡时调用它，避免濒死阶段提前发出死亡信号。
- setCharacter、手牌变化和体力变化会触发对应的状态信号；装备变化同时发出 equipmentChanged(slot) 和 stateChanged()。

---

## 6. 牌堆管理 CardManager

### 6.1 公共接口

~~~cpp
#include <vector>
#include <QObject>
#include "CommonTypes.h"

class Card;

class CardManager : public QObject {
    Q_OBJECT
public:
    explicit CardManager(QObject* parent = nullptr);
    ~CardManager() override;

    void initialize();
    void shuffle();

    Card* drawCard();
    std::vector<Card*> drawCards(int count);
    int remainingCount() const;
    int totalCardCount() const;

    void discard(Card* card);
    void discardMultiple(const std::vector<Card*>& cards);
    int discardPileCount() const;

    void reshuffleDiscardPile();

    Card* findCardById(int id) const;

    const std::vector<Card*>& getDrawPile() const;

signals:
    void drawPileEmpty();
    void reshuffled();
    void cardDiscarded(int cardId);
};
~~~

### 6.2 行为约定

initialize() 创建并洗牌一副 101 张的牌堆：

| 卡牌 | 数量 |
|------|------|
| 杀 | 30 |
| 闪 | 15 |
| 桃 | 8 |
| 酒 | 5 |
| 过河拆桥 | 3 |
| 顺手牵羊 | 3 |
| 无中生有 | 3 |
| 南蛮入侵 | 3 |
| 万箭齐发 | 3 |
| 桃园结义 | 1 |
| 决斗 | 3 |
| 借刀杀人 | 2 |
| 五谷丰登 | 2 |
| 无懈可击 | 3 |
| 乐不思蜀 | 3 |
| 兵粮寸断 | 2 |
| 闪电 | 1 |
| 诸葛连弩 | 2 |
| 青龙偃月刀 | 1 |
| 丈八蛇矛 | 1 |
| 麒麟弓 | 1 |
| 青釭剑 | 1 |
| 寒冰剑 | 1 |
| 雌雄双股剑 | 1 |
| 八卦阵 | 2 |
| 仁王盾 | 1 |

- drawCard() 从牌堆取一张；牌堆为空时会先尝试回收弃牌堆并洗牌。
- 牌堆和弃牌堆都为空时，drawCard() 返回 nullptr 并发出 drawPileEmpty()。
- drawCards(count) 最多返回 count 张；牌堆无法继续提供卡牌时提前结束。
- discard 和 discardMultiple 只负责把卡牌放入弃牌堆，不会自动从玩家区域移除卡牌。
- 卡牌进入弃牌堆时发出 cardDiscarded(cardId)。
- reshuffleDiscardPile() 将弃牌堆全部放回牌堆、清空弃牌堆、洗牌并发出 reshuffled()。
- getDrawPile() 返回内部牌堆的只读引用；调用方不能通过该引用修改容器，但必须保证 CardManager 不被销毁。
- CardManager 通过 m_allCards 管理所有已创建卡牌，并在析构或重新初始化时删除它们。
- 卡牌 ID 由 Card 的静态计数器生成，不由 CardManager 单独维护。

---

## 7. 规则命名空间 GameRule

GameRule 是不保存自身状态的命名空间。函数通过 GameState*、Player* 和 Card* 读取或修改 Model 状态。调用方应在进入规则函数前保证指针有效；多数函数对空指针直接返回。

### 7.1 完整声明

~~~cpp
namespace GameRule {
    constexpr int INITIAL_HAND_COUNT = 4;
    constexpr int DRAW_PHASE_COUNT = 2;

    bool canPlayKill(const GameState* state, const Player* player);
    bool canPlayCard(const GameState* state,
                     const Player* player,
                     const Card* card);

    int handLimit(const Player* player);
    bool hasDodgeToRespond(const Player* player);
    bool hasKillToRespond(const Player* player);
    bool hasPeachToSave(const Player* player,
                        const Player* dyingPlayer);

    void executeKill(GameState* state, Player* user, Player* target);
    void handleKillResponse(GameState* state,
                            Player* responder,
                            Card* dodgeCard);
    void executePeach(GameState* state, Player* user, Player* target);
    void executeWine(GameState* state, Player* user);

    int getAttackRange(const GameState* state, const Player* attacker);
    bool isInAttackRange(const GameState* state,
                         const Player* attacker,
                         const Player* target);
    bool armorEffectCheck(const GameState* state,
                          const Player* defender,
                          Card* attackCard);

    void executeDismantle(GameState* state, Player* user, Player* target);
    void executeSteal(GameState* state, Player* user, Player* target);
    void executeBountiful(GameState* state, Player* user);
    void executeBarbarianInvasion(GameState* state, Player* user);
    void executeVolley(GameState* state, Player* user);
    void executePeachGarden(GameState* state);

    void executeDuel(GameState* state,
                     Player* user,
                     Player* target,
                     Card* duelCard);
    void handleDuelResponse(GameState* state,
                            Player* responder,
                            Card* killCard);
    void executeLightning(GameState* state, Player* user, Player* target);
    void executeNullify(GameState* state, Player* user);
    void executeBorrow(GameState* state, Player* user, Player* target);
    void executeHarvest(GameState* state, Player* user);
    void executeHappy(GameState* state, Player* user, Player* target);
    void executeFamine(GameState* state, Player* user, Player* target);
    bool checkNullifyChain(GameState* state,
                           Card* targetCard,
                           Player* targetPlayer,
                           Player* sourcePlayer);

    void handleAoeKillResponse(GameState* state,
                               Player* responder,
                               Card* killCard);
    void handleAoeDodgeResponse(GameState* state,
                                Player* responder,
                                Card* dodgeCard);
    void handleAoeSkipResponse(GameState* state, Player* responder);

    void dealDamage(GameState* state,
                    Player* target,
                    int value,
                    Player* source);
    void startDyingProcess(GameState* state, Player* dyingPlayer);
    bool handleDyingPeach(GameState* state,
                          Player* dyingPlayer,
                          Player* peachUser,
                          Card* peachCard);
    void skipDyingResponse(GameState* state, Player* dyingPlayer);
    void checkDeath(GameState* state, Player* player);
    void checkGameOver(GameState* state);

    int getDiscardCount(const Player* player);
}
~~~

### 7.2 规则分组

| 分组 | 接口 | 说明 |
|------|------|------|
| 使用判断 | canPlayKill、canPlayCard | 判断是否允许出牌；张飞由 canPlayKill 放宽杀次数 |
| 响应判断 | hasDodgeToRespond、hasKillToRespond、hasPeachToSave | 检查手牌及武将转化后的响应能力 |
| 基本牌 | executeKill、handleKillResponse、executePeach、executeWine | 处理杀、闪、桃、酒的实际效果 |
| 装备与距离 | getAttackRange、isInAttackRange、armorEffectCheck | 计算武器攻击范围并判定防具效果 |
| 锦囊牌 | executeDismantle、executeSteal、executeBountiful、executeBarbarianInvasion、executeVolley、executePeachGarden、executeDuel、executeLightning、executeNullify、executeBorrow、executeHarvest、executeHappy、executeFamine、checkNullifyChain | 处理锦囊牌效果 |
| 决斗响应 | handleDuelResponse | 验证并弃置响应杀，交换下一位响应者；未出杀时结算伤害 |
| AOE 响应 | handleAoeKillResponse、handleAoeDodgeResponse、handleAoeSkipResponse | 处理南蛮入侵和万箭齐发的逐目标响应 |
| 伤害与濒死 | dealDamage、startDyingProcess、handleDyingPeach、skipDyingResponse、checkDeath、checkGameOver | 伤害、救援、死亡及终局判定 |
| 弃牌 | getDiscardCount | 返回手牌数超过体力上限的数量 |

行为要点：

- INITIAL_HAND_COUNT 为 4，DRAW_PHASE_COUNT 为 2。
- executeKill 会设置攻击者本回合已使用杀，并在 GameState 中创建要求出闪的待定动作。
- handleKillResponse 传入有效卡牌时会移除并弃置该牌；传入 nullptr 时对响应者造成伤害。攻击者有酒强化时伤害为 2，随后消耗强化状态。
- executeDuel 在待定动作中保留 duelCard；handleDuelResponse 在响应者出杀后交换 source/target，继续交替响应，传入 nullptr 时由 source 对当前 target 造成 1 点伤害并结束决斗。
- executeBarbarianInvasion 和 executeVolley 会按存活目标顺序创建响应链。
- AOE 响应处理完成当前目标后，会根据 remainingTargets 创建下一个待定动作；目标濒死获救时使用 continuationTargets 恢复响应链。
- dealDamage 会触发受伤武将技能；目标体力不大于 0 且尚未濒死时，会进入 startDyingProcess。
- hasPeachToSave 当前将桃和酒都视为可用于濒死救援的牌。
- handleDyingPeach 会校验当前待定动作、救援者、濒死者、牌主和牌型；桃或酒成功救回目标时返回 true 并清除待定动作。
- 所有存活玩家都跳过濒死救援后，checkDeath 会清除濒死状态并调用 checkGameOver。
- 仅剩一名存活玩家时，该玩家获胜；没有存活玩家时，游戏以 nullptr 获胜者结束。

---

## 8. Model 与 ViewModel 的边界

当前跨层结构如下：

~~~
Model 信号
    ↓
GameViewModel 读取 Model 指针，转换为值类型并完成拦截判断（自动跳过等）
    ↓
GameViewModel 信号 ──（SGSApp 建立的直连）──► View 槽

View 信号 ──（SGSApp 建立的直连）──► ActionViewModel / GameViewModel 的 public slots
    ↓
GameRule 和 Model
~~~

> `SGSApp`（原 `GameBootstrap`，第二次重构更名）是纯组合根：只在 `startLocalGame()` 里建立上述 connect，不参与转发或拦截；路由/拦截逻辑全部在 ViewModel 内部。

### 8.1 Model 信号的使用

| Model 信号 | ViewModel 处理方式 |
|------------|-------------------|
| GameState::phaseChanged(PhaseType) | 转发为 GameViewModel::phaseChanged(PhaseType) |
| GameState::pendingActionCreated(PendingActionInfo) | 转换为 PendingActionData，将玩家和卡牌指针转换为 ID；无响应牌时在 VM 内直接自动跳过，不发信号 |
| GameState::pendingActionCleared() | 转发为 ViewModel 的同名信号 |
| GameState::gameOver(int) | 转换并转发为 ViewModel 的 gameOver(int) |
| Player::handCardsChanged() | 读取卡牌对象，生成 CardList（QVector\<CardData\>） |
| Player::stateChanged() | 生成 PlayerData |
| Player::died(int) | 由 `GameRule::checkDeath()` 通过 `Player::markDead()` 发出，ViewModel 接收并记录死亡日志 |

### 8.2 跨 View 边界的值类型

View 不应接收 PendingActionInfo、Player*、Card* 等 Model 指针。跨越 View 边界的值类型位于 src/Common/：

- PendingActionData：来源/目标玩家 ID、来源卡牌 ID、响应类型、描述、可否跳过和剩余目标 ID。
- CardData（列表别名 CardList）：卡牌展示所需的 ID、类型、花色、点数、名称、描述、可选/可出/高亮状态，以及 isEquipment、equipSlot、attackRange。
- PlayerData：玩家 ID、名称、武将与技能、体力、手牌数量/上限、攻击范围和 equipCards 装备列表。

View/ViewModel 的完整连接表和 ViewModel 内的路由/拦截逻辑见 connection.md。本文件只定义 Model 的对象接口，不再描述 View 与 ViewModel 的直接调用流程。

---

## 9. 文件结构

~~~
src/
├── Common/
│   ├── CommonTypes.h       # 跨层共享枚举
│   ├── PendingActionData.h # 待定动作值类型
│   ├── CardData.h          # 卡牌展示值类型（含 CardList 别名）
│   └── PlayerData.h        # 玩家展示值类型
├── Model/
│   ├── CommonTypes.h       # CommonTypes.h 的 Model 转发头
│   ├── GameState.h/.cpp    # 游戏状态与 PendingActionInfo
│   ├── Card.h/.cpp         # Card 及基本牌、锦囊牌
│   ├── Character.h/.cpp    # Character 及九名武将
│   ├── Player.h/.cpp       # 玩家状态、区域和信号
│   ├── CardManager.h/.cpp  # 牌堆、弃牌堆和卡牌生命周期
│   └── GameRule.h/.cpp     # 无状态规则函数
├── ViewModel/
│   ├── GameViewModel.h/.cpp
│   └── ActionViewModel.h/.cpp
├── App/
│   ├── SGSApp.h/.cpp       # 主程序组合根，管理本地/联网入口
│   ├── ServerApp.h/.cpp    # headless 权威对局 + GameServer
│   └── ClientApp.h/.cpp    # GameBoardWidget + GameClient
├── Network/
│   ├── Protocol.h/.cpp
│   ├── MessageSerializer.h/.cpp
│   ├── GameServer.h/.cpp
│   └── GameClient.h/.cpp
└── View/
    └── ...
~~~

---

## 10. 所有权与生命周期

- CardManager::m_allCards 拥有所有由牌堆创建的 Card 对象，并在析构或重新初始化时删除。
- Player 的手牌、装备区和判定区只保存 Card*，不拥有卡牌。
- GameState 的玩家列表和 cardManager 字段都是非拥有指针。
- 当前本地游戏由 GameViewModel 持有 GameState、CardManager 和 ActionViewModel；玩家以 GameState 为 QObject 父对象创建。
- 当前本地游戏中的武将以 GameViewModel 为 QObject 父对象创建，Player::m_character 只保存指针。
- 网络模式中 ServerApp 持有权威 GameViewModel/Model 与 GameServer；ClientApp 只持有 GameClient 和 GameBoardWidget，不创建客户端 Model。
- Card 不属于 QObject 父子树，不能使用 setParent 或 Qt parent 构造参数管理生命周期。
- GameRule 不拥有任何对象，也不保存全局游戏状态。
- Model 的指针型信号只适合在 Model 和 ViewModel 内部使用；发送到 View 时必须先转换为 src/Common/ 中的值类型。

当头文件声明与本文档不一致时，应先更新本文档或说明设计变更，再让 ViewModel 依赖新的接口。
