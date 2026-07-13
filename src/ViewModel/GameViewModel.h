#ifndef GAMEVIEWMODEL_H
#define GAMEVIEWMODEL_H

#include <QObject>
#include <QString>
#include <vector>
#include <memory>
#include "CommonTypes.h"
#include "GameState.h"

class CardManager;
class CardManager;
class Player;
class Card;
class Character;
class PlayerViewModel;
class ActionViewModel;
class CardViewModel;

/// ViewModel 层的待定动作信息（值类型，无 Model 指针）
struct PendingActionVM {
    int sourceId = -1;
    int targetId = -1;
    int sourceCardId = -1;
    CardType requiredCardType = CardType::Kill;
    QString description;
    bool canSkip = false;
    std::vector<int> remainingTargetIds;
};

/// 游戏 ViewModel — 中央协调器，全部使用 Qt 信号/槽
class GameViewModel : public QObject {
    Q_OBJECT
public:
    explicit GameViewModel(QObject* parent = nullptr);
    ~GameViewModel() override;

    // ==================== 游戏生命周期 ====================

    void startGame(int characterId1, int characterId2);
    void advancePhase();
    void endPlayPhase();

    // ==================== 状态查询 ====================

    int currentPlayerId() const;
    int opponentPlayerId() const;
    PhaseType currentPhase() const;
    int turnCount() const;
    bool isGameOver() const;
    int winnerId() const;
    static QString phaseName(PhaseType phase);

    // ==================== 待定动作 ====================

    bool hasPendingAction() const;
    PendingActionVM pendingActionVM() const;

    // ==================== 子 ViewModel ====================

    PlayerViewModel* playerVM(int index) const;
    ActionViewModel* actionVM() const;
    GameState* gameState() const;

    // ==================== 手牌展示辅助 ====================

    std::vector<std::unique_ptr<CardViewModel>> getCurrentPlayerCardVMs() const;
    std::vector<std::unique_ptr<CardViewModel>> getPlayerCardVMs(int playerIndex) const;

    // ==================== 显示数据查找 ====================

    QString cardNameById(int cardId) const;
    CardType cardTypeById(int cardId) const;
    QString playerDisplayName(int playerId) const;

signals:
    void phaseChanged(PhaseType phase);
    void currentPlayerChanged(int playerIndex);
    void gameOver(int winnerId);
    void logMessage(const QString& msg);
    void pendingActionCreated(const PendingActionVM& info);
    void pendingActionCleared();
    void stateChanged();

private:
    // ---- 初始化 ----
    void initGame(Character* char1, Character* char2);
    Character* createCharacterById(int id);

    // ---- 阶段执行 ----
    void executePhasePrepare();
    void executePhaseJudge();
    void executePhaseDraw();
    void executePhaseEnd();

    // ---- 辅助 ----
    void setNextPhase(PhaseType phase);
    void switchToNextPlayer();
    void emitLog(const QString& msg);

    // ---- Model 事件回调 ----
    void onModelPendingActionCreated(const PendingActionInfo& info);
    void onModelPendingActionCleared();
    void onModelGameOver(int winnerId);
    void onModelPlayerDied(int playerId);

    // ---- 内部 ----
    Player* currentPlayer() const;
    Player* opponentPlayer() const;
    Player* playerByIndex(int index) const;
    Card* findCard(int cardId) const;

    // ---- 成员 ----
    std::unique_ptr<GameState> m_state;
    std::unique_ptr<CardManager> m_cardManager;
    std::unique_ptr<ActionViewModel> m_actionVM;
    std::vector<std::unique_ptr<PlayerViewModel>> m_playerVMs;
    std::vector<QMetaObject::Connection> m_modelConnections;
};

#endif // GAMEVIEWMODEL_H
