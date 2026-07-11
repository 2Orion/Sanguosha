#include "ActionViewModel.h"
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

GameState* ActionViewModel::gameState() const
{
    return m_state;
}

// ==================== 出牌阶段 ====================

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

std::vector<Player*> ActionViewModel::getValidTargets(Card* card, Player* user) const
{
    if (!m_state || !card || !user) return {};
    return card->getValidTargets(m_state, user);
}

ActionResult ActionViewModel::playCard(Card* card, Player* user,
                                        const std::vector<Player*>& targets)
{
    if (!m_state || !card || !user) return ActionResult::Completed;

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

    actionCompleted.emit();
    return result;
}

// ==================== 响应阶段 ====================

std::vector<Card*> ActionViewModel::getResponseCards(Player* player,
                                                       CardType requiredType) const
{
    std::vector<Card*> result;
    if (!player) return result;

    for (Card* card : player->handCards()) {
        if (!card) continue;

        // 直接匹配
        if (card->cardType() == requiredType) {
            result.push_back(card);
            continue;
        }

        // 武将技能转化
        if (player->character()) {
            CardType transformed = player->character()->skillTransformCard(card);
            if (transformed == requiredType) {
                result.push_back(card);
            }
        }
    }
    return result;
}

bool ActionViewModel::canSkipPendingAction() const
{
    if (!m_state) return false;
    if (!m_state->hasPendingAction()) return false;
    return m_state->pendingActionInfo().canSkip;
}

void ActionViewModel::respondCard(Card* card, Player* responder)
{
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

    actionCompleted.emit();
}

void ActionViewModel::skipResponse(Player* responder)
{
    if (!m_state || !responder) return;
    if (!m_state->hasPendingAction()) return;

    const PendingActionInfo& info = m_state->pendingActionInfo();

    if (!info.canSkip) return;  // 不能跳过

    switch (info.requiredCardType) {
    case CardType::Dodge:
    case CardType::Kill:
        // AOE 跳过
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

    actionCompleted.emit();
}

// ==================== 弃牌阶段 ====================

void ActionViewModel::discardCard(Card* card, Player* player)
{
    if (!m_state || !card || !player) return;

    player->removeHandCard(card);
    if (m_state->cardManager()) {
        m_state->cardManager()->discard(card);
    }

    emitLog(player->displayName() + " 弃置了【" + card->cardName() + "】");
}

int ActionViewModel::getDiscardCount(Player* player) const
{
    return GameRule::getDiscardCount(player);
}

// ==================== 内部 ====================

void ActionViewModel::emitLog(const std::string& msg)
{
    logMessage.emit(msg);
}
