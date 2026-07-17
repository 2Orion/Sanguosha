#include "Character.h"
#include "Card.h"
#include "Player.h"
#include "GameState.h"
#include "CardManager.h"

// ==================== Character 基类 ====================

Character::Character(const std::string& name, int maxHp,
                     const std::string& skillName, const std::string& skillDesc,
                     QObject* parent)
    : QObject(parent)
    , m_name(name)
    , m_maxHp(maxHp)
    , m_skillName(skillName)
    , m_skillDescription(skillDesc)
{
}

std::string Character::characterName() const { return m_name; }
int Character::maxHp() const { return m_maxHp; }
std::string Character::skillName() const { return m_skillName; }
std::string Character::skillDescription() const { return m_skillDescription; }

bool Character::hasSkill() const { return true; }
bool Character::hasActiveSkill() const { return false; }

bool Character::triggerCondition(GameEvent, const GameState*, const Player*) const { return false; }

void Character::triggerSkill(GameState*, Player*) {}

CardType Character::skillTransformCard(const Card* card) const {
    if (!card) return CardType::Kill; // null 安全检查
    return card->cardType();
}

int Character::onDrawPhaseBonus() const { return 0; }
bool Character::requireExtraDodge() const { return false; }
bool Character::canRedirectKill() const { return false; }
Player* Character::getRedirectTarget(GameState*, Player*, Player*) const { return nullptr; }
bool Character::canDiscardAndDraw() const { return false; }

// ==================== 曹操 - 奸雄 ====================

CaoCao::CaoCao(QObject* parent)
    : Character("曹操", 4, "奸雄", "受到伤害后，摸一张牌", parent)
{
}

bool CaoCao::hasSkill() const { return true; }

bool CaoCao::triggerCondition(GameEvent event, const GameState*, const Player*) const
{
    return event == GameEvent::OnDamage;
}

void CaoCao::triggerSkill(GameState* state, Player* self)
{
    if (!state || !self || !state->cardManager()) return;
    Card* card = state->cardManager()->drawCard();
    if (card) {
        self->addHandCard(card);
        emit skillTriggered(QString::fromStdString(m_skillName));
    }
}

// ==================== 关羽 - 武圣 ====================

GuanYu::GuanYu(QObject* parent)
    : Character("关羽", 4, "武圣", "红色牌可当【杀】使用或打出", parent)
{
}

bool GuanYu::hasSkill() const { return true; }
bool GuanYu::hasActiveSkill() const { return true; }

CardType GuanYu::skillTransformCard(const Card* card) const
{
    if (!card) return CardType::Kill; // skillTransformCard 判断：关羽可将牌当杀用
    if (card->isRed()) return CardType::Kill;
    return card->cardType();
}

// ==================== 张飞 - 咆哮 ====================

ZhangFei::ZhangFei(QObject* parent)
    : Character("张飞", 4, "咆哮", "出牌阶段可使用任意张【杀】", parent)
{
}

bool ZhangFei::hasSkill() const { return true; }
bool ZhangFei::triggerCondition(GameEvent, const GameState*, const Player*) const { return false; }
void ZhangFei::triggerSkill(GameState*, Player*) {}

// ==================== 赵云 - 龙胆 ====================

ZhaoYun::ZhaoYun(QObject* parent)
    : Character("赵云", 4, "龙胆", "可将【杀】当【闪】，【闪】当【杀】使用或打出", parent)
{
}

bool ZhaoYun::hasSkill() const { return true; }


CardType ZhaoYun::skillTransformCard(const Card* card) const
{
    if (!card) return CardType::Kill;
    if (card->cardType() == CardType::Kill) return CardType::Dodge;
    if (card->cardType() == CardType::Dodge) return CardType::Kill;
    return card->cardType();
}

// ==================== 孙权 - 制衡 ====================

SunQuan::SunQuan(QObject* parent)
    : Character("孙权", 4, "制衡", "出牌阶段限一次，可弃置任意张手牌并摸等量的牌", parent)
{
}

bool SunQuan::hasSkill() const { return true; }
bool SunQuan::hasActiveSkill() const { return true; }

bool SunQuan::canDiscardAndDraw() const { return true; }

// ==================== 周瑜 - 英姿 ====================

ZhouYu::ZhouYu(QObject* parent)
    : Character("周瑜", 3, "英姿", "摸牌阶段多摸一张牌", parent)
{
}

bool ZhouYu::hasSkill() const { return true; }

int ZhouYu::onDrawPhaseBonus() const { return 1; }

// ==================== 吕布 - 无双 ====================

LvBu::LvBu(QObject* parent)
    : Character("吕布", 4, "无双", "你使用的【杀】需两张【闪】才能抵消", parent)
{
}

bool LvBu::hasSkill() const { return true; }

bool LvBu::requireExtraDodge() const { return true; }

// ==================== 大乔 - 流离 ====================

DaQiao::DaQiao(QObject* parent)
    : Character("大乔", 3, "流离", "当你成为【杀】的目标时，可弃一张牌转移此【杀】", parent)
{
}

bool DaQiao::hasSkill() const { return true; }
bool DaQiao::hasActiveSkill() const { return true; }

bool DaQiao::canRedirectKill() const { return true; }

Player* DaQiao::getRedirectTarget(GameState* state, Player* self, Player* attacker) const
{
    if (!state || !self || !attacker) return nullptr;
    // 转移给除自己和攻击者外的存活角色
    for (Player* p : state->alivePlayers()) {
        if (p != self && p != attacker) {
            return p;
        }
    }
    return nullptr;
}

// ==================== 司马懿 - 反馈 ====================

SiMaYi::SiMaYi(QObject* parent)
    : Character("司马懿", 3, "反馈", "受到伤害后可获得伤害来源一张牌", parent)
{
}

bool SiMaYi::hasSkill() const { return true; }

bool SiMaYi::triggerCondition(GameEvent event, const GameState*, const Player*) const
{
    return event == GameEvent::OnDamage;
}

void SiMaYi::triggerSkill(GameState* state, Player* self)
{
    if (!state || !self) return;

    // 获取伤害来源（从 pending action 中推断，简化实现）
    const PendingActionInfo& info = state->pendingActionInfo();
    Player* source = info.source;
    if (!source || source == self) return;

    // 从伤害来源获得一张手牌
    Card* card = source->getRandomHandCard();
    if (card) {
        source->removeHandCard(card);
        self->addHandCard(card);
        emit skillTriggered(QString::fromStdString(m_skillName));
    }
}
