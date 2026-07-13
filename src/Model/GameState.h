#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <vector>
#include <string>
#include <QObject>
#include "CommonTypes.h"

class Player;
class CardManager;
class Card;

/// 待定动作信息（当需要玩家响应时填充此结构）
struct PendingActionInfo {
    Player* source = nullptr;
    Player* target = nullptr;
    Card*   sourceCard = nullptr;
    CardType requiredCardType = CardType::Kill;
    std::string description;
    bool    canSkip = false;
    std::vector<Player*> remainingTargets;
};

class GameState : public QObject {
    Q_OBJECT
public:
    explicit GameState(QObject* parent = nullptr);
    ~GameState() override = default;

    // ---- 阶段管理 ----
    PhaseType currentPhase() const;
    void setCurrentPhase(PhaseType phase);

    // ---- 玩家管理 ----
    int currentPlayerIndex() const;
    void setCurrentPlayerIndex(int index);
    Player* currentPlayer() const;
    Player* player(int index) const;
    int playerCount() const;
    void addPlayer(Player* player);
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

signals:
    void phaseChanged(PhaseType phase);
    void currentPlayerChanged(int playerIndex);
    void pendingActionCreated(const PendingActionInfo& info);
    void pendingActionCleared();
    void gameOver(int winnerId);
    void stateRefreshed();

private:
    PhaseType m_currentPhase = PhaseType::Prepare;
    int m_currentPlayerIndex = 0;
    int m_turnCount = 0;
    std::vector<Player*> m_players;
    CardManager* m_cardManager = nullptr;
    bool m_hasPendingAction = false;
    PendingActionInfo m_pendingAction;
    bool m_gameOver = false;
    Player* m_winner = nullptr;
};

#endif // GAMESTATE_H
