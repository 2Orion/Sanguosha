#ifndef GAMEVIEWMODEL_H
#define GAMEVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QTimer>
#include <memory>
#include "CommonTypes.h"
#include "GameState.h"
#include "CardData.h"
#include "PlayerData.h"
#include "PendingActionData.h"

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

    // ==================== 游戏生命周期（SGSApp 调用） ====================
    /// 返回 false 表示 characterId1/characterId2 非法（不在已知武将范围内），
    /// 未真正开局——调用方（ServerApp）据此回滚，不广播开局成功。
    bool startGame(int characterId1, int characterId2);
    void advancePhase();
    void endPlayPhase();

    // ==================== 子 VM 访问（仅 SGSApp 使用） ====================
    ActionViewModel* actionVM() const;
    GameState* gameState() const;

    static QString phaseName(PhaseType phase);
    /// characterId 是否在 createCharacterById 支持的范围内。供 ServerApp 在
    /// 广播 GameStarted 之前先校验，避免客户端先收到"游戏已开始"再发现
    /// 什么后续状态都不会来。与 createCharacterById 的 switch 需保持同步。
    static bool isValidCharacterId(int id);
    int currentPlayerId() const;

    /// 当前待响应动作应由哪位玩家响应；无待定动作时返回 -1。
    /// 供 ServerApp 校验 SkipRequested 等命令是否来自真正的响应者
    /// （网络模式下任意已连接客户端都可能发来这类无身份参数的命令）。
    int pendingResponderId() const;

signals:
    void phaseChanged(PhaseType phase);
    void currentPlayerChanged(int playerIndex);
    void gameOver(int winnerId);
    void logMessage(const QString& msg);
    void pendingActionCreated(const PendingActionData& info);
    void pendingActionCleared();
    void stateChanged();

    /// 判定结果展示（cardId-1 表示非判定牌，suit/number 为判定牌信息，effective 表示判定生效）
    void judgmentPerformed(const CardData& judgeCard, const QString& resultText, bool effective);

    /// 手牌数据更新（双人模式实时推送双方手牌）
    void handCardsUpdated(int playerId, const CardList& cards);

    /// 玩家数据更新
    void playerDataUpdated(int playerId, const PlayerData& data);

    // ==================== View 命令槽（由 SGSApp 直连） ====================
public slots:
    void onDiscardCardRequested(int cardId, int playerId);
    void onEndPlayRequested();
    void onAdvanceRequested();
    void onSkipRequested();

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

    // 判定辅助
    void processJudgment(Player* target, Card* judgeCard);
    Card* drawJudgeCard();
    void onJudgeTimerFired();

    Player* currentPlayer() const;
    Player* opponentPlayer() const;
    Player* playerByIndex(int index) const;
    Card* findCard(int cardId) const;

    std::unique_ptr<GameState> m_state;
    std::unique_ptr<CardManager> m_cardManager;
    std::unique_ptr<ActionViewModel> m_actionVM;
    std::vector<QMetaObject::Connection> m_modelConnections;

    // 判定阶段
    QTimer* m_judgeTimer = nullptr;
    int m_judgeTargetPlayerId = -1;
    bool m_judgePending = false;
    bool m_skipDrawPhase = false;
    bool m_skipPlayPhase = false;

    // 判定展示定时器在 pending 未清时以 100ms 重挂,此计数器兜底防止无限重挂
    int m_judgeTimerRetries = 0;
    static constexpr int JUDGE_TIMER_MAX_RETRIES = 10;
};

#endif // GAMEVIEWMODEL_H
