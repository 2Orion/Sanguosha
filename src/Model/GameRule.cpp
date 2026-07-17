#include "GameRule.h"
#include "GameState.h"
#include "Player.h"
#include "Card.h"
#include "Character.h"
#include "CardManager.h"
#include <algorithm>

namespace GameRule {

namespace {

std::vector<Player*> dyingSaviors(GameState* state, Player* dyingPlayer)
{
    std::vector<Player*> saviors;
    if (!state || !dyingPlayer) return saviors;

    // 濒死者即使体力不大于 0 也仍未死亡，必须先获得自救机会。
    saviors.push_back(dyingPlayer);
    for (Player* player : state->allPlayers()) {
        if (player && player != dyingPlayer && player->isAlive()) {
            saviors.push_back(player);
        }
    }
    return saviors;
}

void resumeAoeContinuation(GameState* state, const PendingActionInfo& info)
{
    if (!state || !info.continuationSource) return;

    std::vector<Player*> remaining;
    for (Player* player : info.continuationTargets) {
        if (player && player->isAlive()) remaining.push_back(player);
    }
    if (remaining.empty()) return;

    Player* nextTarget = remaining.front();
    PendingActionInfo next;
    next.source = info.continuationSource;
    next.target = nextTarget;
    next.sourceCard = info.continuationSourceCard;
    next.requiredCardType = info.continuationCardType;
    next.description = (info.continuationSource->displayName() +
            (info.continuationCardType == CardType::Kill
                 ? " 使用了【南蛮入侵】，" : " 使用了【万箭齐发】，") +
            nextTarget->displayName() +
            (info.continuationCardType == CardType::Kill
                 ? " 需打出【杀】" : " 需打出【闪】")).toStdString();
    next.canSkip = true;
    next.remainingTargets = std::vector<Player*>(remaining.begin() + 1, remaining.end());
    state->setPendingAction(next);
}

} // namespace

// ==================== 规则判断 ====================

bool canPlayKill(const GameState* state, const Player* player)
{
    if (!state || !player) return false;

    // 诸葛连弩：无限制出杀
    if (player->hasCrossbow()) {
        return true;
    }

    // 张飞咆哮：无限制出杀
    if (player->character() &&
        player->character()->characterName() == "张飞") {
        return true;
    }

    return !player->hasUsedKillThisTurn();
}

bool canPlayCard(const GameState* state, const Player* player, const Card* card)
{
    if (!state || !player || !card) return false;

    return card->canUse(state, player);
}

int handLimit(const Player* player)
{
    if (!player) return 0;
    return player->handCardLimit();
}

bool hasDodgeToRespond(const Player* player)
{
    if (!player) return false;

    for (Card* card : player->handCards()) {
        if (card && card->cardType() == CardType::Dodge) {
            return true;
        }

        if (player->character()) {
            CardType transformed = player->character()->skillTransformCard(card);
            if (transformed == CardType::Dodge) {
                return true;
            }
        }
    }

    return false;
}

bool hasKillToRespond(const Player* player)
{
    if (!player) return false;

    for (Card* card : player->handCards()) {
        if (card && card->cardType() == CardType::Kill) {
            return true;
        }

        if (player->character()) {
            CardType transformed = player->character()->skillTransformCard(card);
            if (transformed == CardType::Kill) {
                return true;
            }
        }
    }

    return false;
}

bool hasPeachToSave(const Player* player, const Player* dyingPlayer)
{
    if (!player || !dyingPlayer) return false;

    for (Card* card : player->handCards()) {
        if (card && (card->cardType() == CardType::Peach ||
                     (player == dyingPlayer && card->cardType() == CardType::Wine))) {
            return true;
        }
    }

    return false;
}

// ==================== 卡牌执行 ====================

void executeKill(GameState* state, Player* user, Player* target, Card* killCard)
{
    if (!state || !user || !target) return;

    user->setUsedKillThisTurn(true);

    // 青釭剑：无视防具；否则做防具判定（仁王盾挡黑杀）
    bool ignoreArmor = false;
    if (EquipmentCard* weapon = user->equippedAt(EquipSlot::Weapon)) {
        if (weapon->ignoreArmor())
            ignoreArmor = true;
    }
    if (!ignoreArmor && killCard && armorEffectCheck(state, target, killCard)) {
        // 防具生效，杀直接无效，不进入出闪流程
        user->setWineEnhanced(false);
        return;
    }

    // 大乔流离：可以将杀转移给其他角色
    if (target->character() && target->character()->canRedirectKill()) {
        Player* redirectTarget = target->character()->getRedirectTarget(state, target, user);
        if (redirectTarget && redirectTarget->isAlive() && redirectTarget != target) {
            PendingActionInfo info;
            info.source = user;
            info.target = redirectTarget;
            info.sourceCard = killCard;
            info.requiredCardType = CardType::Dodge;
            info.description = (user->displayName() + " 对 " + target->displayName()
                            + " 使用了【杀】，被【流离】转移至 " + redirectTarget->displayName()
                            + "，请打出【闪】").toStdString();
            info.canSkip = false;
            if (user->character() && user->character()->requireExtraDodge())
                info.requiredDodgeTotal = 2;
            state->setPendingAction(info);
            return;
        }
    }

    PendingActionInfo info;
    info.source = user;
    info.target = target;
    info.sourceCard = killCard;
    info.requiredCardType = CardType::Dodge;
    info.description = (user->displayName() + " 对 " + target->displayName() + " 使用了【杀】，请打出【闪】").toStdString();
    info.canSkip = false;

    // 吕布无双：需要两张闪
    if (user->character() && user->character()->requireExtraDodge()) {
        info.requiredDodgeTotal = 2;
    }

    state->setPendingAction(info);
}

void handleKillResponse(GameState* state, Player* responder, Card* dodgeCard)
{
    if (!state || !responder) return;
    if (!state->hasPendingAction()) return;

    PendingActionInfo info = state->pendingActionInfo();
    if (info.requiredCardType != CardType::Dodge || info.target != responder) return;
    if (dodgeCard && (!responder->hasCard(dodgeCard) ||
                      (dodgeCard->cardType() != CardType::Dodge &&
                       (!responder->character() ||
                        responder->character()->skillTransformCard(dodgeCard) != CardType::Dodge)))) {
        return;
    }

    Player* attacker = info.source;

    if (dodgeCard) {
        responder->removeHandCard(dodgeCard);
        if (state->cardManager()) {
            state->cardManager()->discard(dodgeCard);
        }

        // 吕布无双：需要多张闪，检查是否还需继续出闪
        if (info.requiredDodgeTotal > 1 && info.dodgeProgress < info.requiredDodgeTotal - 1) {
            PendingActionInfo next = info;
            next.dodgeProgress = info.dodgeProgress + 1;
            next.description = (QStringLiteral("【吕布无双】还需 %1 张【闪】— ")
                                .arg(info.requiredDodgeTotal - next.dodgeProgress)
                                + responder->displayName()).toStdString();
            state->setPendingAction(next);
            return;
        }

        if (attacker) attacker->setWineEnhanced(false);
        state->clearPendingAction();
    } else {
        int damageValue = 1;
        if (attacker && attacker->isWineEnhanced()) {
            damageValue = 2;
            attacker->setWineEnhanced(false);
        }

        dealDamage(state, responder, damageValue, attacker, info.sourceCard);
        // dealDamage() may replace the kill response with the dying response.
        // Only clear the original response when the target remains alive.
        if (!responder->isDying() && state->hasPendingAction()) {
            const PendingActionInfo& current = state->pendingActionInfo();
            if (current.source != info.source || current.target != info.target ||
                current.requiredCardType != CardType::Dodge) {
                return;
            }
            state->clearPendingAction();
        }
    }
}

void executePeach(GameState* state, Player* user, Player* target)
{
    if (!state || !target) return;

    (void)user;

    target->heal(1);
}

void executeWine(GameState* state, Player* user)
{
    if (!state || !user) return;

    user->setWineEnhanced(true);
    user->setUsedWineThisTurn(true);
}

// ==================== 锦囊执行 ====================

void executeDismantle(GameState* state, Player* user, Player* target)
{
    if (!state || !user || !target) return;

    Card* card = target->getRandomHandCard();
    if (!card && target->hasEquipCards()) {
        const std::vector<Card*>& equips = target->equipCards();
        if (!equips.empty()) {
            card = equips.front();
        }
    }

    if (card) {
        target->removeHandCard(card);
        target->removeEquipCard(card);

        if (state->cardManager()) {
            state->cardManager()->discard(card);
        }
    }
}

void executeSteal(GameState* state, Player* user, Player* target)
{
    if (!state || !user || !target) return;

    Card* card = target->getRandomHandCard();
    if (!card && target->hasEquipCards()) {
        const std::vector<Card*>& equips = target->equipCards();
        if (!equips.empty()) {
            card = equips.front();
        }
    }

    if (card) {
        target->removeHandCard(card);
        target->removeEquipCard(card);
        user->addHandCard(card);
    }
}

void executeBountiful(GameState* state, Player* user)
{
    if (!state || !user || !state->cardManager()) return;

    std::vector<Card*> cards = state->cardManager()->drawCards(2);
    for (Card* card : cards) {
        if (card) {
            user->addHandCard(card);
        }
    }
}

void executeBarbarianInvasion(GameState* state, Player* user, Card* sourceCard)
{
    if (!state || !user) return;

    std::vector<Player*> targets;
    for (Player* p : state->alivePlayers()) {
        if (p != user) {
            targets.push_back(p);
        }
    }

    if (targets.empty()) return;

    Player* firstTarget = targets.front();

    PendingActionInfo info;
    info.source = user;
    info.target = firstTarget;
    info.sourceCard = sourceCard;
    info.requiredCardType = CardType::Kill;
    info.description = (user->displayName() + " 使用了【南蛮入侵】，" + firstTarget->displayName() + " 需打出【杀】").toStdString();
    info.canSkip = true;
    info.remainingTargets = std::vector<Player*>(targets.begin() + 1, targets.end());

    state->setPendingAction(info);
}

void executeVolley(GameState* state, Player* user, Card* sourceCard)
{
    if (!state || !user) return;

    std::vector<Player*> targets;
    for (Player* p : state->alivePlayers()) {
        if (p != user) {
            targets.push_back(p);
        }
    }

    if (targets.empty()) return;

    Player* firstTarget = targets.front();

    PendingActionInfo info;
    info.source = user;
    info.target = firstTarget;
    info.sourceCard = sourceCard;
    info.requiredCardType = CardType::Dodge;
    info.description = (user->displayName() + " 使用了【万箭齐发】，" + firstTarget->displayName() + " 需打出【闪】").toStdString();
    info.canSkip = true;
    info.remainingTargets = std::vector<Player*>(targets.begin() + 1, targets.end());

    state->setPendingAction(info);
}

void executePeachGarden(GameState* state)
{
    if (!state) return;

    for (Player* p : state->alivePlayers()) {
        if (p && !p->isFullHp()) {
            p->heal(1);
        }
    }
}

// ==================== AOE 响应处理 ====================

void handleAoeKillResponse(GameState* state, Player* responder, Card* killCard)
{
    if (!state || !responder) return;
    if (!state->hasPendingAction()) return;

    // 复制一份，避免 clearPendingAction() 重置内部状态后引用失效
    PendingActionInfo info = state->pendingActionInfo();
    if (info.requiredCardType != CardType::Kill || info.target != responder) return;
    if (killCard && (!responder->hasCard(killCard) ||
                     (killCard->cardType() != CardType::Kill &&
                      (!responder->character() ||
                       responder->character()->skillTransformCard(killCard) != CardType::Kill)))) {
        return;
    }

    if (killCard) {
        responder->removeHandCard(killCard);
        if (state->cardManager()) {
            state->cardManager()->discard(killCard);
        }
    } else {
        dealDamage(state, responder, 1, info.source, info.sourceCard);
        if (responder->isDying() || !state->hasPendingAction()) return;
        const PendingActionInfo& current = state->pendingActionInfo();
        if (current.source != info.source || current.target != info.target ||
            current.requiredCardType != info.requiredCardType || current.isSkillChoice) {
            return;
        }
    }

    state->clearPendingAction();

    std::vector<Player*> remainingTargets;
    for (Player* p : info.remainingTargets) {
        if (p && p->isAlive()) {
            remainingTargets.push_back(p);
        }
    }

    if (!remainingTargets.empty()) {
        Player* nextTarget = remainingTargets.front();
        PendingActionInfo nextInfo;
        nextInfo.source = info.source;
        nextInfo.target = nextTarget;
        nextInfo.sourceCard = info.sourceCard;
        nextInfo.requiredCardType = CardType::Kill;
        nextInfo.description = (info.source->displayName() + " 使用了【南蛮入侵】，" + nextTarget->displayName() + " 需打出【杀】").toStdString();
        nextInfo.canSkip = true;
        nextInfo.remainingTargets = std::vector<Player*>(remainingTargets.begin() + 1, remainingTargets.end());

        state->setPendingAction(nextInfo);
    }
}

void handleAoeDodgeResponse(GameState* state, Player* responder, Card* dodgeCard)
{
    if (!state || !responder) return;
    if (!state->hasPendingAction()) return;

    // 复制一份，避免 clearPendingAction() 重置内部状态后引用失效
    PendingActionInfo info = state->pendingActionInfo();
    if (info.requiredCardType != CardType::Dodge || info.target != responder) return;
    if (dodgeCard && (!responder->hasCard(dodgeCard) ||
                      (dodgeCard->cardType() != CardType::Dodge &&
                       (!responder->character() ||
                        responder->character()->skillTransformCard(dodgeCard) != CardType::Dodge)))) {
        return;
    }

    if (dodgeCard) {
        responder->removeHandCard(dodgeCard);
        if (state->cardManager()) {
            state->cardManager()->discard(dodgeCard);
        }
    } else {
        dealDamage(state, responder, 1, info.source, info.sourceCard);
        if (responder->isDying() || !state->hasPendingAction()) return;
        const PendingActionInfo& current = state->pendingActionInfo();
        if (current.source != info.source || current.target != info.target ||
            current.requiredCardType != info.requiredCardType || current.isSkillChoice) {
            return;
        }
    }

    state->clearPendingAction();

    std::vector<Player*> remainingTargets;
    for (Player* p : info.remainingTargets) {
        if (p && p->isAlive()) {
            remainingTargets.push_back(p);
        }
    }

    if (!remainingTargets.empty()) {
        Player* nextTarget = remainingTargets.front();
        PendingActionInfo nextInfo;
        nextInfo.source = info.source;
        nextInfo.target = nextTarget;
        nextInfo.sourceCard = info.sourceCard;
        nextInfo.requiredCardType = CardType::Dodge;
        nextInfo.description = (info.source->displayName() + " 使用了【万箭齐发】，" + nextTarget->displayName() + " 需打出【闪】").toStdString();
        nextInfo.canSkip = true;
        nextInfo.remainingTargets = std::vector<Player*>(remainingTargets.begin() + 1, remainingTargets.end());

        state->setPendingAction(nextInfo);
    }
}

void handleAoeSkipResponse(GameState* state, Player* responder)
{
    if (!state || !responder) return;

    const PendingActionInfo& info = state->pendingActionInfo();

    if (info.requiredCardType == CardType::Kill) {
        handleAoeKillResponse(state, responder, nullptr);
    } else if (info.requiredCardType == CardType::Dodge) {
        handleAoeDodgeResponse(state, responder, nullptr);
    }
}

// ==================== 伤害与濒死 ====================

void dealDamage(GameState* state, Player* target, int value, Player* source,
                Card* damageCard)
{
    if (!state || !target || value <= 0) return;

    target->damage(value);

    if (target->character()) {
        Character* character = target->character();
        if (character->triggerCondition(GameEvent::OnDamage, state, target)) {
            character->triggerSkill(state, target, damageCard, source);
        }
    }

    if (target->hp() <= 0 && !target->isDying()) {
        target->setDying(true);
        startDyingProcess(state, target);
    }
}

void startDyingProcess(GameState* state, Player* dyingPlayer)
{
    if (!state || !dyingPlayer) return;

    PendingActionInfo previousAction;
    const bool hasDeferredSkill = state->hasPendingAction() &&
            state->pendingActionInfo().isSkillChoice &&
            state->pendingActionInfo().target == dyingPlayer;
    const bool hasAoeContinuation = state->hasPendingAction() &&
            ((state->pendingActionInfo().target == dyingPlayer &&
              !state->pendingActionInfo().remainingTargets.empty() &&
              (state->pendingActionInfo().requiredCardType == CardType::Kill ||
               state->pendingActionInfo().requiredCardType == CardType::Dodge)) ||
             (hasDeferredSkill &&
              !state->pendingActionInfo().continuationTargets.empty()));
    if (hasAoeContinuation || hasDeferredSkill) {
        previousAction = state->pendingActionInfo();
    }

    std::vector<Player*> saviors = dyingSaviors(state, dyingPlayer);
    if (saviors.empty()) {
        checkDeath(state, dyingPlayer);
        return;
    }

    Player* firstSavior = saviors.front();

    PendingActionInfo info;
    info.source = firstSavior;
    info.target = dyingPlayer;
    info.sourceCard = nullptr;
    info.requiredCardType = CardType::Peach;
    info.description = (dyingPlayer->displayName() + " 濒死，" +
            firstSavior->displayName() +
            (firstSavior == dyingPlayer ? " 可以使用【桃】或【酒】"
                                        : " 可以使用【桃】")).toStdString();
    info.canSkip = true;
    if (hasAoeContinuation) {
        if (hasDeferredSkill) {
            info.continuationSource = previousAction.continuationSource;
            info.continuationSourceCard = previousAction.continuationSourceCard;
            info.continuationCardType = previousAction.continuationCardType;
            info.continuationTargets = previousAction.continuationTargets;
        } else {
            info.continuationSource = previousAction.source;
            info.continuationSourceCard = previousAction.sourceCard;
            info.continuationCardType = previousAction.requiredCardType;
            info.continuationTargets = previousAction.remainingTargets;
        }
    }
    if (hasDeferredSkill) {
        info.deferredSkillPlayer = dyingPlayer;
        info.deferredSkillCard = previousAction.sourceCard;
    }

    state->setPendingAction(info);
}

bool handleDyingPeach(GameState* state, Player* dyingPlayer, Player* peachUser, Card* peachCard)
{
    if (!state || !dyingPlayer || !peachUser || !peachCard) return false;
    if (!state->hasPendingAction()) return false;

    const PendingActionInfo info = state->pendingActionInfo();
    if (info.requiredCardType != CardType::Peach ||
        info.source != peachUser || info.target != dyingPlayer ||
        !peachUser->hasCard(peachCard) ||
        (peachCard->cardType() != CardType::Peach &&
         (peachCard->cardType() != CardType::Wine || peachUser != dyingPlayer))) {
        return false;
    }

    peachUser->removeHandCard(peachCard);
    if (state->cardManager()) {
        state->cardManager()->discard(peachCard);
    }

    if (peachCard->cardType() == CardType::Peach) {
        dyingPlayer->heal(1);
    } else if (peachCard->cardType() == CardType::Wine) {
        dyingPlayer->heal(1);
    }

    if (dyingPlayer->hp() > 0) {
        dyingPlayer->setDying(false);
        state->clearPendingAction();

        if (info.deferredSkillPlayer && info.deferredSkillCard &&
            state->cardManager() &&
            state->cardManager()->isInDiscardPile(info.deferredSkillCard)) {
            PendingActionInfo skill;
            skill.source = info.deferredSkillPlayer;
            skill.target = info.deferredSkillPlayer;
            skill.sourceCard = info.deferredSkillCard;
            skill.description = (info.deferredSkillPlayer->displayName() +
                    " 可以发动【奸雄】获得造成伤害的牌").toStdString();
            skill.canSkip = true;
            skill.isSkillChoice = true;
            skill.continuationSource = info.continuationSource;
            skill.continuationSourceCard = info.continuationSourceCard;
            skill.continuationCardType = info.continuationCardType;
            skill.continuationTargets = info.continuationTargets;
            state->setPendingAction(skill);
        } else {
            resumeAoeContinuation(state, info);
        }
        return true;
    }

    return false;
}

void skipDyingResponse(GameState* state, Player* dyingPlayer)
{
    if (!state || !dyingPlayer) return;
    if (!state->hasPendingAction()) return;

    const PendingActionInfo& info = state->pendingActionInfo();
    if (info.requiredCardType != CardType::Peach || info.target != dyingPlayer) return;
    Player* currentSavior = info.source;

    std::vector<Player*> allPlayers = dyingSaviors(state, dyingPlayer);
    auto it = std::find(allPlayers.begin(), allPlayers.end(), currentSavior);
    int currentIndex = (it != allPlayers.end())
                        ? static_cast<int>(std::distance(allPlayers.begin(), it))
                        : -1;

    if (currentIndex >= 0 && currentIndex + 1 < static_cast<int>(allPlayers.size())) {
        Player* nextSavior = allPlayers[currentIndex + 1];

        PendingActionInfo nextInfo;
        nextInfo.source = nextSavior;
        nextInfo.target = dyingPlayer;
        nextInfo.sourceCard = nullptr;
        nextInfo.requiredCardType = CardType::Peach;
        nextInfo.description = (dyingPlayer->displayName() + " 濒死，" +
                nextSavior->displayName() +
                (nextSavior == dyingPlayer ? " 可以使用【桃】或【酒】"
                                           : " 可以使用【桃】")).toStdString();
        nextInfo.canSkip = true;
        nextInfo.continuationSource = info.continuationSource;
        nextInfo.continuationSourceCard = info.continuationSourceCard;
        nextInfo.continuationCardType = info.continuationCardType;
        nextInfo.continuationTargets = info.continuationTargets;
        nextInfo.deferredSkillPlayer = info.deferredSkillPlayer;
        nextInfo.deferredSkillCard = info.deferredSkillCard;

        state->setPendingAction(nextInfo);
    } else {
        checkDeath(state, dyingPlayer);
    }
}

void checkDeath(GameState* state, Player* player)
{
    if (!state || !player) return;

    if (player->hp() <= 0) {
        player->setDying(false);
        player->markDead();
        if (state->hasPendingAction()) {
            state->clearPendingAction();
        }
        checkGameOver(state);
    }
}

void checkGameOver(GameState* state)
{
    if (!state) return;

    std::vector<Player*> alive = state->alivePlayers();

    if (alive.size() == 1) {
        state->setGameOver(alive.front());
    } else if (alive.empty()) {
        state->setGameOver(nullptr);
    }
}

bool handleSkillChoice(GameState* state, Player* player, bool accept)
{
    if (!state || !player || !state->hasPendingAction()) return false;

    const PendingActionInfo info = state->pendingActionInfo();
    if (!info.isSkillChoice || info.target != player || !info.sourceCard) return false;

    state->clearPendingAction();
    bool obtained = false;
    if (accept && state->cardManager() &&
        state->cardManager()->takeFromDiscard(info.sourceCard)) {
        player->addHandCard(info.sourceCard);
        obtained = true;
        if (player->character()) {
            emit player->character()->skillTriggered(
                    QString::fromStdString(player->character()->skillName()));
        }
    }

    resumeAoeContinuation(state, info);
    return obtained;
}

// ==================== 弃牌阶段 ====================

int getDiscardCount(const Player* player)
{
    if (!player) return 0;

    int handCount = player->handCardCount();
    int limit = handLimit(player);

    return std::max(0, handCount - limit);
}

// ==================== 装备与距离 ====================

int getAttackRange(const GameState* state, const Player* attacker)
{
    (void)state;
    if (!attacker) return 1;
    return attacker->attackRange();
}

bool isInAttackRange(const GameState* state, const Player* attacker, const Player* target)
{
    if (!state || !attacker || !target) return false;
    int range = getAttackRange(state, attacker);
    // 简化：两人游戏，距离始终为 1
    // TODO: 多人游戏时需计算座位距离
    (void)range;
    return true;
}

bool armorEffectCheck(const GameState* state, const Player* defender, Card* attackCard)
{
    (void)state;
    if (!defender || !attackCard) return false;

    EquipmentCard* armor = defender->equippedAt(EquipSlot::Armor);
    if (!armor) return false;

    // 仁王盾：黑色杀无效
    if (armor->cardType() == CardType::BenevolentShield && attackCard->isBlack()) {
        return true;  // 防具生效，杀被抵消
    }

    return false;
}

// ==================== 新锦囊执行 ====================

void executeDuel(GameState* state, Player* user, Player* target, Card* duelCard)
{
    if (!state || !user || !target || !duelCard ||
        duelCard->cardType() != CardType::Duel) return;

    PendingActionInfo info;
    info.source = user;
    info.target = target;
    info.sourceCard = duelCard;
    info.requiredCardType = CardType::Kill;
    info.description = (user->displayName() + " 对 " + target->displayName()
                       + " 使用了【决斗】，请打出【杀】").toStdString();
    info.canSkip = true;
    info.isDuel = true;

    state->setPendingAction(info);
}

void handleDuelResponse(GameState* state, Player* responder, Card* killCard)
{
    if (!state || !responder || !state->hasPendingAction()) return;

    PendingActionInfo info = state->pendingActionInfo();
    if (info.requiredCardType != CardType::Kill || info.target != responder ||
        !info.source || !info.isDuel || !info.sourceCard ||
        info.sourceCard->cardType() != CardType::Duel) {
        return;
    }
    if (killCard && (!responder->hasCard(killCard) ||
                     (killCard->cardType() != CardType::Kill &&
                      (!responder->character() ||
                       responder->character()->skillTransformCard(killCard) != CardType::Kill)))) {
        return;
    }

    if (!killCard) {
        dealDamage(state, responder, 1, info.source, info.sourceCard);
        // dealDamage() may replace the duel response with a dying response.
        if (!responder->isDying() && state->hasPendingAction()) {
            const PendingActionInfo& current = state->pendingActionInfo();
            if (current.source == info.source && current.target == info.target &&
                current.sourceCard == info.sourceCard &&
                current.requiredCardType == CardType::Kill && current.isDuel) {
                state->clearPendingAction();
            }
        }
        return;
    }

    responder->removeHandCard(killCard);
    if (state->cardManager()) {
        state->cardManager()->discard(killCard);
    }

    state->clearPendingAction();

    PendingActionInfo nextInfo;
    nextInfo.source = responder;
    nextInfo.target = info.source;
    nextInfo.sourceCard = info.sourceCard;
    nextInfo.requiredCardType = CardType::Kill;
    nextInfo.description = (responder->displayName() + " 在【决斗】中打出【杀】，"
                           + info.source->displayName() + " 需继续打出【杀】").toStdString();
    nextInfo.canSkip = true;
    nextInfo.isDuel = true;
    state->setPendingAction(nextInfo);
}

bool hasNullifyToRespond(const Player* player)
{
    if (!player || !player->isAlive()) return false;
    for (Card* card : player->handCards()) {
        if (!card) continue;
        if (card->cardType() == CardType::Nullify ||
            (player->character() &&
             player->character()->skillTransformCard(card) == CardType::Nullify)) {
            return true;
        }
    }
    return false;
}

bool checkNullifyBeforeEffect(GameState* state, Card* strategyCard,
                               Player* target, Player* source,
                               std::function<void()> onSkip)
{
    if (!state || !strategyCard || !target || !source) return false;
    if (!hasNullifyToRespond(target)) return false;

    PendingActionInfo info;
    info.source = source;
    info.target = target;
    info.sourceCard = strategyCard;
    info.requiredCardType = CardType::Nullify;
    info.description = (target->displayName() + " 可以使用【无懈可击】抵消 "
                        + source->displayName() + " 的【"
                        + QString::fromStdString(strategyCard->cardName()) + "】").toStdString();
    info.canSkip = true;
    info.onNullifySkipped = std::move(onSkip);

    state->setPendingAction(info);
    return true;
}

void handleNullifyResponse(GameState* state, Player* responder,
                            Card* nullifyCard, bool usedNullify)
{
    if (!state || !responder || !state->hasPendingAction()) return;

    PendingActionInfo info = state->pendingActionInfo();
    if (info.requiredCardType != CardType::Nullify || info.target != responder) return;

    if (usedNullify) {
        if (!nullifyCard || !responder->hasCard(nullifyCard) ||
            (nullifyCard->cardType() != CardType::Nullify &&
             (!responder->character() ||
              responder->character()->skillTransformCard(nullifyCard) != CardType::Nullify))) {
            return;
        }

        // 使用了无懈可击：消除锦囊效果，无懈可击进弃牌堆
        responder->removeHandCard(nullifyCard);
        if (state->cardManager()) {
            state->cardManager()->discard(nullifyCard);
        }
        state->clearPendingAction();

        // 普通锦囊通常已在 ActionViewModel 中进入弃牌堆；延时锦囊会
        // 等无懈结果，此处统一补入。CardManager 会忽略重复指针。
        if (state->cardManager() && info.sourceCard) {
            state->cardManager()->discard(info.sourceCard);
        }

        // TODO: 实现连锁——锦囊来源方也可使用无懈反无懈
    } else {
        // 放弃使用无懈可击：执行被延迟的锦囊效果
        state->clearPendingAction();
        if (info.onNullifySkipped) {
            info.onNullifySkipped();
        }
    }
}

bool checkNullifyChain(GameState* state, Card* targetCard,
                        Player* targetPlayer, Player* sourcePlayer)
{
    (void)state;
    (void)targetCard;
    (void)targetPlayer;
    (void)sourcePlayer;
    // checkNullifyBeforeEffect 已在各锦囊 execute 中直接调用
    // 此函数保留作为扩展点（如 multiple target AOE 的集中判定）
    return false;
}

void executeBorrow(GameState* state, Player* user, Player* target)
{
    if (!state || !user || !target) return;

    // 令目标对一名角色使用杀（若目标有武器）
    // 简化实现（双人）：目标需对借刀使用者出杀，否则将武器交给使用者
    PendingActionInfo info;
    info.source = user;
    info.target = target;
    info.sourceCard = nullptr;
    info.requiredCardType = CardType::Kill;
    info.description = (user->displayName() + " 对 " + target->displayName()
                       + " 使用了【借刀杀人】，请使用【杀】").toStdString();
    info.canSkip = true;
    info.isBorrow = true;

    state->setPendingAction(info);
}

void handleBorrowResponse(GameState* state, Player* responder, Card* killCard)
{
    if (!state || !responder || !state->hasPendingAction()) return;

    // 复制一份，避免 clearPendingAction() 后引用失效
    PendingActionInfo info = state->pendingActionInfo();
    if (!info.isBorrow || info.requiredCardType != CardType::Kill ||
        info.target != responder || !info.source) {
        return;
    }
    if (killCard && (!responder->hasCard(killCard) ||
                     (killCard->cardType() != CardType::Kill &&
                      (!responder->character() ||
                       responder->character()->skillTransformCard(killCard) != CardType::Kill)))) {
        return;
    }

    state->clearPendingAction();

    if (killCard) {
        // 出杀：双人下杀作用于借刀使用者
        responder->removeHandCard(killCard);
        if (state->cardManager()) {
            state->cardManager()->discard(killCard);
        }
        dealDamage(state, info.source, 1, responder, killCard);
    } else {
        // 不出杀：将武器交给借刀使用者（无武器则无事发生）
        EquipmentCard* weapon = responder->equippedAt(EquipSlot::Weapon);
        if (weapon) {
            responder->unequipSlot(EquipSlot::Weapon);
            // 使用者原武器（若有）进弃牌堆，再装上移交的武器
            EquipmentCard* oldWeapon = info.source->equippedAt(EquipSlot::Weapon);
            if (oldWeapon && state->cardManager()) {
                info.source->unequipSlot(EquipSlot::Weapon);
                state->cardManager()->discard(oldWeapon);
            }
            info.source->equipCard(weapon);
        }
    }
}

void executeHarvest(GameState* state, Player* user)
{
    if (!state || !user || !state->cardManager()) return;

    int aliveCount = static_cast<int>(state->alivePlayers().size());
    std::vector<Card*> revealed = state->cardManager()->drawCards(aliveCount);

    if (revealed.empty()) return;

    // 简化实现：展示的牌由使用者获得一张，其余弃置
    if (!revealed.empty()) {
        Card* chosen = revealed.front();
        user->addHandCard(chosen);
        for (size_t i = 1; i < revealed.size(); ++i) {
            if (revealed[i]) {
                state->cardManager()->discard(revealed[i]);
            }
        }
    }
}

} // namespace GameRule
