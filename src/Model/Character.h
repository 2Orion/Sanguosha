#ifndef CHARACTER_H
#define CHARACTER_H

#include <string>
#include "CommonTypes.h"
#include "Event.h"

class GameState;
class Player;
class Card;

class Character {
public:
    Character(const std::string& name, int maxHp,
              const std::string& skillName, const std::string& skillDesc);
    virtual ~Character() = default;

    // ==================== 只读属性 ====================

    std::string characterName() const;
    int maxHp() const;
    std::string skillName() const;
    std::string skillDescription() const;

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

    // ==================== 事件通知 ====================

    /// 技能发动事件（ViewModel 用来显示"XX发动了技能"）
    EventListener<const std::string&> skillTriggered;

protected:
    std::string m_name;
    int m_maxHp;
    std::string m_skillName;
    std::string m_skillDescription;
};

// ==================== 具体武将 ====================

/// 曹操 - 奸雄
class CaoCao : public Character {
public:
    CaoCao();

    bool hasSkill() const override;
    bool triggerCondition(GameEvent event,
                          const GameState* state,
                          const Player* self) const override;
    void triggerSkill(GameState* state, Player* self) override;
};

/// 关羽 - 武圣
class GuanYu : public Character {
public:
    GuanYu();

    bool hasSkill() const override;
    CardType skillTransformCard(const Card* card) const override;
};

/// 张飞 - 咆哮
class ZhangFei : public Character {
public:
    ZhangFei();

    bool hasSkill() const override;
    bool triggerCondition(GameEvent event,
                          const GameState* state,
                          const Player* self) const override;
    void triggerSkill(GameState* state, Player* self) override;
};

/// 赵云 - 龙胆
class ZhaoYun : public Character {
public:
    ZhaoYun();

    bool hasSkill() const override;
    CardType skillTransformCard(const Card* card) const override;
};

#endif // CHARACTER_H
