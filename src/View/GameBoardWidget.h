#ifndef GAMEBOARDWIDGET_H
#define GAMEBOARDWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include "CommonTypes.h"
#include "CardDisplayData.h"
#include "PlayerDisplayData.h"

class PlayerInfoWidget;
class HandCardAreaWidget;
class ActionPanelWidget;
struct PendingActionVM;

/// 游戏桌面 — 零 ViewModel 依赖，全信号驱动
class GameBoardWidget : public QWidget {
    Q_OBJECT
public:
    explicit GameBoardWidget(QWidget* parent = nullptr);
    ~GameBoardWidget() override;

signals:
    // View → GameBootstrap（命令）
    void playCardRequested(int cardId, int playerId);
    void respondCardRequested(int cardId, int responderId);
    void discardCardRequested(int cardId, int playerId);
    void targetPlayerSelected(int playerIndex);
    void endPlayRequested();       // 用户主动结束出牌
    void advanceRequested();       // 自动阶段推进（由 AutoAdvanceTimer 触发）
    void skipRequested();          // 跳过响应
    void confirmDiscardRequested();
    void gameFinished();

public slots:
    // GameBootstrap/ViewModel → View（状态推送）
    void onPhaseChanged(PhaseType phase);
    void onPlayerDataUpdated(int playerId, const PlayerDisplayData& data);
    void onHandCardsUpdated(int playerId, const CardDisplayList& data);
    void onPendingActionCreated(const PendingActionVM& info);
    void onPendingActionCleared();
    void onGameOver(int winnerId);
    void onLogMessage(const QString& msg);
    void refreshDisplay();

private slots:
    void onCardClicked(int cardId);
    void onCardDoubleClicked(int cardId);
    void onTargetClicked(int playerIndex);
    void onPlayPhaseEnded();
    void onResponseSkipped();
    void onDiscardConfirmed();

private:
    void setupLayout();

    enum class State { Idle, SelectingTarget, Responding, Discarding };
    State m_state = State::Idle;

    PhaseType m_currentPhase = PhaseType::Prepare;
    int m_currentPlayerId = 0;
    int m_responderId = -1;       // 当前需要响应的玩家 ID
    int m_pendingCardId = -1;
    int m_pendingUserId = -1;
    int m_requiredDiscardCount = 0;

    // 子控件
    PlayerInfoWidget*   m_topPlayerInfo;
    PlayerInfoWidget*   m_bottomPlayerInfo;
    HandCardAreaWidget* m_topHandArea;
    HandCardAreaWidget* m_bottomHandArea;
    ActionPanelWidget*  m_actionPanel;
    QLabel*             m_logLabel;
    QTimer*             m_autoAdvanceTimer;
};

#endif // GAMEBOARDWIDGET_H
