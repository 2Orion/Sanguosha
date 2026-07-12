#ifndef GAMEBOARDWIDGET_H
#define GAMEBOARDWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <vector>
#include <cstddef>
#include "CommonTypes.h"

class GameViewModel;
class Player;
class Card;
class PlayerInfoWidget;
class HandCardAreaWidget;
class ActionPanelWidget;
struct PendingActionInfo;

/// 游戏桌面总布局 — 组合所有子控件，连接 ViewModel 事件
class GameBoardWidget : public QWidget {
    Q_OBJECT
public:
    explicit GameBoardWidget(GameViewModel* gvm, QWidget* parent = nullptr);
    ~GameBoardWidget() override;

signals:
    void gameFinished();      // 返回主菜单

private slots:
    void autoAdvancePhase();  // 自动推进非交互阶段

private:
    // ---- 初始化 ----
    void setupLayout();
    void connectViewModel();
    void disconnectViewModel();
    void refreshDisplay();

    // ---- 阶段切换 ----
    void onPhaseChanged(PhaseType phase);
    void onPlayerChanged(int playerIndex);

    // ---- 待定动作（响应流程） ----
    void onPendingActionCreated(const PendingActionInfo& info);
    void onPendingActionCleared();

    // ---- 交互处理 ----
    void onCardClicked(Card* card);
    void onCardDoubleClicked(Card* card);
    void onTargetClicked(int playerIndex);
    void onPlayPhaseEnded();
    void onResponseSkipped();
    void onDiscardConfirmed();
    void onGameOver(Player* winner);

    // ---- 辅助 ----
    /// 跳过初始自动阶段（Prepare→Judge→Draw→Play），直接进入出牌阶段
    void processInitialAutoPhases();
    void showLog(const QString& msg);
    void enterTargetSelection(Card* card, Player* user,
                               const std::vector<Player*>& targets);
    void exitTargetSelection();
    void refreshHandCards();
    void setInteractionState(int state);

    // ---- 成员 ----
    GameViewModel* m_gvm;  // 不持有，由 MainWindow 持有
    std::vector<size_t> m_connectionIds;

    // 子控件
    PlayerInfoWidget*   m_topPlayerInfo;
    PlayerInfoWidget*   m_bottomPlayerInfo;
    HandCardAreaWidget* m_topHandArea;
    HandCardAreaWidget* m_bottomHandArea;
    ActionPanelWidget*  m_actionPanel;
    QLabel*             m_logLabel;
    QTimer*             m_autoAdvanceTimer;

    // ---- 交互状态 ----
    enum class State { Idle, SelectingTarget, Responding, Discarding };
    State m_state = State::Idle;

    // 目标选择暂存
    Card* m_pendingCard = nullptr;
    Player* m_pendingCardUser = nullptr;
    std::vector<Player*> m_pendingTargets;

    // 弃牌追踪
    int m_requiredDiscardCount = 0;

    bool m_initialBatch = true;  // 首次加载时跳过计时器延迟
};

#endif // GAMEBOARDWIDGET_H
