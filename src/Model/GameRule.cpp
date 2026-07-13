#include "GameRule.h"
#include "GameState.h"
#include "Player.h"
#include "Card.h"
#include "Character.h"
#include "CardManager.h"
#include <algorithm>

namespace GameRule {

// ==================== 规则判断 ====================

bool canPlayKill(const GameState* state, const Player* player)
{
    if (!state || !player) return false;

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
                     card->cardType() == CardType::Wine)) {
            return true;
        }
    }

    return false;
}

// ==================== 卡牌执行 ====================

void executeKill(GameState* state, Player* user, Player* target)
{
    if (!state || !user || !target) return;

    user->setUsedKillThisTurn(true);

    PendingActionInfo info;
    info.source = user;
    info.target = target;
    info.sourceCard = nullptr;
    info.requiredCardType = CardType::Dodge;
    info.description = user->displayName() + " 对 " + target->displayName() + " 使用了【杀】，请打出【闪】";
    info.canSkip = false;

    state->setPendingAction(info);
}

void handleKillResponse(GameState* state, Player* responder, Card* dodgeCard)
{
    if (!state || !responder) return;

    const PendingActionInfo& info = state->pendingActionInfo();
    Player* attacker = info.source;

    if (dodgeCard) {
        responder->removeHandCard(dodgeCard);
        if (state->cardManager()) {
            state->cardManager()->discard(dodgeCard);
        }
        state->clearPendingAction();
    } else {
        int damageValue = 1;
        if (attacker && attacker->isWineEnhanced()) {
            damageValue = 2;
            attacker->setWineEnhanced(false);
        }

        dealDamage(state, responder, damageValue, attacker);
        state->clearPendingAction();
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

void executeBarbarianInvasion(GameState* state, Player* user)
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
    info.sourceCard = nullptr;
    info.requiredCardType = CardType::Kill;
    info.description = user->displayName() + " 使用了【南蛮入侵】，" + firstTarget->displayName() + " 需打出【杀】";
    info.canSkip = true;
    info.remainingTargets = std::vector<Player*>(targets.begin() + 1, targets.end());

    state->setPendingAction(info);
}

void executeVolley(GameState* state, Player* user)
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
    info.sourceCard = nullptr;
    info.requiredCardType = CardType::Dodge;
    info.description = user->displayName() + " 使用了【万箭齐发】，" + firstTarget->displayName() + " 需打出【闪】";
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

    // 复制一份，避免 clearPendingAction() 重置内部状态后引用失效
    PendingActionInfo info = state->pendingActionInfo();

    if (killCard) {
        responder->removeHandCard(killCard);
        if (state->cardManager()) {
            state->cardManager()->discard(killCard);
        }
    } else {
        dealDamage(state, responder, 1, info.source);
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
        nextInfo.sourceCard = nullptr;
        nextInfo.requiredCardType = CardType::Kill;
        nextInfo.description = info.source->displayName() + " 使用了【南蛮入侵】，" + nextTarget->displayName() + " 需打出【杀】";
        nextInfo.canSkip = true;
        nextInfo.remainingTargets = std::vector<Player*>(remainingTargets.begin() + 1, remainingTargets.end());

        state->setPendingAction(nextInfo);
    }
}

void handleAoeDodgeResponse(GameState* state, Player* responder, Card* dodgeCard)
{
    if (!state || !responder) return;

    // 复制一份，避免 clearPendingAction() 重置内部状态后引用失效
    PendingActionInfo info = state->pendingActionInfo();

    if (dodgeCard) {
        responder->removeHandCard(dodgeCard);
        if (state->cardManager()) {
            state->cardManager()->discard(dodgeCard);
        }
    } else {
        dealDamage(state, responder, 1, info.source);
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
        nextInfo.sourceCard = nullptr;
        nextInfo.requiredCardType = CardType::Dodge;
        nextInfo.description = info.source->displayName() + " 使用了【万箭齐发】，" + nextTarget->displayName() + " 需打出【闪】";
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

void dealDamage(GameState* state, Player* target, int value, Player* source)
{
    if (!state || !target || value <= 0) return;

    target->damage(value);

    if (target->character() && source) {
        Character* character = target->character();
        if (character->triggerCondition(GameEvent::OnDamage, state, target)) {
            character->triggerSkill(state, target);
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

    std::vector<Player*> saviors = state->alivePlayers();
    if (saviors.empty()) return;

    Player* firstSavior = saviors.front();

    PendingActionInfo info;
    info.source = firstSavior;
    info.target = dyingPlayer;
    info.sourceCard = nullptr;
    info.requiredCardType = CardType::Peach;
    info.description = dyingPlayer->displayName() + " 濒死，" + firstSavior->displayName() + " 可以使用【桃】或【酒】";
    info.canSkip = true;

    state->setPendingAction(info);
}

bool handleDyingPeach(GameState* state, Player* dyingPlayer, Player* peachUser, Card* peachCard)
{
    if (!state || !dyingPlayer || !peachUser || !peachCard) return false;

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
        return true;
    }

    return false;
}

void skipDyingResponse(GameState* state, Player* dyingPlayer)
{
    if (!state || !dyingPlayer) return;

    const PendingActionInfo& info = state->pendingActionInfo();
    Player* currentSavior = info.source;

    std::vector<Player*> allPlayers = state->alivePlayers();
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
        nextInfo.description = dyingPlayer->displayName() + " 濒死，" + nextSavior->displayName() + " 可以使用【桃】或【酒】";
        nextInfo.canSkip = true;

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
        player->died.notify(player);
        state->clearPendingAction();
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

// ==================== 弃牌阶段 ====================

int getDiscardCount(const Player* player)
{
    if (!player) return 0;

    int handCount = player->handCardCount();
    int limit = handLimit(player);

    return std::max(0, handCount - limit);
}

} // namespace GameRule
