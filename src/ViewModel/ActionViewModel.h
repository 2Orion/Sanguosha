#ifndef ACTIONVIEWMODEL_H
#define ACTIONVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QVector>
#include <vector>
#include "CommonTypes.h"

class GameState;
class Player;
class Card;

/// 操作 ViewModel — 管理出牌、响应、目标选择等逻辑
class ActionViewModel : public QObject {
    Q_OBJECT
public:
    explicit ActionViewModel(QObject* parent = nullptr);

    void setGameState(GameState* state);

    // ==================== 出牌阶段 ====================

    std::vector<int> getPlayableCardIds(int playerId) const;
    std::vector<int> getValidTargetIds(int cardId, int playerId) const;
    bool isOwnCard(int cardId, int playerId) const;
    bool canPlayCard(int cardId, int playerId) const;
    ActionResult playCard(int cardId, int playerId, const std::vector<int>& targetIds);

    // ==================== 响应阶段 ====================

    std::vector<int> getResponseCardIds(int playerId, CardType requiredType) const;
    bool isValidResponseCard(int cardId, int playerId, CardType requiredType) const;
    bool canSkipPendingAction() const;
    void respondCard(int cardId, int playerId);
    void skipResponse(int playerId, bool forceNoCard = false);

    // ==================== 弃牌阶段 ====================

    void discardCard(int cardId, int playerId);
    int getDiscardCount(int playerId) const;

    // ==================== View 命令槽（由 SGSApp 直连） ====================
public slots:
    void onPlayCardRequested(int cardId, int playerId);
    void onTargetSelected(int playerIndex);
    void onRespondCardRequested(int cardId, int responderId);

signals:
    void logMessage(const QString& msg);
    void actionCompleted();
    void targetSelectionStarted(const QVector<int>& targetIds);
    void targetSelectionFinished();

private:
    // === 内部 Model 对象查找 ===
    Player* findPlayer(int playerId) const;
    Card* findCard(int cardId) const;
    std::vector<Card*> getPlayableCards(Player* player) const;
    bool playsAsKill(const Card* card, const Player* player) const;
    void emitLog(const QString& msg);

    GameState* m_state = nullptr;

    // 多目标选择暂存（从 SGSApp 迁入）
    int m_pendingCardId = -1;
    int m_pendingUserId = -1;
    QVector<int> m_pendingTargetIds;
};

#endif // ACTIONVIEWMODEL_H
