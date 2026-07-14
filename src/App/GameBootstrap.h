#ifndef GAMEBOOTSTRAP_H
#define GAMEBOOTSTRAP_H

#include <QObject>
#include <QVector>
#include <QMainWindow>
#include <memory>
#include "PendingActionVM.h"

class GameViewModel;
class GameBoardWidget;

/// 组合根 — 应用程序入口，创建并拥有所有顶层对象
class GameBootstrap : public QObject {
    Q_OBJECT
public:
    explicit GameBootstrap(QObject* parent = nullptr);

    /// 主窗口（main.cpp 只需 show()，不需要知道 MainWindow 的具体类型）
    QMainWindow* mainWindow() const { return m_mainWindow; }

    /// 开始一局新游戏
    void startLocalGame(int charId1, int charId2);

private slots:
    void onGameFinished();
    void onPendingActionFromVM(const PendingActionVM& info);
    void onPhaseFromVM(PhaseType phase);
    void onPlayCardRequested(int cardId, int playerId);
    void onRespondCardRequested(int cardId, int responderId);
    void onDiscardCardRequested(int cardId, int playerId);
    void onTargetSelected(int playerIndex);
    void onEndPlayRequested();
    void onAdvanceRequested();
    void onSkipRequested();

private:
    void wireAll();

    QMainWindow*      m_mainWindow = nullptr;
    GameViewModel*   m_gvm = nullptr;
    GameBoardWidget* m_board = nullptr;

    // 目标选择暂存
    int m_pendingCardId = -1;
    int m_pendingUserId = -1;
    QVector<int> m_pendingTargetIds;
};

#endif // GAMEBOOTSTRAP_H
