#include "ActionViewModel.h"
#include "GameState.h"
#include "GameRule.h"
#include "CardManager.h"
#include "Player.h"
#include "Card.h"
#include "Character.h"
#include <algorithm>
#include <unordered_set>

namespace {

bool isResponseType(const Card* card, const Player* player, CardType requiredType)
{
    if (!card) return false;
    if (card->cardType() == requiredType) return true;
    if (requiredType == CardType::Peach && card->cardType() == CardType::Wine) return true;

    return player && player->character() &&
           player->character()->skillTransformCard(card) == requiredType;
}

bool requiresExplicitTarget(const Card* card)
{
    if (!card) return false;

    switch (card->cardType()) {
    case CardType::Kill:
    case CardType::Duel:
    case CardType::Dismantle:
    case CardType::Steal:
        return true;
    default:
        return false;
    }
}

bool isDuelResponse(const PendingActionInfo& info)
{
    return info.isDuel ||
           (info.sourceCard && info.sourceCard->cardType() == CardType::Duel);
}

bool isDelayedStrategy(CardType type)
{
    return type == CardType::Happy || type == CardType::Famine ||
           type == CardType::Lightning;
}

} // namespace

ActionViewModel::ActionViewModel(QObject* parent)
    : QObject(parent)
{
}

void ActionViewModel::setGameState(GameState* state) { m_state = state; }

// ==================== 内部 Model 对象查找 ====================

Player* ActionViewModel::findPlayer(int playerId) const
{
    return m_state ? m_state->player(playerId) : nullptr;
}

Card* ActionViewModel::findCard(int cardId) const
{
    if (!m_state) return nullptr;
    for (Player* p : m_state->allPlayers()) {
        if (!p) continue;
        for (Card* c : p->handCards()) {
            if (c && c->id() == cardId) return c;
        }
    }
    return nullptr;
}

// ==================== 出牌阶段（公共 ID 接口） ====================

std::vector<int> ActionViewModel::getPlayableCardIds(int playerId) const
{
    std::vector<int> result;
    Player* player = findPlayer(playerId);
    if (!player) return result;
    for (Card* card : getPlayableCards(player)) {
        if (card) result.push_back(card->id());
    }
    return result;
}

std::vector<int> ActionViewModel::getValidTargetIds(int cardId, int playerId) const
{
    Card* card = findCard(cardId);
    Player* user = findPlayer(playerId);
    if (!m_state || !card || !user || !isOwnCard(cardId, playerId)) return {};

    // 技能转化为【杀】使用时，目标规则按杀处理（除自己外的存活角色）
    if (playsAsKill(card, user)) {
        std::vector<int> result;
        for (Player* p : m_state->alivePlayers()) {
            if (p && p != user) result.push_back(p->playerId());
        }
        return result;
    }

    std::vector<Player*> targets = card->getValidTargets(m_state, user);
    std::vector<int> result;
    for (Player* t : targets) {
        if (t) result.push_back(t->playerId());
    }
    return result;
}

bool ActionViewModel::isOwnCard(int cardId, int playerId) const
{
    Player* player = findPlayer(playerId);
    if (!player) return false;
    for (Card* c : player->handCards()) {
        if (c && c->id() == cardId) return true;
    }
    return false;
}

bool ActionViewModel::canPlayCard(int cardId, int playerId) const
{
    Card* card = findCard(cardId);
    Player* player = findPlayer(playerId);
    if (!m_state || !card || !player || !isOwnCard(cardId, playerId)) return false;

    // 出牌只能是当前回合玩家：card->canUse() 各子类只查全局 currentPhase()，
    // 不查 player 是不是 currentPlayer()，本地模式靠 View 只让当前玩家手牌
    // 可交互侥幸掩盖，网络模式下非当前回合玩家会用自己真实身份出牌。
    if (player != m_state->currentPlayer()) return false;

    // 待定响应（如杀等待闪、南蛮/万箭链）尚未结算时不能再出牌：GameState::
    // setPendingAction 无重入保护，第二张主动牌会通过 executeKill 等静默
    // 覆盖第一个待定动作，原响应永久悬空。
    if (m_state->hasPendingAction()) return false;

    bool playable = (card->canUse(m_state, player) &&
                     GameRule::canPlayCard(m_state, player, card)) ||
                    playsAsKill(card, player);
    if (!playable) return false;

    return !getValidTargetIds(cardId, playerId).empty();
}

ActionResult ActionViewModel::playCard(int cardId, int playerId,
                                        const std::vector<int>& targetIds)
{
    Card* card = findCard(cardId);
    Player* user = findPlayer(playerId);
    if (!m_state || !card || !user || !isOwnCard(cardId, playerId) ||
        !canPlayCard(cardId, playerId)) {
        return ActionResult::Completed;
    }

    const std::vector<int> validTargetIds = getValidTargetIds(cardId, playerId);
    if (targetIds.size() > 1) return ActionResult::Completed;
    for (int targetId : targetIds) {
        if (std::find(validTargetIds.begin(), validTargetIds.end(), targetId) == validTargetIds.end()) {
            return ActionResult::Completed;
        }
    }
    if (targetIds.empty() && requiresExplicitTarget(card)) return ActionResult::Completed;

    std::vector<Player*> targets;
    for (int tid : targetIds) {
        Player* t = findPlayer(tid);
        if (!t) return ActionResult::Completed;
        targets.push_back(t);
    }

    // 技能转化为【杀】使用：不执行原牌效果，走杀的结算（关羽武圣 / 赵云龙胆）
    if (playsAsKill(card, user)) {
        if (targets.empty()) return ActionResult::Completed;
        GameRule::executeKill(m_state, user, targets.front());
        user->removeHandCard(card);
        if (m_state->cardManager()) m_state->cardManager()->discard(card);

        QString skillName = user->character()
                ? QString::fromStdString(user->character()->skillName()) : QString();
        emitLog(user->displayName() + QStringLiteral(" 发动【") + skillName +
                QStringLiteral("】，将【") + QString::fromStdString(card->cardName()) +
                QStringLiteral("】当【杀】使用 → ") + targets.front()->displayName());

        emit actionCompleted();
        return ActionResult::RequiresDodge;
    }

    // 装备牌：execute 内部已处理装备逻辑，不从手牌移除（由 equipCard 内部处理），
    // 也不需要进弃牌堆（牌已装备到装备区）
    bool isEquip = card->isEquipment();
    EquipmentCard* oldEquip = nullptr;
    if (isEquip) {
        EquipmentCard* eq = dynamic_cast<EquipmentCard*>(card);
        if (eq) {
            EquipSlot slot = eq->equipSlot();
            oldEquip = user->equippedAt(slot);
        }
    }

    ActionResult result = card->execute(m_state, user, targets);

    if (isEquip) {
        // 装备牌：从手牌移除，但不进弃牌堆
        user->removeHandCard(card);
        // 旧装备进弃牌堆
        if (oldEquip && m_state->cardManager()) {
            m_state->cardManager()->discard(oldEquip);
        }
    } else if (isDelayedStrategy(card->cardType())) {
        // 延时锦囊已经进入判定区，或正在等待【无懈可击】响应。
        // 真正被抵消/判定完成时再进入弃牌堆。
        user->removeHandCard(card);
    } else {
        user->removeHandCard(card);
        if (m_state->cardManager()) m_state->cardManager()->discard(card);
    }

    QString targetStr;
    for (size_t i = 0; i < targets.size(); ++i) {
        if (i > 0) targetStr += QStringLiteral("、");
        targetStr += targets[i]->displayName();
    }
    emitLog(user->displayName() + QStringLiteral(" 使用了【") +
            QString::fromStdString(card->cardName()) + QStringLiteral("】") +
            (targetStr.isEmpty() ? QString() : QStringLiteral(" → ") + targetStr));

    emit actionCompleted();
    return result;
}

// ==================== 响应阶段 ====================

std::vector<int> ActionViewModel::getResponseCardIds(int playerId, CardType requiredType) const
{
    std::vector<int> result;
    Player* player = findPlayer(playerId);
    if (!player) return result;

    for (Card* card : player->handCards()) {
        if (!card) continue;
        if (isResponseType(card, player, requiredType)) result.push_back(card->id());
    }
    return result;
}

bool ActionViewModel::isValidResponseCard(int cardId, int playerId, CardType requiredType) const
{
    auto ids = getResponseCardIds(playerId, requiredType);
    return std::find(ids.begin(), ids.end(), cardId) != ids.end();
}

bool ActionViewModel::canSkipPendingAction() const
{
    return m_state && m_state->hasPendingAction() && m_state->pendingActionInfo().canSkip;
}

void ActionViewModel::respondCard(int cardId, int playerId)
{
    Card* card = findCard(cardId);
    Player* responder = findPlayer(playerId);
    if (!m_state || !card || !responder || !m_state->hasPendingAction()) return;

    const PendingActionInfo info = m_state->pendingActionInfo();
    Player* expectedResponder = (info.requiredCardType == CardType::Peach)
            ? info.source : info.target;
    if (expectedResponder != responder ||
        (info.requiredCardType == CardType::Peach && !info.target) ||
        !isValidResponseCard(cardId, playerId, info.requiredCardType)) {
        return;
    }

    switch (info.requiredCardType) {
    case CardType::Dodge:
        if (info.canSkip) {
            GameRule::handleAoeDodgeResponse(m_state, responder, card);
            emitLog(responder->displayName() + QStringLiteral(" 打出【") +
                    QString::fromStdString(card->cardName()) + QStringLiteral("】响应万箭齐发"));
        } else {
            GameRule::handleKillResponse(m_state, responder, card);
            emitLog(responder->displayName() + QStringLiteral(" 打出【") +
                    QString::fromStdString(card->cardName()) + QStringLiteral("】响应杀"));
        }
        break;

    case CardType::Kill:
        if (isDuelResponse(info)) {
            GameRule::handleDuelResponse(m_state, responder, card);
            emitLog(responder->displayName() + QStringLiteral(" 打出【") +
                    QString::fromStdString(card->cardName()) + QStringLiteral("】响应决斗"));
        } else {
            GameRule::handleAoeKillResponse(m_state, responder, card);
            emitLog(responder->displayName() + QStringLiteral(" 打出【") +
                    QString::fromStdString(card->cardName()) + QStringLiteral("】响应南蛮入侵"));
        }
        break;

    case CardType::Peach: {
        Player* dyingPlayer = info.target;
        bool saved = GameRule::handleDyingPeach(m_state, dyingPlayer, responder, card);
        emitLog(responder->displayName() + QStringLiteral(" 对 ") +
                dyingPlayer->displayName() + QStringLiteral(" 使用【") +
                QString::fromStdString(card->cardName()) + QStringLiteral("】") +
                (saved ? QStringLiteral("，救回！") : QStringLiteral("，仍需继续救援")));
        break;
    }

    case CardType::Nullify:
        GameRule::handleNullifyResponse(m_state, responder, card, true);
        emitLog(responder->displayName() + QStringLiteral(" 使用【") +
                QString::fromStdString(card->cardName()) + QStringLiteral("】抵消锦囊"));
        break;

    default:
        break;
    }

    emit actionCompleted();
}

void ActionViewModel::skipResponse(int playerId, bool forceNoCard)
{
    Player* responder = findPlayer(playerId);
    if (!m_state || !responder || !m_state->hasPendingAction()) return;

    const PendingActionInfo info = m_state->pendingActionInfo();
    Player* expectedResponder = (info.requiredCardType == CardType::Peach)
            ? info.source : info.target;
    if (expectedResponder != responder ||
        (info.requiredCardType == CardType::Peach && !info.target)) return;
    if (!forceNoCard && !info.canSkip) return;

    switch (info.requiredCardType) {
    case CardType::Dodge:
        if (info.canSkip) {
            GameRule::handleAoeSkipResponse(m_state, responder);
            emitLog(responder->displayName() + QStringLiteral(" 放弃了响应"));
        } else {
            GameRule::handleKillResponse(m_state, responder, nullptr);
            emitLog(responder->displayName() + QStringLiteral(" 没有【闪】，受到伤害"));
        }
        break;

    case CardType::Kill:
        if (isDuelResponse(info)) {
            GameRule::handleDuelResponse(m_state, responder, nullptr);
            emitLog(responder->displayName() + QStringLiteral(" 未在决斗中打出【杀】，受到伤害"));
        } else {
            GameRule::handleAoeSkipResponse(m_state, responder);
            emitLog(responder->displayName() + QStringLiteral(" 放弃了响应"));
        }
        break;

    case CardType::Peach: {
        Player* dyingPlayer = info.target;
        GameRule::skipDyingResponse(m_state, dyingPlayer);
        emitLog(responder->displayName() + QStringLiteral(" 放弃救助 ") +
                dyingPlayer->displayName());
        break;
    }

    case CardType::Nullify: {
        GameRule::handleNullifyResponse(m_state, responder, nullptr, false);
        emitLog(responder->displayName() + QStringLiteral(" 放弃使用【无懈可击】，锦囊生效"));
        break;
    }

    default:
        break;
    }

    emit actionCompleted();
}

// ==================== 弃牌阶段 ====================

void ActionViewModel::discardCard(int cardId, int playerId)
{
    Card* card = findCard(cardId);
    Player* player = findPlayer(playerId);
    if (!m_state || !card || !player ||
        m_state->currentPhase() != PhaseType::Discard ||
        player != m_state->currentPlayer() ||
        !isOwnCard(cardId, playerId)) return;

    player->removeHandCard(card);
    if (m_state->cardManager()) m_state->cardManager()->discard(card);
    emitLog(player->displayName() + QStringLiteral(" 弃置了【") +
            QString::fromStdString(card->cardName()) + QStringLiteral("】"));
}

int ActionViewModel::getDiscardCount(int playerId) const
{
    Player* player = findPlayer(playerId);
    return GameRule::getDiscardCount(player);
}

// ==================== 主动技能 ====================

bool ActionViewModel::canUseActiveSkill(int playerId) const
{
    Player* player = findPlayer(playerId);
    if (!m_state || !player || !player->isAlive() || !player->character()) return false;
    if (m_state->currentPhase() != PhaseType::Play ||
        m_state->currentPlayer() != player || m_state->hasPendingAction()) {
        return false;
    }
    if (m_pendingCardId >= 0 || player->hasUsedActiveSkillThisTurn()) return false;

    // 孙权制衡：手中有牌即可发动
    if (player->character()->canDiscardAndDraw() && player->hasHandCards())
        return true;

    // 关羽武圣：有红色牌可转化为杀
    if (player->character()->hasActiveSkill()) {
        for (Card* card : player->handCards()) {
            if (!card) continue;
            auto t = player->character()->skillTransformCard(card);
            if (t == CardType::Kill && card->isRed() && GameRule::canPlayKill(m_state, player))
                return true;
        }
    }

    return false;
}

bool ActionViewModel::useActiveSkill(int playerId, const std::vector<int>& cardIds)
{
    Player* player = findPlayer(playerId);
    if (!canUseActiveSkill(playerId) || cardIds.empty() || !player) return false;

    std::unordered_set<int> uniqueIds;
    std::vector<Card*> cards;
    cards.reserve(cardIds.size());
    for (int cardId : cardIds) {
        if (!uniqueIds.insert(cardId).second) return false;
        Card* card = findCard(cardId);
        if (!card || !player->hasCard(card)) return false;
        cards.push_back(card);
    }

    player->setUsedActiveSkillThisTurn(true);

    // 关羽武圣：将红色牌当【杀】使用
    if (cards.size() == 1 && player->handCards().size() == 1 &&
        player->character()->skillTransformCard(cards.front()) == CardType::Kill) {
        Card* card = cards.front();
        QString skillName = QString::fromStdString(player->character()->skillName());
        emitLog(player->displayName() + QStringLiteral(" 发动【") + skillName +
                QStringLiteral("】，将【") + QString::fromStdString(card->cardName()) +
                QStringLiteral("】当【杀】使用"));

        // 用 playsAsKill 路径执行（选择目标后走杀的结算流程）
        player->removeHandCard(card);
        std::vector<Player*> targets;
        for (Player* p : m_state->alivePlayers()) {
            if (p && p != player && p->isAlive())
                targets.push_back(p);
        }
        if (targets.empty()) {
            if (m_state->cardManager()) m_state->cardManager()->discard(card);
            emit actionCompleted();
            return true;
        }
        // 选择第一个活着的玩家作为目标并执行杀
        GameRule::executeKill(m_state, player, targets.front());
        if (m_state->cardManager()) m_state->cardManager()->discard(card);
        emit actionCompleted();
        return true;
    }

    // 孙权制衡：弃牌摸牌
    for (Card* card : cards) player->removeHandCard(card);
    if (m_state->cardManager()) m_state->cardManager()->discardMultiple(cards);

    std::vector<Card*> drawn;
    if (m_state->cardManager()) {
        drawn = m_state->cardManager()->drawCards(static_cast<int>(cards.size()));
        for (Card* card : drawn) {
            if (card) player->addHandCard(card);
        }
    }

    emitLog(player->displayName() + QStringLiteral(" 发动【制衡】，弃置 %1 张牌并摸 %2 张牌")
            .arg(static_cast<int>(cards.size()))
            .arg(static_cast<int>(drawn.size())));
    emit actionCompleted();
    return true;
}

// ==================== 内部 ====================

std::vector<Card*> ActionViewModel::getPlayableCards(Player* player) const
{
    std::vector<Card*> result;
    if (!m_state || !player) return result;
    for (Card* card : player->handCards()) {
        if (!card) continue;
        if (canPlayCard(card->id(), player->playerId())) result.push_back(card);
    }
    return result;
}

/// 出牌阶段主动技能转化判定：卡牌本体不可用，但武将技能（武圣/龙胆）
/// 可将其当【杀】使用。本体可用时优先按本体使用（如关羽缺血时的红桃仍按桃使用）。
bool ActionViewModel::playsAsKill(const Card* card, const Player* player) const
{
    if (!m_state || !card || !player) return false;
    if (m_state->currentPhase() != PhaseType::Play) return false;
    if (!player->isAlive() || !player->character()) return false;
    if (card->cardType() == CardType::Kill) return false;
    if (card->canUse(m_state, player)) return false;
    if (player->character()->skillTransformCard(card) != CardType::Kill) return false;
    return GameRule::canPlayKill(m_state, player);
}

void ActionViewModel::emitLog(const QString& msg)
{
    emit logMessage(msg);
}

// ==================== View 命令槽（从 SGSApp 迁入） ====================

void ActionViewModel::onPlayCardRequested(int cardId, int playerId)
{
    if (m_pendingCardId >= 0) return;
    if (!isOwnCard(cardId, playerId)) return;
    if (!canPlayCard(cardId, playerId)) return;

    auto targets = getValidTargetIds(cardId, playerId);
    if (targets.empty()) {
        playCard(cardId, playerId, {});
    } else if (targets.size() == 1) {
        playCard(cardId, playerId, {targets[0]});
    } else {
        m_pendingCardId = cardId;
        m_pendingUserId = playerId;
        m_pendingTargetIds = QVector<int>(targets.begin(), targets.end());
        emit targetSelectionStarted(m_pendingTargetIds);
    }
}

void ActionViewModel::onTargetSelected(int playerIndex)
{
    if (m_pendingCardId >= 0 && !m_pendingTargetIds.isEmpty()) {
        bool validTarget = false;
        for (int t : m_pendingTargetIds) {
            if (t == playerIndex) {
                validTarget = true;
                break;
            }
        }

        if (!validTarget) return;

        const int cardId = m_pendingCardId;
        const int userId = m_pendingUserId;
        m_pendingCardId = -1;
        m_pendingUserId = -1;
        m_pendingTargetIds.clear();
        emit targetSelectionFinished();
        playCard(cardId, userId, {playerIndex});
    }
}

void ActionViewModel::onRespondCardRequested(int cardId, int responderId)
{
    respondCard(cardId, responderId);
}

void ActionViewModel::onSkillRequested(const QVector<int>& cardIds, int playerId)
{
    useActiveSkill(playerId, std::vector<int>(cardIds.begin(), cardIds.end()));
}
