#include "GameBootstrap.h"
#include "GameViewModel.h"
#include "ActionViewModel.h"
#include "GameBoardWidget.h"
#include "MainWindow.h"
#include "GameState.h"
#include "Player.h"

GameBootstrap::GameBootstrap(QObject* parent)
    : QObject(parent)
{
    // 1. 创建主窗口（内部用 MainWindow*，对外暴露 QMainWindow*）
    auto* mainWin = new MainWindow();
    mainWin->setAttribute(Qt::WA_DeleteOnClose, false);
    m_mainWindow = mainWin;

    // 2. 连接选将信号
    connect(mainWin, &MainWindow::startGameRequested,
            this, &GameBootstrap::startLocalGame);
}

void GameBootstrap::startLocalGame(int charId1, int charId2)
{
    // 创建游戏对象
    m_gvm = new GameViewModel(this);
    m_board = new GameBoardWidget();

    wireAll();
    m_gvm->startGame(charId1, charId2);

    // 切换到游戏页面（m_mainWindow 实际是 MainWindow*，隐式转 QMainWindow* 保存）
    static_cast<MainWindow*>(m_mainWindow)->showGamePage(m_board);
    connect(m_board, &GameBoardWidget::gameFinished, this, &GameBootstrap::onGameFinished);
}

// ==================== 信号/槽连接 ====================

void GameBootstrap::wireAll()
{
    if (!m_gvm || !m_board) return;

    // ViewModel → GameBootstrap（中转）
    m_gvm->connect(m_gvm, &GameViewModel::phaseChanged, this, &GameBootstrap::onPhaseFromVM);
    m_gvm->connect(m_gvm, &GameViewModel::playerDataUpdated, m_board, &GameBoardWidget::onPlayerDataUpdated);
    m_gvm->connect(m_gvm, &GameViewModel::handCardsUpdated, m_board, &GameBoardWidget::onHandCardsUpdated);
    m_gvm->connect(m_gvm, &GameViewModel::pendingActionCreated, this, &GameBootstrap::onPendingActionFromVM);
    m_gvm->connect(m_gvm, &GameViewModel::pendingActionCleared, m_board, &GameBoardWidget::onPendingActionCleared);
    m_gvm->connect(m_gvm, &GameViewModel::gameOver, m_board, &GameBoardWidget::onGameOver);
    m_gvm->connect(m_gvm, &GameViewModel::logMessage, m_board, &GameBoardWidget::onLogMessage);

    // View → GameBootstrap（中转）
    m_board->connect(m_board, &GameBoardWidget::playCardRequested, this, &GameBootstrap::onPlayCardRequested);
    m_board->connect(m_board, &GameBoardWidget::respondCardRequested, this, &GameBootstrap::onRespondCardRequested);
    m_board->connect(m_board, &GameBoardWidget::discardCardRequested, this, &GameBootstrap::onDiscardCardRequested);
    m_board->connect(m_board, &GameBoardWidget::targetPlayerSelected, this, &GameBootstrap::onTargetSelected);
    m_board->connect(m_board, &GameBoardWidget::endPlayRequested, this, &GameBootstrap::onEndPlayRequested);
    m_board->connect(m_board, &GameBoardWidget::advanceRequested, this, &GameBootstrap::onAdvanceRequested);
    m_board->connect(m_board, &GameBoardWidget::skipRequested, this, &GameBootstrap::onSkipRequested);
}

// ==================== 游戏生命周期 ====================

void GameBootstrap::onGameFinished()
{
    m_gvm = nullptr;
    if (m_board) {
        m_board->deleteLater();
        m_board = nullptr;
    }
    static_cast<MainWindow*>(m_mainWindow)->showSelectionPage();
}

// ==================== ViewModel → View 拦截中转 ====================

void GameBootstrap::onPhaseFromVM(PhaseType phase)
{
    if (phase == PhaseType::Discard) {
        int curId = m_gvm->currentPlayerId();
        if (m_gvm->actionVM()->getDiscardCount(curId) <= 0) {
            m_gvm->advancePhase();
            return;
        }
    }
    m_board->onPhaseChanged(phase);
}

void GameBootstrap::onPendingActionFromVM(const PendingActionVM& info)
{
    auto responseCards = m_gvm->actionVM()->getResponseCardIds(info.targetId, info.requiredCardType);
    if (responseCards.empty()) {
        m_gvm->actionVM()->skipResponse(info.targetId, true);
        return;
    }
    m_board->onPendingActionCreated(info);
}

// ==================== View → ViewModel 路由 ====================

void GameBootstrap::onPlayCardRequested(int cardId, int playerId)
{
    auto* avm = m_gvm->actionVM();
    if (!avm->isOwnCard(cardId, playerId)) return;
    if (!avm->canPlayCard(cardId, playerId)) return;

    auto targets = avm->getValidTargetIds(cardId, playerId);
    if (targets.empty()) {
        avm->playCard(cardId, playerId, {});
    } else if (targets.size() == 1) {
        avm->playCard(cardId, playerId, {targets[0]});
    } else {
        m_pendingCardId = cardId;
        m_pendingUserId = playerId;
        m_pendingTargetIds = QVector<int>(targets.begin(), targets.end());
        avm->playCard(cardId, playerId, {targets[0]});
    }
}

void GameBootstrap::onRespondCardRequested(int cardId, int responderId)
{
    m_gvm->actionVM()->respondCard(cardId, responderId);
}

void GameBootstrap::onDiscardCardRequested(int cardId, int playerId)
{
    m_gvm->actionVM()->discardCard(cardId, playerId);
    if (m_gvm->actionVM()->getDiscardCount(playerId) <= 0)
        m_gvm->advancePhase();
}

void GameBootstrap::onTargetSelected(int playerIndex)
{
    if (m_pendingCardId >= 0 && !m_pendingTargetIds.isEmpty()) {
        for (int t : m_pendingTargetIds) {
            if (t == playerIndex) {
                m_gvm->actionVM()->playCard(m_pendingCardId, m_pendingUserId, {playerIndex});
                break;
            }
        }
        m_pendingCardId = -1;
        m_pendingTargetIds.clear();
    }
}

void GameBootstrap::onEndPlayRequested() { m_gvm->endPlayPhase(); }
void GameBootstrap::onAdvanceRequested() { m_gvm->advancePhase(); }

void GameBootstrap::onSkipRequested()
{
    auto* state = m_gvm->gameState();
    if (!state || !state->hasPendingAction()) return;
    auto* avm = m_gvm->actionVM();
    int responderId = state->pendingActionInfo().target->playerId();
    avm->skipResponse(responderId, true);
}
