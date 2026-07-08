#include "Character.h"
#include "Card.h"
#include "Player.h"
#include "GameState.h"
#include "CardManager.h"

// ==================== Character 基类 ====================

Character::Character(const std::string& name, int maxHp,
                     const std::string& skillName, const std::string& skillDesc)
    : m_name(name)
    , m_maxHp(maxHp)
    , m_skillName(skillName)
    , m_skillDescription(skillDesc)
{
}

std::string Character::characterName() const
{
    return m_name;
}

int Character::maxHp() const
{
    return m_maxHp;
}

std::string Character::skillName() const
{
    return m_skillName;
}

std::string Character::skillDescription() const
{
    return m_skillDescription;
}

bool Character::hasSkill() const
{
    return true;
}

bool Character::triggerCondition(GameEvent event,
                                const GameState* state,
                                const Player* self) const
{
    (void)event;
    (void)state;
    (void)self;
    return false;
}

void Character::triggerSkill(GameState* state, Player* self)
{
    (void)state;
    (void)self;
}

CardType Character::skillTransformCard(const Card* card) const
{
    return card->cardType();
}

// ==================== 曹操 - 奸雄 ====================

CaoCao::CaoCao()
    : Character("曹操", 4, "奸雄", "受到伤害后，摸一张牌")
{
}

bool CaoCao::hasSkill() const
{
    return true;
}

bool CaoCao::triggerCondition(GameEvent event,
                              const GameState* state,
                              const Player* self) const
{
    (void)state;
    (void)self;
    return event == GameEvent::OnDamage;
}

void CaoCao::triggerSkill(GameState* state, Player* self)
{
    if (!state || !self || !state->cardManager()) {
        return;
    }

    Card* card = state->cardManager()->drawCard();
    if (card) {
        self->addHandCard(card);
        skillTriggered.emit(m_skillName);
    }
}

// ==================== 关羽 - 武圣 ====================

GuanYu::GuanYu()
    : Character("关羽", 4, "武圣", "红色牌可当【杀】使用或打出")
{
}

bool GuanYu::hasSkill() const
{
    return true;
}

CardType GuanYu::skillTransformCard(const Card* card) const
{
    if (card && card->isRed()) {
        return CardType::Kill;
    }
    return card->cardType();
}

// ==================== 张飞 - 咆哮 ====================

ZhangFei::ZhangFei()
    : Character("张飞", 4, "咆哮", "出牌阶段可使用任意张【杀】")
{
}

bool ZhangFei::hasSkill() const
{
    return true;
}

bool ZhangFei::triggerCondition(GameEvent event,
                                const GameState* state,
                                const Player* self) const
{
    (void)state;
    (void)self;
    (void)event;
    return false;
}

void ZhangFei::triggerSkill(GameState* state, Player* self)
{
    (void)state;
    (void)self;
}

// ==================== 赵云 - 龙胆 ====================

ZhaoYun::ZhaoYun()
    : Character("赵云", 4, "龙胆", "可将【杀】当【闪】，【闪】当【杀】使用或打出")
{
}

bool ZhaoYun::hasSkill() const
{
    return true;
}

CardType ZhaoYun::skillTransformCard(const Card* card) const
{
    if (!card) {
        return CardType::Kill;
    }

    if (card->cardType() == CardType::Kill) {
        return CardType::Dodge;
    } else if (card->cardType() == CardType::Dodge) {
        return CardType::Kill;
    }

    return card->cardType();
}
