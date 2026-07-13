#ifndef GAMEVIEWMODEL_H
#define GAMEVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QVector>
#include <memory>
#include "CommonTypes.h"
#include "GameState.h"
#include "CardDisplayData.h"
#include "PlayerDisplayData.h"
#include "PendingActionVM.h"

class CardManager;
class Player;
class Card;
class Character;
class ActionViewModel;

/// 游戏 ViewModel — 中央协调器，对外只暴露 Qt 信号 + 值类型
class GameViewModel : public QObject {
    Q_OBJECT
public:
    explicit GameViewModel(QObject* parent = nullptr);
    ~GameViewModel() override;

    // ==================== 游戏生命周期（GameBootstrap 调用） ====================
    void startGame(int characterId1, int characterId2);
    void advancePhase();
    void endPlayPhase();

    // ==================== 子 VM 访问（仅 GameBootstrap 使用） ====================
    ActionViewModel* actionVM() const;
    GameState* gameState() const;

    static QString phaseName(PhaseType phase);
    int currentPlayerId() const;

signals:
    void phaseChanged(PhaseType phase);
    void currentPlayerChanged(int playerIndex);
    void gameOver(int winnerId);
    void logMessage(const QString& msg);
    void pendingActionCreated(const PendingActionVM& info);
    void pendingActionCleared();
    void stateChanged();

    /// 手牌数据更新（双人模式实时推送双方手牌）
    void handCardsUpdated(int playerId, const CardDisplayList& cards);

    /// 玩家数据更新
    void playerDataUpdated(int playerId, const PlayerDisplayData& data);

private slots:
    void onModelPendingActionCreated(const PendingActionInfo& info);
    void onModelPendingActionCleared();
    void onModelGameOver(int winnerPlayerId);
    void onModelPlayerDied(int playerId);

private:
    void initGame(Character* char1, Character* char2);
    Character* createCharacterById(int id);
    void executePhasePrepare();
    void executePhaseJudge();
    void executePhaseDraw();
    void executePhaseEnd();
    void setNextPhase(PhaseType phase);
    void switchToNextPlayer();
    void emitLog(const QString& msg);

    // 数据推送辅助
    void pushPlayerData(int playerId);
    void pushHandCards(int playerId);
    void pushAllData();

    Player* currentPlayer() const;
    Player* opponentPlayer() const;
    Player* playerByIndex(int index) const;
    Card* findCard(int cardId) const;

    std::unique_ptr<GameState> m_state;
    std::unique_ptr<CardManager> m_cardManager;
    std::unique_ptr<ActionViewModel> m_actionVM;
    std::vector<QMetaObject::Connection> m_modelConnections;
};

#endif // GAMEVIEWMODEL_H
