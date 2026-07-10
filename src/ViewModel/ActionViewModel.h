#ifndef ACTIONVIEWMODEL_H
#define ACTIONVIEWMODEL_H

#include <string>
#include <vector>
#include "CommonTypes.h"
#include "Event.h"

class GameState;
class Player;
class Card;

/// 操作 ViewModel — 管理出牌、响应、目标选择等逻辑
class ActionViewModel {
public:
    ActionViewModel();

    void setGameState(GameState* state);
    GameState* gameState() const;

    // ==================== 出牌阶段 ====================

    /// 获取当前玩家在出牌阶段可打出的手牌
    std::vector<Card*> getPlayableCards(Player* player) const;

    /// 获取某张牌对指定玩家的合法目标列表
    std::vector<Player*> getValidTargets(Card* card, Player* user) const;

    /// 出牌（由当前玩家主动使用）
    ActionResult playCard(Card* card, Player* user,
                          const std::vector<Player*>& targets);

    // ==================== 响应阶段 ====================

    /// 获取响应某类请求时可打出的牌（含武将技能转化）
    std::vector<Card*> getResponseCards(Player* player, CardType requiredType) const;

    /// 是否可以跳过当前待定动作
    bool canSkipPendingAction() const;

    /// 响应出牌（如出闪响应杀、出杀响应南蛮、出桃救人）
    void respondCard(Card* card, Player* responder);

    /// 跳过响应
    void skipResponse(Player* responder);

    // ==================== 弃牌阶段 ====================

    void discardCard(Card* card, Player* player);

    /// 获取需要弃牌的数量（0 则不需要）
    int getDiscardCount(Player* player) const;

    // ==================== 事件 ====================

    EventListener<const std::string&> logMessage;
    EventListener<> actionCompleted;

private:
    void emitLog(const std::string& msg);

    GameState* m_state = nullptr;
};

#endif // ACTIONVIEWMODEL_H
