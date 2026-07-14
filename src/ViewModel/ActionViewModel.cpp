#include "ActionViewModel.h"
#include "GameState.h"
#include "GameRule.h"
#include "CardManager.h"
#include "Player.h"
#include "Card.h"
#include "Character.h"

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
    if (!m_state || !card || !user) return {};
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
    if (!m_state || !card || !player) return false;
    return card->canUse(m_state, player) && GameRule::canPlayCard(m_state, player, card);
}

ActionResult ActionViewModel::playCard(int cardId, int playerId,
                                        const std::vector<int>& targetIds)
{
    Card* card = findCard(cardId);
    Player* user = findPlayer(playerId);
    if (!m_state || !card || !user) return ActionResult::Completed;

    std::vector<Player*> targets;
    for (int tid : targetIds) {
        Player* t = findPlayer(tid);
        if (t) targets.push_back(t);
    }

    ActionResult result = card->execute(m_state, user, targets);
    user->removeHandCard(card);
    if (m_state->cardManager()) m_state->cardManager()->discard(card);

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
        if (card->cardType() == requiredType) {
            result.push_back(card->id());
            continue;
        }
        if (player->character()) {
            CardType transformed = player->character()->skillTransformCard(card);
            if (transformed == requiredType) result.push_back(card->id());
        }
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

    const PendingActionInfo& info = m_state->pendingActionInfo();

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
        GameRule::handleAoeKillResponse(m_state, responder, card);
        emitLog(responder->displayName() + QStringLiteral(" 打出【") +
                QString::fromStdString(card->cardName()) + QStringLiteral("】响应南蛮入侵"));
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

    default:
        break;
    }

    emit actionCompleted();
}

void ActionViewModel::skipResponse(int playerId, bool forceNoCard)
{
    Player* responder = findPlayer(playerId);
    if (!m_state || !responder || !m_state->hasPendingAction()) return;

    const PendingActionInfo& info = m_state->pendingActionInfo();
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
        GameRule::handleAoeSkipResponse(m_state, responder);
        emitLog(responder->displayName() + QStringLiteral(" 放弃了响应"));
        break;

    case CardType::Peach: {
        Player* dyingPlayer = info.target;
        GameRule::skipDyingResponse(m_state, dyingPlayer);
        emitLog(responder->displayName() + QStringLiteral(" 放弃救助 ") +
                dyingPlayer->displayName());
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
    if (!m_state || !card || !player) return;

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

// ==================== 内部 ====================

std::vector<Card*> ActionViewModel::getPlayableCards(Player* player) const
{
    std::vector<Card*> result;
    if (!m_state || !player) return result;
    for (Card* card : player->handCards()) {
        if (!card) continue;
        if (!card->canUse(m_state, player)) continue;
        if (!GameRule::canPlayCard(m_state, player, card)) continue;
        result.push_back(card);
    }
    return result;
}

void ActionViewModel::emitLog(const QString& msg)
{
    emit logMessage(msg);
}

// ==================== View 命令槽（从 SGSApp 迁入） ====================

void ActionViewModel::onPlayCardRequested(int cardId, int playerId)
{
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
        playCard(cardId, playerId, {targets[0]});
    }
}

void ActionViewModel::onTargetSelected(int playerIndex)
{
    if (m_pendingCardId >= 0 && !m_pendingTargetIds.isEmpty()) {
        for (int t : m_pendingTargetIds) {
            if (t == playerIndex) {
                playCard(m_pendingCardId, m_pendingUserId, {playerIndex});
                break;
            }
        }
        m_pendingCardId = -1;
        m_pendingUserId = -1;
        m_pendingTargetIds.clear();
    }
}

void ActionViewModel::onRespondCardRequested(int cardId, int responderId)
{
    respondCard(cardId, responderId);
}
