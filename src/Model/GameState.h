#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <vector>
#include <string>
#include <memory>
#include "CommonTypes.h"
#include "Event.h"

class Player;
class CardManager;
class Card;

/// 待定动作信息（当需要玩家响应时填充此结构）
struct PendingActionInfo {
    Player* source = nullptr;           // 动作来源（使用牌的玩家）
    Player* target = nullptr;           // 需要响应的玩家
    Card*   sourceCard = nullptr;       // 触发动作的卡牌
    CardType requiredCardType;          // 需要打出的卡牌类型
    std::string description;            // 界面提示文字（如"请打出一张【闪】"）
    bool    canSkip = false;            // 是否可以跳过响应
    std::vector<Player*> remainingTargets; // AOE/链式响应中尚未询问的后续目标
};

class GameState {
public:
    GameState();
    ~GameState() = default;

    // ---- 阶段管理 ----
    PhaseType currentPhase() const;
    void setCurrentPhase(PhaseType phase);

    // ---- 玩家管理 ----
    int currentPlayerIndex() const;
    void setCurrentPlayerIndex(int index);
    Player* currentPlayer() const;
    Player* player(int index) const;
    int playerCount() const;
    void addPlayer(std::unique_ptr<Player> player);
    std::vector<Player*> alivePlayers() const;
    std::vector<Player*> allPlayers() const;

    // ---- 回合追踪 ----
    int turnCount() const;
    void incrementTurn();

    // ---- 待定动作 ----
    bool hasPendingAction() const;
    const PendingActionInfo& pendingActionInfo() const;
    void setPendingAction(const PendingActionInfo& info);
    void clearPendingAction();

    // ---- 牌堆管理器 ----
    CardManager* cardManager() const;
    void setCardManager(CardManager* mgr);

    // ---- 游戏结束 ----
    bool isGameOver() const;
    Player* winner() const;
    void setGameOver(Player* winnerPlayer);

    // ---- 事件通知 ----
    EventListener<PhaseType> phaseChanged;
    EventListener<int> currentPlayerChanged;
    EventListener<const PendingActionInfo&> pendingActionCreated;
    EventListener<> pendingActionCleared;
    EventListener<Player*> gameOver;
    EventListener<> stateRefreshed;

private:
    PhaseType m_currentPhase = PhaseType::Prepare;
    int m_currentPlayerIndex = 0;
    int m_turnCount = 0;
    std::vector<std::unique_ptr<Player>> m_players;
    CardManager* m_cardManager = nullptr;
    bool m_hasPendingAction = false;
    PendingActionInfo m_pendingAction;
    bool m_gameOver = false;
    Player* m_winner = nullptr;
};

#endif // GAMESTATE_H
