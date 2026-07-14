#include "Card.h"
#include "GameState.h"
#include "Player.h"
#include "GameRule.h"

int Card::s_nextCardId = 1;

// ==================== Card 基类 ====================

Card::Card(CardType type, CardSuit suit, int number)
    : m_id(s_nextCardId++)
    , m_type(type)
    , m_suit(suit)
    , m_number(number)
{
    m_cardName = cardTypeName(type);
}

int Card::id() const
{
    return m_id;
}

CardType Card::cardType() const
{
    return m_type;
}

CardSuit Card::suit() const
{
    return m_suit;
}

int Card::number() const
{
    return m_number;
}

CardColor Card::color() const
{
    if (m_suit == CardSuit::Heart || m_suit == CardSuit::Diamond) {
        return CardColor::Red;
    }
    return CardColor::Black;
}

std::string Card::cardName() const
{
    return m_cardName;
}

std::string Card::description() const
{
    return m_description;
}

CardCategory Card::category() const
{
    switch (m_type) {
    case CardType::Kill:
    case CardType::Dodge:
    case CardType::Peach:
    case CardType::Wine:
        return CardCategory::Basic;
    case CardType::Dismantle:
    case CardType::Steal:
    case CardType::Bountiful:
    case CardType::BarbarianInvasion:
    case CardType::Volley:
    case CardType::PeachGarden:
        return CardCategory::Strategy;
    default:
        return CardCategory::Basic;
    }
}

bool Card::isBasic() const
{
    return category() == CardCategory::Basic;
}

bool Card::isStrategy() const
{
    return category() == CardCategory::Strategy;
}

bool Card::isEquipment() const
{
    return category() == CardCategory::Equipment;
}

bool Card::isRed() const
{
    return color() == CardColor::Red;
}

bool Card::isBlack() const
{
    return color() == CardColor::Black;
}

bool Card::canUse(const GameState* state, const Player* user) const
{
    (void)state;
    (void)user;
    return false;
}

bool Card::canTarget(const GameState* state,
                     const Player* user,
                     const Player* target) const
{
    (void)state;
    (void)user;
    (void)target;
    return false;
}

std::vector<Player*> Card::getValidTargets(const GameState* state,
                                            const Player* user) const
{
    (void)state;
    (void)user;
    return std::vector<Player*>();
}

ActionResult Card::execute(GameState* state,
                           Player* user,
                           const std::vector<Player*>& targets)
{
    (void)state;
    (void)user;
    (void)targets;
    return ActionResult::Completed;
}

std::string Card::cardTypeName(CardType type)
{
    switch (type) {
    case CardType::Kill: return "杀";
    case CardType::Dodge: return "闪";
    case CardType::Peach: return "桃";
    case CardType::Wine: return "酒";
    case CardType::Dismantle: return "过河拆桥";
    case CardType::Steal: return "顺手牵羊";
    case CardType::Bountiful: return "无中生有";
    case CardType::BarbarianInvasion: return "南蛮入侵";
    case CardType::Volley: return "万箭齐发";
    case CardType::PeachGarden: return "桃园结义";
    default: return "未知";
    }
}

std::string Card::suitSymbol() const
{
    switch (m_suit) {
    case CardSuit::Spade: return "♠";
    case CardSuit::Heart: return "♥";
    case CardSuit::Club: return "♣";
    case CardSuit::Diamond: return "♦";
    default: return "";
    }
}

std::string Card::numberString() const
{
    switch (m_number) {
    case 1: return "A";
    case 11: return "J";
    case 12: return "Q";
    case 13: return "K";
    default: return std::to_string(m_number);
    }
}

void Card::setCardName(const std::string& name)
{
    m_cardName = name;
}

void Card::setDescription(const std::string& desc)
{
    m_description = desc;
}

// ==================== 基本牌：杀 ====================

KillCard::KillCard(CardSuit suit, int number)
    : Card(CardType::Kill, suit, number)
{
    setDescription("对目标造成1点伤害");
}

bool KillCard::canUse(const GameState* state, const Player* user) const
{
    if (!state || !user) return false;
    if (state->currentPhase() != PhaseType::Play) return false;
    if (!user->isAlive()) return false;

    return GameRule::canPlayKill(state, user);
}

bool KillCard::canTarget(const GameState* state,
                         const Player* user,
                         const Player* target) const
{
    if (!state || !user || !target) return false;
    if (user == target) return false;
    if (!target->isAlive()) return false;

    return true;
}

std::vector<Player*> KillCard::getValidTargets(const GameState* state,
                                                const Player* user) const
{
    std::vector<Player*> targets;
    if (!state || !user) return targets;

    for (Player* p : state->alivePlayers()) {
        if (p != user && canTarget(state, user, p)) {
            targets.push_back(p);
        }
    }
    return targets;
}

ActionResult KillCard::execute(GameState* state,
                                Player* user,
                                const std::vector<Player*>& targets)
{
    if (!state || !user || targets.empty()) {
        return ActionResult::Completed;
    }

    Player* target = targets.front();
    GameRule::executeKill(state, user, target);

    return ActionResult::RequiresDodge;
}

// ==================== 基本牌：闪 ====================

DodgeCard::DodgeCard(CardSuit suit, int number)
    : Card(CardType::Dodge, suit, number)
{
    setDescription("抵消一次【杀】的效果");
}

bool DodgeCard::canUse(const GameState* state, const Player* user) const
{
    (void)state;
    (void)user;
    return false;
}

// ==================== 基本牌：桃 ====================

PeachCard::PeachCard(CardSuit suit, int number)
    : Card(CardType::Peach, suit, number)
{
    setDescription("回复1点体力");
}

bool PeachCard::canUse(const GameState* state, const Player* user) const
{
    if (!state || !user) return false;
    if (!user->isAlive()) return false;

    if (state->currentPhase() == PhaseType::Play) {
        return !user->isFullHp();
    }

    return false;
}

bool PeachCard::canTarget(const GameState* state,
                          const Player* user,
                          const Player* target) const
{
    (void)state;
    if (!user || !target) return false;

    return user == target;
}

std::vector<Player*> PeachCard::getValidTargets(const GameState* state,
                                                 const Player* user) const
{
    std::vector<Player*> targets;
    if (canTarget(state, user, const_cast<Player*>(user))) {
        targets.push_back(const_cast<Player*>(user));
    }
    return targets;
}

ActionResult PeachCard::execute(GameState* state,
                                 Player* user,
                                 const std::vector<Player*>& targets)
{
    if (!state || !user) {
        return ActionResult::Completed;
    }

    Player* target = targets.empty() ? user : targets.front();
    GameRule::executePeach(state, user, target);

    return ActionResult::Completed;
}

// ==================== 基本牌：酒 ====================

WineCard::WineCard(CardSuit suit, int number)
    : Card(CardType::Wine, suit, number)
{
    setDescription("下张【杀】伤害+1 / 濒死时回复1体力");
}

bool WineCard::canUse(const GameState* state, const Player* user) const
{
    if (!state || !user) return false;
    if (!user->isAlive()) return false;
    if (state->currentPhase() != PhaseType::Play) return false;

    return !user->isWineEnhanced();
}

std::vector<Player*> WineCard::getValidTargets(const GameState* state,
                                                const Player* user) const
{
    std::vector<Player*> targets;
    if (state && user) {
        targets.push_back(const_cast<Player*>(user));
    }
    return targets;
}

ActionResult WineCard::execute(GameState* state,
                                Player* user,
                                const std::vector<Player*>& targets)
{
    (void)targets;

    if (!state || !user) {
        return ActionResult::Completed;
    }

    GameRule::executeWine(state, user);

    return ActionResult::Completed;
}

// ==================== 锦囊牌基类 ====================

StrategyCard::StrategyCard(CardType type, CardSuit suit, int number)
    : Card(type, suit, number)
{
}

bool StrategyCard::canUse(const GameState* state, const Player* user) const
{
    if (!state || !user) return false;
    if (state->currentPhase() != PhaseType::Play) return false;
    if (!user->isAlive()) return false;

    return !getValidTargets(state, user).empty();
}

// ==================== 锦囊牌：过河拆桥 ====================

DismantleCard::DismantleCard(CardSuit suit, int number)
    : StrategyCard(CardType::Dismantle, suit, number)
{
    setDescription("弃置目标玩家的一张牌");
}

bool DismantleCard::canTarget(const GameState* state,
                               const Player* user,
                               const Player* target) const
{
    if (!state || !user || !target) return false;
    if (user == target) return false;
    if (!target->isAlive()) return false;

    return target->hasHandCards() || target->hasEquipCards();
}

std::vector<Player*> DismantleCard::getValidTargets(const GameState* state,
                                                      const Player* user) const
{
    std::vector<Player*> targets;
    if (!state || !user) return targets;

    for (Player* p : state->alivePlayers()) {
        if (canTarget(state, user, p)) {
            targets.push_back(p);
        }
    }
    return targets;
}

ActionResult DismantleCard::execute(GameState* state,
                                     Player* user,
                                     const std::vector<Player*>& targets)
{
    if (!state || !user || targets.empty()) {
        return ActionResult::Completed;
    }

    Player* target = targets.front();
    GameRule::executeDismantle(state, user, target);

    return ActionResult::Completed;
}

// ==================== 锦囊牌：顺手牵羊 ====================

StealCard::StealCard(CardSuit suit, int number)
    : StrategyCard(CardType::Steal, suit, number)
{
    setDescription("获得目标玩家的一张牌");
}

bool StealCard::canTarget(const GameState* state,
                          const Player* user,
                          const Player* target) const
{
    if (!state || !user || !target) return false;
    if (user == target) return false;
    if (!target->isAlive()) return false;

    return target->hasHandCards() || target->hasEquipCards();
}

std::vector<Player*> StealCard::getValidTargets(const GameState* state,
                                                 const Player* user) const
{
    std::vector<Player*> targets;
    if (!state || !user) return targets;

    for (Player* p : state->alivePlayers()) {
        if (canTarget(state, user, p)) {
            targets.push_back(p);
        }
    }
    return targets;
}

ActionResult StealCard::execute(GameState* state,
                                 Player* user,
                                 const std::vector<Player*>& targets)
{
    if (!state || !user || targets.empty()) {
        return ActionResult::Completed;
    }

    Player* target = targets.front();
    GameRule::executeSteal(state, user, target);

    return ActionResult::Completed;
}

// ==================== 锦囊牌：无中生有 ====================

BountifulCard::BountifulCard(CardSuit suit, int number)
    : StrategyCard(CardType::Bountiful, suit, number)
{
    setDescription("摸两张牌");
}

std::vector<Player*> BountifulCard::getValidTargets(const GameState* state,
                                                      const Player* user) const
{
    std::vector<Player*> targets;
    if (state && user) {
        targets.push_back(const_cast<Player*>(user));
    }
    return targets;
}

ActionResult BountifulCard::execute(GameState* state,
                                     Player* user,
                                     const std::vector<Player*>& targets)
{
    (void)targets;

    if (!state || !user) {
        return ActionResult::Completed;
    }

    GameRule::executeBountiful(state, user);

    return ActionResult::Completed;
}

// ==================== 锦囊牌：南蛮入侵 ====================

BarbarianCard::BarbarianCard(CardSuit suit, int number)
    : StrategyCard(CardType::BarbarianInvasion, suit, number)
{
    setDescription("除你以外的所有角色需打出一张【杀】，否则受到1点伤害");
}

bool BarbarianCard::canTarget(const GameState* state,
                               const Player* user,
                               const Player* target) const
{
    if (!state || !user || !target) return false;
    if (user == target) return false;

    return target->isAlive();
}

std::vector<Player*> BarbarianCard::getValidTargets(const GameState* state,
                                                      const Player* user) const
{
    std::vector<Player*> targets;
    if (!state || !user) return targets;

    for (Player* p : state->alivePlayers()) {
        if (p != user) {
            targets.push_back(p);
        }
    }
    return targets;
}

ActionResult BarbarianCard::execute(GameState* state,
                                     Player* user,
                                     const std::vector<Player*>& targets)
{
    (void)targets;

    if (!state || !user) {
        return ActionResult::Completed;
    }

    GameRule::executeBarbarianInvasion(state, user);

    return ActionResult::RequiresKill;
}

// ==================== 锦囊牌：万箭齐发 ====================

VolleyCard::VolleyCard(CardSuit suit, int number)
    : StrategyCard(CardType::Volley, suit, number)
{
    setDescription("除你以外的所有角色需打出一张【闪】，否则受到1点伤害");
}

bool VolleyCard::canTarget(const GameState* state,
                           const Player* user,
                           const Player* target) const
{
    if (!state || !user || !target) return false;
    if (user == target) return false;

    return target->isAlive();
}

std::vector<Player*> VolleyCard::getValidTargets(const GameState* state,
                                                  const Player* user) const
{
    std::vector<Player*> targets;
    if (!state || !user) return targets;

    for (Player* p : state->alivePlayers()) {
        if (p != user) {
            targets.push_back(p);
        }
    }
    return targets;
}

ActionResult VolleyCard::execute(GameState* state,
                                  Player* user,
                                  const std::vector<Player*>& targets)
{
    (void)targets;

    if (!state || !user) {
        return ActionResult::Completed;
    }

    GameRule::executeVolley(state, user);

    return ActionResult::RequiresDodge;
}

// ==================== 锦囊牌：桃园结义 ====================

PeachGardenCard::PeachGardenCard(CardSuit suit, int number)
    : StrategyCard(CardType::PeachGarden, suit, number)
{
    setDescription("所有角色回复1点体力");
}

std::vector<Player*> PeachGardenCard::getValidTargets(const GameState* state,
                                                        const Player* user) const
{
    (void)user;

    if (!state) return std::vector<Player*>();

    return state->alivePlayers();
}

ActionResult PeachGardenCard::execute(GameState* state,
                                       Player* user,
                                       const std::vector<Player*>& targets)
{
    (void)user;
    (void)targets;

    if (!state) {
        return ActionResult::Completed;
    }

    GameRule::executePeachGarden(state);

    return ActionResult::Completed;
}
