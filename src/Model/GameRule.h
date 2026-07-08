#ifndef GAMERULE_H
#define GAMERULE_H

class GameState;
class Player;
class Card;

namespace GameRule {

    // ==================== 常量 ====================

    constexpr int INITIAL_HAND_COUNT = 4;  // 初始手牌数
    constexpr int DRAW_PHASE_COUNT = 2;    // 摸牌阶段摸牌数

    // ==================== 规则判断 ====================

    /// 玩家当前是否可以使用【杀】（含张飞咆哮判定）
    bool canPlayKill(const GameState* state, const Player* player);

    /// 玩家是否可以使用某张牌（通用入口）
    bool canPlayCard(const GameState* state, const Player* player, const Card* card);

    /// 手牌上限（= 当前体力值）
    int handLimit(const Player* player);

    /// 是否有闪可响应
    bool hasDodgeToRespond(const Player* player);

    /// 是否有杀可响应（南蛮入侵）
    bool hasKillToRespond(const Player* player);

    /// 是否有桃可救濒死玩家
    bool hasPeachToSave(const Player* player, const Player* dyingPlayer);

    // ==================== 卡牌执行 ====================

    void executeKill(GameState* state, Player* user, Player* target);
    void handleKillResponse(GameState* state, Player* responder, Card* dodgeCard);
    void executePeach(GameState* state, Player* user, Player* target);
    void executeWine(GameState* state, Player* user);

    // ==================== 锦囊执行 ====================

    void executeDismantle(GameState* state, Player* user, Player* target);
    void executeSteal(GameState* state, Player* user, Player* target);
    void executeBountiful(GameState* state, Player* user);
    void executeBarbarianInvasion(GameState* state, Player* user);
    void executeVolley(GameState* state, Player* user);
    void executePeachGarden(GameState* state);

    // ==================== AOE 响应处理 ====================

    void handleAoeKillResponse(GameState* state, Player* responder, Card* killCard);
    void handleAoeDodgeResponse(GameState* state, Player* responder, Card* dodgeCard);
    void handleAoeSkipResponse(GameState* state, Player* responder);

    // ==================== 伤害与濒死 ====================

    void dealDamage(GameState* state, Player* target, int value, Player* source);
    void startDyingProcess(GameState* state, Player* dyingPlayer);
    bool handleDyingPeach(GameState* state, Player* dyingPlayer, Player* peachUser, Card* peachCard);
    void skipDyingResponse(GameState* state, Player* dyingPlayer);
    void checkDeath(GameState* state, Player* player);
    void checkGameOver(GameState* state);

    // ==================== 弃牌阶段 ====================

    /// 返回需要弃置的张数（0 则不需要）
    int getDiscardCount(const Player* player);

} // namespace GameRule

#endif // GAMERULE_H
