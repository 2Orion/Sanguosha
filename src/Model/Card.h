#ifndef CARD_H
#define CARD_H

#include <string>
#include <vector>
#include "CommonTypes.h"

class GameState;
class Player;

class Card {
public:
    /// @param type   卡牌类型
    /// @param suit   花色
    /// @param number 点数（1-13，A-K）
    Card(CardType type, CardSuit suit, int number);
    virtual ~Card() = default;

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
    std::string cardName() const;

    /// 卡牌说明文字
    std::string description() const;

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
    virtual std::vector<Player*> getValidTargets(const GameState* state,
                                                  const Player* user) const;

    /// 执行卡牌效果
    /// @return 动作结果，指示是否需要玩家进一步响应
    virtual ActionResult execute(GameState* state,
                                  Player* user,
                                  const std::vector<Player*>& targets);

    // ==================== 工具方法 ====================

    /// 根据 CardType 获取中文名称
    static std::string cardTypeName(CardType type);

    /// 花色符号字符串
    std::string suitSymbol() const;

    /// 点数显示字符串
    std::string numberString() const;

protected:
    /// 子类在构造函数中调用以设置自定义名称/描述
    void setCardName(const std::string& name);
    void setDescription(const std::string& desc);

private:
    int m_id;
    CardType m_type;
    CardSuit m_suit;
    int m_number;
    std::string m_cardName;
    std::string m_description;

    static int s_nextCardId;  // 全局自增 ID
};

// ==================== 基本牌 ====================

/// 杀
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

/// 闪
class DodgeCard : public Card {
public:
    DodgeCard(CardSuit suit, int number);
    ~DodgeCard() override = default;

    /// 闪不能在出牌阶段主动使用，始终返回 false
    bool canUse(const GameState* state, const Player* user) const override;
};

/// 桃
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

/// 酒
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

// ==================== 锦囊牌 ====================

/// 锦囊牌基类
class StrategyCard : public Card {
public:
    StrategyCard(CardType type, CardSuit suit, int number);
    ~StrategyCard() override = default;

    bool canUse(const GameState* state, const Player* user) const override;
};

/// 过河拆桥
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

/// 顺手牵羊
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

/// 无中生有
class BountifulCard : public StrategyCard {
public:
    BountifulCard(CardSuit suit, int number);

    std::vector<Player*> getValidTargets(const GameState* state,
                                          const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const std::vector<Player*>& targets) override;
};

/// 南蛮入侵
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

/// 万箭齐发
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

/// 桃园结义
class PeachGardenCard : public StrategyCard {
public:
    PeachGardenCard(CardSuit suit, int number);

    std::vector<Player*> getValidTargets(const GameState* state,
                                          const Player* user) const override;
    ActionResult execute(GameState* state,
                          Player* user,
                          const std::vector<Player*>& targets) override;
};

#endif // CARD_H
