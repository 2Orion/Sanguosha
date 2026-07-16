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

        state->clearPendingAction();
    } else {
        int damageValue = 1;
        if (attacker && attacker->isWineEnhanced()) {
            damageValue = 2;
            attacker->setWineEnhanced(false);
        }

        dealDamage(state, responder, damageValue, attacker);
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
    info.description = (user->displayName() + " 使用了【南蛮入侵】，" + firstTarget->displayName() + " 需打出【杀】").toStdString();
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
        dealDamage(state, responder, 1, info.source);
        if (responder->isDying() || !state->hasPendingAction()) return;
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
        dealDamage(state, responder, 1, info.source);
        if (responder->isDying() || !state->hasPendingAction()) return;
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

    PendingActionInfo previousAction;
    const bool hasAoeContinuation = state->hasPendingAction() &&
            state->pendingActionInfo().target == dyingPlayer &&
            !state->pendingActionInfo().remainingTargets.empty() &&
            (state->pendingActionInfo().requiredCardType == CardType::Kill ||
             state->pendingActionInfo().requiredCardType == CardType::Dodge);
    if (hasAoeContinuation) {
        previousAction = state->pendingActionInfo();
    }

    std::vector<Player*> saviors = state->alivePlayers();
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
    info.description = (dyingPlayer->displayName() + " 濒死，" + firstSavior->displayName() + " 可以使用【桃】或【酒】").toStdString();
    info.canSkip = true;
    if (hasAoeContinuation) {
        info.continuationSource = previousAction.source;
        info.continuationCardType = previousAction.requiredCardType;
        info.continuationTargets = previousAction.remainingTargets;
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
         peachCard->cardType() != CardType::Wine)) {
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

        std::vector<Player*> remainingTargets;
        for (Player* player : info.continuationTargets) {
            if (player && player->isAlive()) remainingTargets.push_back(player);
        }
        if (info.continuationSource && !remainingTargets.empty()) {
            Player* nextTarget = remainingTargets.front();
            PendingActionInfo nextInfo;
            nextInfo.source = info.continuationSource;
            nextInfo.target = nextTarget;
            nextInfo.sourceCard = nullptr;
            nextInfo.requiredCardType = info.continuationCardType;
            nextInfo.description = (info.continuationSource->displayName() +
                    (info.continuationCardType == CardType::Kill
                         ? " 使用了【南蛮入侵】，" : " 使用了【万箭齐发】，") +
                    nextTarget->displayName() +
                    (info.continuationCardType == CardType::Kill
                         ? " 需打出【杀】" : " 需打出【闪】")).toStdString();
            nextInfo.canSkip = true;
            nextInfo.remainingTargets = std::vector<Player*>(remainingTargets.begin() + 1,
                                                               remainingTargets.end());
            state->setPendingAction(nextInfo);
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
        nextInfo.description = (dyingPlayer->displayName() + " 濒死，" + nextSavior->displayName() + " 可以使用【桃】或【酒】").toStdString();
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

void executeDuel(GameState* state, Player* user, Player* target)
{
    if (!state || !user || !target) return;

    PendingActionInfo info;
    info.source = user;
    info.target = target;
    info.sourceCard = nullptr;
    info.requiredCardType = CardType::Kill;
    info.description = (user->displayName() + " 对 " + target->displayName()
                       + " 使用了【决斗】，请打出【杀】").toStdString();
    info.canSkip = true;
    info.isDuel = true;

    state->setPendingAction(info);
}

/// 处理决斗回合中的出杀响应
void handleDuelKillResponse(GameState* state, Player* responder, Card* killCard)
{
    if (!state || !responder || !killCard) return;

    responder->removeHandCard(killCard);
    if (state->cardManager()) {
        state->cardManager()->discard(killCard);
    }
    if (!state->hasPendingAction()) return;

    const PendingActionInfo& prev = state->pendingActionInfo();

    // 判断下一位出杀者：当前是谁出的杀，对面的那位就是下一位
    Player* nextResponder = (responder == prev.target) ? prev.source : prev.target;

    state->clearPendingAction();

    PendingActionInfo next;
    next.source = prev.source;
    next.target = prev.target;
    next.requiredCardType = CardType::Kill;
    next.description = (QStringLiteral("【决斗】轮到你出【杀】— ") +
                        nextResponder->displayName()).toStdString();
    next.canSkip = true;
    next.isDuel = true;

    state->setPendingAction(next);
}

/// 处理决斗回合中放弃出杀（受到 1 点伤害）
void handleDuelSkipResponse(GameState* state, Player* responder)
{
    if (!state || !responder) return;
    if (!state->hasPendingAction()) return;

    const PendingActionInfo& prev = state->pendingActionInfo();

    // 伤害来源是对方玩家
    Player* damager = (responder == prev.target) ? prev.source : prev.target;

    state->clearPendingAction();
    dealDamage(state, responder, 1, damager);
}

void executeLightning(GameState* state, Player* user, Player* target)
{
    if (!state || !user || !target) return;
    // 闪电放入目标判定区，在判定阶段结算
    // 简化实现：直接对当前使用者进行判定
    (void)target;
    target->addJudgmentCard(nullptr);  // 实际应由 Card 对象传入
    // TODO: 在 Judge 阶段实现完整的闪电结算（判定 + 伤害/转移）
}

void executeNullify(GameState* state, Player* user)
{
    // 无懈可击由 checkNullifyChain 在锦囊结算前触发
    (void)state;
    (void)user;
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
    if (!state || !responder) return;
    if (!state->hasPendingAction()) return;

    PendingActionInfo info = state->pendingActionInfo();

    if (usedNullify && nullifyCard) {
        // 使用了无懈可击：消除锦囊效果，无懈可击进弃牌堆
        responder->removeHandCard(nullifyCard);
        if (state->cardManager()) {
            state->cardManager()->discard(nullifyCard);
        }
        state->clearPendingAction();

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
    // 简化实现：目标必须对使用者之外的另一名角色出杀
    PendingActionInfo info;
    info.source = user;
    info.target = target;
    info.sourceCard = nullptr;
    info.requiredCardType = CardType::Kill;
    info.description = (user->displayName() + " 对 " + target->displayName()
                       + " 使用了【借刀杀人】，请使用【杀】").toStdString();
    info.canSkip = true;

    state->setPendingAction(info);
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

void executeHappy(GameState* state, Player* user, Player* target)
{
    if (!state || !user || !target) return;
    // 乐不思蜀放入目标判定区
    // 简化实现：直接标记，在 Judge 阶段处理
    // 实际应由 Card 对象传入后放入 judgmentCards
}

void executeFamine(GameState* state, Player* user, Player* target)
{
    if (!state || !user || !target) return;
    // 兵粮寸断放入目标判定区
    // 简化实现：直接标记，在 Judge 阶段处理
}

} // namespace GameRule
