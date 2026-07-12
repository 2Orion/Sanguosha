#include "ActionViewModel.h"
#include "Core/CommonTypes.h"
#include "GameState.h"
#include "GameRule.h"
#include "CardManager.h"
#include "Player.h"
#include "Card.h"
#include "Character.h"

ActionViewModel::ActionViewModel()
{
}

void ActionViewModel::setGameState(GameState* state)
{
    m_state = state;
}

// ==================== 内部 Model 对象查找 ====================

Player* ActionViewModel::findPlayer(int playerId) const
{
    if (!m_state) return nullptr;
    return m_state->player(playerId);
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
    return card->canUse(m_state, player) &&
           GameRule::canPlayCard(m_state, player, card);
}

ActionResult ActionViewModel::playCard(int cardId, int playerId,
                                        const std::vector<int>& targetIds)
{
    Card* card = findCard(cardId);
    Player* user = findPlayer(playerId);
    if (!m_state || !card || !user) return ActionResult::Completed;

    // 将 target IDs 转为 Player* 列表
    std::vector<Player*> targets;
    for (int tid : targetIds) {
        Player* t = findPlayer(tid);
        if (t) targets.push_back(t);
    }

    ActionResult result = card->execute(m_state, user, targets);

    // 使用后从手牌移除并放入弃牌堆
    user->removeHandCard(card);
    if (m_state->cardManager()) {
        m_state->cardManager()->discard(card);
    }

    // 记录日志
    std::string targetStr;
    for (size_t i = 0; i < targets.size(); ++i) {
        if (i > 0) targetStr += "、";
        targetStr += targets[i]->displayName();
    }

    emitLog(user->displayName() + " 使用了【" + card->cardName() + "】" +
            (targetStr.empty() ? "" : " → " + targetStr));

    actionCompleted.notify();
    return result;
}

// ==================== 响应阶段 ====================

std::vector<int> ActionViewModel::getResponseCardIds(int playerId,
                                                       CardType requiredType) const
{
    std::vector<int> result;
    Player* player = findPlayer(playerId);
    if (!player) return result;

    for (Card* card : player->handCards()) {
        if (!card) continue;

        // 直接匹配
        if (card->cardType() == requiredType) {
            result.push_back(card->id());
            continue;
        }

        // 武将技能转化
        if (player->character()) {
            CardType transformed = player->character()->skillTransformCard(card);
            if (transformed == requiredType) {
                result.push_back(card->id());
            }
        }
    }
    return result;
}

bool ActionViewModel::isValidResponseCard(int cardId, int playerId,
                                           CardType requiredType) const
{
    std::vector<int> responseCardIds = getResponseCardIds(playerId, requiredType);
    for (int id : responseCardIds) {
        if (id == cardId) return true;
    }
    return false;
}

bool ActionViewModel::canSkipPendingAction() const
{
    if (!m_state) return false;
    if (!m_state->hasPendingAction()) return false;
    return m_state->pendingActionInfo().canSkip;
}

void ActionViewModel::respondCard(int cardId, int playerId)
{
    Card* card = findCard(cardId);
    Player* responder = findPlayer(playerId);
    if (!m_state || !card || !responder) return;
    if (!m_state->hasPendingAction()) return;

    const PendingActionInfo& info = m_state->pendingActionInfo();
    std::string cardName = card->cardName();

    switch (info.requiredCardType) {
    case CardType::Dodge:
        if (info.canSkip) {
            // AOE 闪避响应（万箭齐发）
            GameRule::handleAoeDodgeResponse(m_state, responder, card);
            emitLog(responder->displayName() + " 打出【" + cardName + "】响应万箭齐发");
        } else {
            // 普通杀响应
            GameRule::handleKillResponse(m_state, responder, card);
            emitLog(responder->displayName() + " 打出【" + cardName + "】响应杀");
        }
        break;

    case CardType::Kill:
        // 南蛮入侵响应
        GameRule::handleAoeKillResponse(m_state, responder, card);
        emitLog(responder->displayName() + " 打出【" + cardName + "】响应南蛮入侵");
        break;

    case CardType::Peach:
        // 濒死救助
        {
            Player* dyingPlayer = info.target;
            bool saved = GameRule::handleDyingPeach(m_state, dyingPlayer, responder, card);
            emitLog(responder->displayName() + " 对 " + dyingPlayer->displayName() +
                    " 使用【" + cardName + "】" +
                    (saved ? "，救回！" : "，仍需继续救援"));
        }
        break;

    default:
        break;
    }

    actionCompleted.notify();
}

void ActionViewModel::skipResponse(int playerId, bool forceNoCard)
{
    Player* responder = findPlayer(playerId);
    if (!m_state || !responder) return;
    if (!m_state->hasPendingAction()) return;

    const PendingActionInfo& info = m_state->pendingActionInfo();

    // forceNoCard=true: 玩家没有可用的响应牌，强制跳过（即使 canSkip=false）
    if (!forceNoCard && !info.canSkip) return;

    switch (info.requiredCardType) {
    case CardType::Dodge:
        if (info.canSkip) {
            // AOE 闪避跳过（万箭齐发）
            GameRule::handleAoeSkipResponse(m_state, responder);
            emitLog(responder->displayName() + " 放弃了响应");
        } else {
            // 普通杀 — 无闪可出，直接扣血
            GameRule::handleKillResponse(m_state, responder, nullptr);
            emitLog(responder->displayName() + " 没有【闪】，受到伤害");
        }
        break;

    case CardType::Kill:
        // AOE 杀跳过（南蛮入侵）
        GameRule::handleAoeSkipResponse(m_state, responder);
        emitLog(responder->displayName() + " 放弃了响应");
        break;

    case CardType::Peach:
        // 放弃救助
        GameRule::skipDyingResponse(m_state, info.target);
        emitLog(responder->displayName() + " 放弃救助 " + info.target->displayName());
        break;

    default:
        break;
    }

    actionCompleted.notify();
}

// ==================== 弃牌阶段 ====================

void ActionViewModel::discardCard(int cardId, int playerId)
{
    Card* card = findCard(cardId);
    Player* player = findPlayer(playerId);
    if (!m_state || !card || !player) return;

    player->removeHandCard(card);
    if (m_state->cardManager()) {
        m_state->cardManager()->discard(card);
    }

    emitLog(player->displayName() + " 弃置了【" + card->cardName() + "】");
}

int ActionViewModel::getDiscardCount(int playerId) const
{
    Player* player = findPlayer(playerId);
    return GameRule::getDiscardCount(player);
}

// ==================== 内部：Model 指针版本（供 ViewModel 层使用） ====================

std::vector<Card*> ActionViewModel::getPlayableCards(Player* player) const
{
    std::vector<Card*> result;
    if (!m_state || !player) return result;

    for (Card* card : player->handCards()) {
        if (!card) continue;

        // 检查卡牌自身的使用条件
        if (!card->canUse(m_state, player)) continue;

        // 通用规则检查
        if (!GameRule::canPlayCard(m_state, player, card)) continue;

        result.push_back(card);
    }
    return result;
}

// ==================== 内部 ====================

void ActionViewModel::emitLog(const std::string& msg)
{
    logMessage.notify(msg);
}
