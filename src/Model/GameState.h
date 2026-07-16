#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <vector>
#include <string>
#include <functional>
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

    // AOE 目标进入濒死时，救援完成后恢复原响应链。
    Player* continuationSource = nullptr;
    CardType continuationCardType = CardType::Kill;
    std::vector<Player*> continuationTargets;

    /// 无懈可击：当此待定动作被跳过时执行的回调
    /// 用于恢复被延迟的锦囊牌效果（如对方选择不使用无懈，则原锦囊继续结算）
    std::function<void()> onNullifySkipped;

    /// 是否为决斗的轮次（决斗双方交替出杀，不能走 AOE 的分支）
    bool isDuel = false;

    /// 闪响应计数（吕布无双需 2 张闪，默认 1）
    int requiredDodgeTotal = 1;
    int dodgeProgress = 0;
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
