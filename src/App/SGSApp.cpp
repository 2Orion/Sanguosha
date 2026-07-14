#include "SGSApp.h"
#include "GameViewModel.h"
#include "ActionViewModel.h"
#include "GameBoardWidget.h"
#include "MainWindow.h"
#include "GameState.h"
#include "Player.h"

SGSApp::SGSApp(QObject* parent)
    : QObject(parent)
{
    // 1. 创建主窗口（内部用 MainWindow*，对外暴露 QMainWindow*）
    auto* mainWin = new MainWindow();
    mainWin->setAttribute(Qt::WA_DeleteOnClose, false);
    m_mainWindow = mainWin;

    // 2. 连接选将信号
    connect(mainWin, &MainWindow::startGameRequested,
            this, &SGSApp::startLocalGame);
}

void SGSApp::startLocalGame(int charId1, int charId2)
{
    // 创建游戏对象
    m_gvm = new GameViewModel(this);
    m_board = new GameBoardWidget();

    auto* avm = m_gvm->actionVM();

    // ==================== View → ViewModel（直连） ====================
    connect(m_board, &GameBoardWidget::playCardRequested,
            avm, &ActionViewModel::onPlayCardRequested);
    connect(m_board, &GameBoardWidget::respondCardRequested,
            avm, &ActionViewModel::onRespondCardRequested);
    connect(m_board, &GameBoardWidget::targetPlayerSelected,
            avm, &ActionViewModel::onTargetSelected);
    connect(m_board, &GameBoardWidget::discardCardRequested,
            m_gvm, &GameViewModel::onDiscardCardRequested);
    connect(m_board, &GameBoardWidget::endPlayRequested,
            m_gvm, &GameViewModel::onEndPlayRequested);
    connect(m_board, &GameBoardWidget::advanceRequested,
            m_gvm, &GameViewModel::onAdvanceRequested);
    connect(m_board, &GameBoardWidget::skipRequested,
            m_gvm, &GameViewModel::onSkipRequested);

    // ==================== ViewModel → View（直连） ====================
    connect(m_gvm, &GameViewModel::phaseChanged,
            m_board, &GameBoardWidget::onPhaseChanged);
    connect(m_gvm, &GameViewModel::playerDataUpdated,
            m_board, &GameBoardWidget::onPlayerDataUpdated);
    connect(m_gvm, &GameViewModel::handCardsUpdated,
            m_board, &GameBoardWidget::onHandCardsUpdated);
    connect(m_gvm, &GameViewModel::pendingActionCreated,
            m_board, &GameBoardWidget::onPendingActionCreated);
    connect(m_gvm, &GameViewModel::pendingActionCleared,
            m_board, &GameBoardWidget::onPendingActionCleared);
    connect(m_gvm, &GameViewModel::gameOver,
            m_board, &GameBoardWidget::onGameOver);
    connect(m_gvm, &GameViewModel::logMessage,
            m_board, &GameBoardWidget::onLogMessage);

    // ==================== 生命周期 ====================
    connect(m_board, &GameBoardWidget::gameFinished,
            this, &SGSApp::onGameFinished);

    m_gvm->startGame(charId1, charId2);

    // 切换到游戏页面（m_mainWindow 实际是 MainWindow*，隐式转 QMainWindow* 保存）
    static_cast<MainWindow*>(m_mainWindow)->showGamePage(m_board);
}

void SGSApp::onGameFinished()
{
    m_gvm = nullptr;
    if (m_board) {
        m_board->deleteLater();
        m_board = nullptr;
    }
    static_cast<MainWindow*>(m_mainWindow)->showSelectionPage();
}
