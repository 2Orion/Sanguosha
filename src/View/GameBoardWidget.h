#ifndef GAMEBOARDWIDGET_H
#define GAMEBOARDWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <vector>
#include <cstddef>
#include "Core/CommonTypes.h"

class GameViewModel;
class CardViewModel;
class PlayerInfoWidget;
class HandCardAreaWidget;
class ActionPanelWidget;
struct PendingActionVM;

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
    void onPendingActionCreated(const PendingActionVM& info);
    void onPendingActionCleared();

    // ---- 交互处理（全部使用 int ID） ----
    void onCardClicked(int cardId);
    void onCardDoubleClicked(int cardId);
    void onTargetClicked(int playerIndex);
    void onPlayPhaseEnded();
    void onResponseSkipped();
    void onDiscardConfirmed();
    void onGameOver(int winnerId);

    // ---- 辅助 ----
    void showLog(const QString& msg);
    void enterTargetSelection(int cardId, int userId,
                               const std::vector<int>& targetIds);
    void exitTargetSelection();
    void refreshHandCards();

    /// 通过 cardId 查找 CardViewModel（遍历两个手牌区）
    CardViewModel* findCardVM(int cardId) const;

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

    // ---- 交互状态（全部使用 int ID） ----
    enum class State { Idle, SelectingTarget, Responding, Discarding };
    State m_state = State::Idle;

    // 目标选择暂存
    int m_pendingCardId = -1;
    int m_pendingCardUserId = -1;
    std::vector<int> m_pendingTargetIds;

    // 弃牌追踪
    int m_requiredDiscardCount = 0;
};

#endif // GAMEBOARDWIDGET_H
