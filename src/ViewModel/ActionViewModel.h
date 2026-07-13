#ifndef ACTIONVIEWMODEL_H
#define ACTIONVIEWMODEL_H

#include <string>
#include <vector>
#include "Core/CommonTypes.h"
#include "Core/Event.h"

class GameState;
class Player;
class Card;

/// 操作 ViewModel — 管理出牌、响应、目标选择等逻辑
/// 公共接口全部使用 int ID，内部查找 Model 对象
class ActionViewModel {
public:
    ActionViewModel();

    void setGameState(GameState* state);

    // ==================== 出牌阶段（所有参数为 ID） ====================

    /// 获取当前玩家在出牌阶段可打出的手牌 ID 列表
    std::vector<int> getPlayableCardIds(int playerId) const;

    /// 获取某张牌对指定玩家的合法目标 ID 列表
    std::vector<int> getValidTargetIds(int cardId, int playerId) const;

    /// 检查某张牌是否属于指定玩家（手牌所有权检查）
    bool isOwnCard(int cardId, int playerId) const;

    /// 综合检查：牌是否可被玩家使用（canUse + GameRule::canPlayCard）
    bool canPlayCard(int cardId, int playerId) const;

    /// 出牌（由当前玩家主动使用），返回动作结果
    ActionResult playCard(int cardId, int playerId,
                          const std::vector<int>& targetIds);

    // ==================== 响应阶段 ====================

    /// 获取响应某类请求时可打出的牌 ID 列表（含武将技能转化）
    std::vector<int> getResponseCardIds(int playerId, CardType requiredType) const;

    /// 检查某张牌是否可以作为指定类型的响应牌打出
    bool isValidResponseCard(int cardId, int playerId, CardType requiredType) const;

    /// 是否可以跳过当前待定动作
    bool canSkipPendingAction() const;

    /// 响应出牌（如出闪响应杀、出杀响应南蛮、出桃救人）
    void respondCard(int cardId, int playerId);

    /// 跳过响应（forceNoCard=true 时强制跳过，用于玩家无可用响应牌的情况）
    void skipResponse(int playerId, bool forceNoCard = false);

    // ==================== 弃牌阶段 ====================

    void discardCard(int cardId, int playerId);

    /// 获取需要弃牌的数量（0 则不需要）
    int getDiscardCount(int playerId) const;

    // ==================== 事件 ====================

    EventListener<const std::string&> logMessage;
    EventListener<> actionCompleted;

private:
    // ---- 内部 Model 对象查找 ----
    Player* findPlayer(int playerId) const;
    Card* findCard(int cardId) const;

    // ---- 内部：使用 Model 指针的版本（供 ViewModel 层内部使用） ----
    std::vector<Card*> getPlayableCards(Player* player) const;

    void emitLog(const std::string& msg);

    GameState* m_state = nullptr;
};

#endif // ACTIONVIEWMODEL_H
