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
    case CardType::Duel:
    case CardType::Lightning:
    case CardType::Nullify:
    case CardType::Borrow:
    case CardType::Harvest:
    case CardType::Happy:
    case CardType::Famine:
        return CardCategory::Strategy;
    case CardType::Crossbow:
    case CardType::QinglongBlade:
    case CardType::ZhangbaSnake:
    case CardType::KylinBow:
    case CardType::QinggangSword:
    case CardType::IceSword:
    case CardType::DualSword:
    case CardType::EightDiagrams:
    case CardType::BenevolentShield:
        return CardCategory::Equipment;
    }
    return CardCategory::Basic;
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
    case CardType::Duel: return "决斗";
    case CardType::Lightning: return "闪电";
    case CardType::Nullify: return "无懈可击";
    case CardType::Borrow: return "借刀杀人";
    case CardType::Harvest: return "五谷丰登";
    case CardType::Happy: return "乐不思蜀";
    case CardType::Famine: return "兵粮寸断";
    case CardType::Crossbow: return "诸葛连弩";
    case CardType::QinglongBlade: return "青龙偃月刀";
    case CardType::ZhangbaSnake: return "丈八蛇矛";
    case CardType::KylinBow: return "麒麟弓";
    case CardType::QinggangSword: return "青釭剑";
    case CardType::IceSword: return "寒冰剑";
    case CardType::DualSword: return "雌雄双股剑";
    case CardType::EightDiagrams: return "八卦阵";
    case CardType::BenevolentShield: return "仁王盾";
    }
    return "未知";
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
    GameRule::executeKill(state, user, target, this);

    // 仁王盾等防具可能直接抵消杀，此时无 pending
    return state->hasPendingAction() ? ActionResult::RequiresDodge
                                     : ActionResult::Completed;
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

    return !user->hasUsedWineThisTurn() && !user->isWineEnhanced();
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

    // 无懈可击判定
    if (GameRule::checkNullifyBeforeEffect(state, this, target, user,
            [state, user, target]() { GameRule::executeDismantle(state, user, target); })) {
        return ActionResult::Completed;
    }

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

    // 无懈可击判定
    if (GameRule::checkNullifyBeforeEffect(state, this, target, user,
            [state, user, target]() { GameRule::executeSteal(state, user, target); })) {
        return ActionResult::Completed;
    }

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

    GameRule::executeBarbarianInvasion(state, user, this);

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

    GameRule::executeVolley(state, user, this);

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

// ==================== 新锦囊牌：决斗 ====================

DuelCard::DuelCard(CardSuit suit, int number)
    : StrategyCard(CardType::Duel, suit, number)
{
    setDescription("目标先打出【杀】，然后双方轮流打出【杀】，未打出者受到1点伤害");
}

bool DuelCard::canTarget(const GameState* state,
                         const Player* user,
                         const Player* target) const
{
    if (!state || !user || !target) return false;
    if (user == target) return false;
    return target->isAlive();
}

std::vector<Player*> DuelCard::getValidTargets(const GameState* state,
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

ActionResult DuelCard::execute(GameState* state,
                                Player* user,
                                const std::vector<Player*>& targets)
{
    if (!state || !user || targets.empty()) {
        return ActionResult::Completed;
    }
    Player* target = targets.front();
    // 无懈可击判定
    if (GameRule::checkNullifyBeforeEffect(state, this, target, user,
            [state, user, target, this]() {
                GameRule::executeDuel(state, user, target, this);
            })) {
        return ActionResult::Completed;
    }

    GameRule::executeDuel(state, user, target, this);
    return ActionResult::RequiresKill;
}

// ==================== 新锦囊牌：闪电 ====================

LightningCard::LightningCard(CardSuit suit, int number)
    : StrategyCard(CardType::Lightning, suit, number)
{
    setDescription("判定：黑桃2-9则受到3点雷电伤害，否则移至下家");
}

bool LightningCard::canUse(const GameState* state, const Player* user) const
{
    return StrategyCard::canUse(state, user) &&
           !user->hasJudgmentCard(CardType::Lightning);
}

std::vector<Player*> LightningCard::getValidTargets(const GameState* state,
                                                      const Player* user) const
{
    std::vector<Player*> targets;
    if (state && user && !user->hasJudgmentCard(CardType::Lightning)) {
        targets.push_back(const_cast<Player*>(user));
    }
    return targets;
}

ActionResult LightningCard::execute(GameState* state,
                                     Player* user,
                                     const std::vector<Player*>& targets)
{
    (void)targets;
    if (!state || !user) {
        return ActionResult::Completed;
    }
    // 闪电放入使用者自己的判定区，在判定阶段结算
    user->addJudgmentCard(this);
    return ActionResult::Completed;
}

// ==================== 新锦囊牌：无懈可击 ====================

NullifyCard::NullifyCard(CardSuit suit, int number)
    : StrategyCard(CardType::Nullify, suit, number)
{
    setDescription("抵消一张锦囊牌对一名角色的效果");
}

bool NullifyCard::canUse(const GameState* state, const Player* user) const
{
    if (!state || !user) return false;
    if (!user->isAlive()) return false;
    return true;
}

std::vector<Player*> NullifyCard::getValidTargets(const GameState* state,
                                                    const Player* user) const
{
    (void)state;
    (void)user;
    return std::vector<Player*>();
}

ActionResult NullifyCard::execute(GameState* state,
                                   Player* user,
                                   const std::vector<Player*>& targets)
{
    (void)state;
    (void)user;
    (void)targets;
    // 无懈可击不会主动使用，而是通过 pending action 系统响应
    // 实际逻辑在 ActionViewModel::respondCard + GameRule::handleNullifyResponse
    return ActionResult::Completed;
}

// ==================== 新锦囊牌：借刀杀人 ====================

BorrowCard::BorrowCard(CardSuit suit, int number)
    : StrategyCard(CardType::Borrow, suit, number)
{
    setDescription("令装备武器的角色使用【杀】");
}

bool BorrowCard::canTarget(const GameState* state,
                           const Player* user,
                           const Player* target) const
{
    if (!state || !user || !target) return false;
    if (user == target) return false;
    if (!target->isAlive()) return false;
    return target->hasEquipCards();
}

std::vector<Player*> BorrowCard::getValidTargets(const GameState* state,
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

ActionResult BorrowCard::execute(GameState* state,
                                  Player* user,
                                  const std::vector<Player*>& targets)
{
    if (!state || !user || targets.empty()) {
        return ActionResult::Completed;
    }
    Player* target = targets.front();

    // 无懈可击判定
    if (GameRule::checkNullifyBeforeEffect(state, this, target, user,
            [state, user, target]() { GameRule::executeBorrow(state, user, target); })) {
        return ActionResult::Completed;
    }

    GameRule::executeBorrow(state, user, target);
    return ActionResult::Completed;
}

// ==================== 新锦囊牌：五谷丰登 ====================

HarvestCard::HarvestCard(CardSuit suit, int number)
    : StrategyCard(CardType::Harvest, suit, number)
{
    setDescription("展示牌堆顶至存活人数张牌，依次选择一张");
}

std::vector<Player*> HarvestCard::getValidTargets(const GameState* state,
                                                    const Player* user) const
{
    (void)user;
    if (!state) return std::vector<Player*>();
    return state->alivePlayers();
}

ActionResult HarvestCard::execute(GameState* state,
                                   Player* user,
                                   const std::vector<Player*>& targets)
{
    (void)targets;
    if (!state || !user) {
        return ActionResult::Completed;
    }
    GameRule::executeHarvest(state, user);
    return ActionResult::Completed;
}

// ==================== 新锦囊牌：乐不思蜀 ====================

HappyCard::HappyCard(CardSuit suit, int number)
    : StrategyCard(CardType::Happy, suit, number)
{
    setDescription("判定：非红桃则跳过出牌阶段");
}

bool HappyCard::canTarget(const GameState* state,
                          const Player* user,
                          const Player* target) const
{
    if (!state || !user || !target) return false;
    if (user == target) return false;
    return target->isAlive() && !target->hasJudgmentCard(CardType::Happy);
}

std::vector<Player*> HappyCard::getValidTargets(const GameState* state,
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

ActionResult HappyCard::execute(GameState* state,
                                 Player* user,
                                 const std::vector<Player*>& targets)
{
    if (!state || !user || targets.empty()) {
        return ActionResult::Completed;
    }
    Player* target = targets.front();

    // 无懈可击判定
    if (GameRule::checkNullifyBeforeEffect(state, this, target, user,
            [target, this]() { target->addJudgmentCard(this); })) {
        return ActionResult::Completed;
    }

    // 直接放入目标判定区
    target->addJudgmentCard(this);
    return ActionResult::Completed;
}

// ==================== 新锦囊牌：兵粮寸断 ====================

FamineCard::FamineCard(CardSuit suit, int number)
    : StrategyCard(CardType::Famine, suit, number)
{
    setDescription("判定：非梅花则跳过摸牌阶段");
}

bool FamineCard::canTarget(const GameState* state,
                           const Player* user,
                           const Player* target) const
{
    if (!state || !user || !target) return false;
    if (user == target) return false;
    return target->isAlive() && !target->hasJudgmentCard(CardType::Famine);
}

std::vector<Player*> FamineCard::getValidTargets(const GameState* state,
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

ActionResult FamineCard::execute(GameState* state,
                                  Player* user,
                                  const std::vector<Player*>& targets)
{
    if (!state || !user || targets.empty()) {
        return ActionResult::Completed;
    }
    Player* target = targets.front();

    // 无懈可击判定
    if (GameRule::checkNullifyBeforeEffect(state, this, target, user,
            [target, this]() { target->addJudgmentCard(this); })) {
        return ActionResult::Completed;
    }

    // 直接放入目标判定区
    target->addJudgmentCard(this);
    return ActionResult::Completed;
}

// ==================== 装备牌基类 ====================

EquipmentCard::EquipmentCard(CardType type, CardSuit suit, int number, EquipSlot slot)
    : Card(type, suit, number)
    , m_slot(slot)
{
}

EquipSlot EquipmentCard::equipSlot() const { return m_slot; }

bool EquipmentCard::canUse(const GameState* state, const Player* user) const
{
    if (!state || !user) return false;
    if (state->currentPhase() != PhaseType::Play) return false;
    if (!user->isAlive()) return false;
    return true;
}

bool EquipmentCard::canTarget(const GameState* state,
                               const Player* user,
                               const Player* target) const
{
    (void)state;
    if (!user || !target) return false;
    return user == target;
}

std::vector<Player*> EquipmentCard::getValidTargets(const GameState* state,
                                                      const Player* user) const
{
    std::vector<Player*> targets;
    if (state && user) {
        targets.push_back(const_cast<Player*>(user));
    }
    return targets;
}

ActionResult EquipmentCard::execute(GameState* state,
                                     Player* user,
                                     const std::vector<Player*>& targets)
{
    (void)targets;
    if (!state || !user) return ActionResult::Completed;
    user->equipCard(this);
    return ActionResult::Completed;
}

int EquipmentCard::attackRangeBonus() const { return 0; }
int EquipmentCard::defenseBonus() const { return 0; }
bool EquipmentCard::canExtraKill() const { return false; }
bool EquipmentCard::ignoreArmor() const { return false; }

// ==================== 武器牌 ====================

CrossbowCard::CrossbowCard(CardSuit suit, int number)
    : EquipmentCard(CardType::Crossbow, suit, number, EquipSlot::Weapon)
{
    setDescription("出牌阶段无限制出【杀】");
}

int CrossbowCard::attackRangeBonus() const { return 0; }
bool CrossbowCard::canExtraKill() const { return true; }

QinglongBladeCard::QinglongBladeCard(CardSuit suit, int number)
    : EquipmentCard(CardType::QinglongBlade, suit, number, EquipSlot::Weapon)
{
    setDescription("攻击距离+2");
}

int QinglongBladeCard::attackRangeBonus() const { return 2; }

ZhangbaSnakeCard::ZhangbaSnakeCard(CardSuit suit, int number)
    : EquipmentCard(CardType::ZhangbaSnake, suit, number, EquipSlot::Weapon)
{
    setDescription("攻击距离+3");
}

int ZhangbaSnakeCard::attackRangeBonus() const { return 3; }

KylinBowCard::KylinBowCard(CardSuit suit, int number)
    : EquipmentCard(CardType::KylinBow, suit, number, EquipSlot::Weapon)
{
    setDescription("攻击距离+5");
}

int KylinBowCard::attackRangeBonus() const { return 5; }

QinggangSwordCard::QinggangSwordCard(CardSuit suit, int number)
    : EquipmentCard(CardType::QinggangSword, suit, number, EquipSlot::Weapon)
{
    setDescription("攻击距离+2，无视防具");
}

int QinggangSwordCard::attackRangeBonus() const { return 2; }
bool QinggangSwordCard::ignoreArmor() const { return true; }

IceSwordCard::IceSwordCard(CardSuit suit, int number)
    : EquipmentCard(CardType::IceSword, suit, number, EquipSlot::Weapon)
{
    setDescription("攻击距离+2");
}

int IceSwordCard::attackRangeBonus() const { return 2; }

DualSwordCard::DualSwordCard(CardSuit suit, int number)
    : EquipmentCard(CardType::DualSword, suit, number, EquipSlot::Weapon)
{
    setDescription("攻击距离+2");
}

int DualSwordCard::attackRangeBonus() const { return 2; }

// ==================== 防具牌 ====================

EightDiagramsCard::EightDiagramsCard(CardSuit suit, int number)
    : EquipmentCard(CardType::EightDiagrams, suit, number, EquipSlot::Armor)
{
    setDescription("需要出【闪】时判定：红色则视为出【闪】");
}

BenevolentShieldCard::BenevolentShieldCard(CardSuit suit, int number)
    : EquipmentCard(CardType::BenevolentShield, suit, number, EquipSlot::Armor)
{
    setDescription("黑色【杀】对你无效");
}
