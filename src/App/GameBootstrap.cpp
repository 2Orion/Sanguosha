#include "GameBootstrap.h"
#include "GameViewModel.h"
#include "ActionViewModel.h"
#include "GameBoardWidget.h"
#include "GameState.h"   // PendingActionInfo
#include "Player.h"

GameBootstrap::GameBootstrap(QWidget* boardParent, QObject* parent)
    : QObject(parent), m_boardParent(boardParent) {}

void GameBootstrap::startLocalGame(int charId1, int charId2)
{
    m_gvm = new GameViewModel(this);
    m_board = new GameBoardWidget(m_boardParent);

    wireAll();

    // 初始数据推送（GameViewModel 初始化后自行 emit handCardsUpdated / playerDataUpdated）
    m_gvm->startGame(charId1, charId2);
}

// ==================== 信号/槽连接（核心：View ↔ ViewModel 完全在此解耦） ====================

void GameBootstrap::wireAll()
{
    if (!m_gvm || !m_board) return;

    // ── ViewModel 信号 → View 槽 ──
    m_gvm->connect(m_gvm, &GameViewModel::phaseChanged,
                   m_board, &GameBoardWidget::onPhaseChanged);
    m_gvm->connect(m_gvm, &GameViewModel::playerDataUpdated,
                   m_board, &GameBoardWidget::onPlayerDataUpdated);
    m_gvm->connect(m_gvm, &GameViewModel::handCardsUpdated,
                   m_board, &GameBoardWidget::onHandCardsUpdated);
    m_gvm->connect(m_gvm, &GameViewModel::pendingActionCreated,
                   m_board, &GameBoardWidget::onPendingActionCreated);
    m_gvm->connect(m_gvm, &GameViewModel::pendingActionCleared,
                   m_board, &GameBoardWidget::onPendingActionCleared);
    m_gvm->connect(m_gvm, &GameViewModel::gameOver,
                   m_board, &GameBoardWidget::onGameOver);
    m_gvm->connect(m_gvm, &GameViewModel::logMessage,
                   m_board, &GameBoardWidget::onLogMessage);

    // ── View 信号 → GameBootstrap 槽（中转） ──
    m_board->connect(m_board, &GameBoardWidget::playCardRequested,
                     this, &GameBootstrap::onPlayCardRequested);
    m_board->connect(m_board, &GameBoardWidget::respondCardRequested,
                     this, &GameBootstrap::onRespondCardRequested);
    m_board->connect(m_board, &GameBoardWidget::discardCardRequested,
                     this, &GameBootstrap::onDiscardCardRequested);
    m_board->connect(m_board, &GameBoardWidget::targetPlayerSelected,
                     this, &GameBootstrap::onTargetSelected);
    m_board->connect(m_board, &GameBoardWidget::endPlayRequested,
                     this, &GameBootstrap::onEndPlayRequested);
    m_board->connect(m_board, &GameBoardWidget::advanceRequested,
                     this, &GameBootstrap::onAdvanceRequested);
    m_board->connect(m_board, &GameBoardWidget::skipRequested,
                     this, &GameBootstrap::onSkipRequested);
    m_board->connect(m_board, &GameBoardWidget::gameFinished,
                     this, &GameBootstrap::gameFinished);
}

// ==================== View 信号 → ViewModel 方法（中转逻辑） ====================

void GameBootstrap::onPlayCardRequested(int cardId, int playerId)
{
    ActionViewModel* avm = m_gvm->actionVM();
    GameState* state = m_gvm->gameState();

    // 权限检查
    if (!avm->isOwnCard(cardId, playerId)) return;
    if (!avm->canPlayCard(cardId, playerId)) return;

    // 获取合法目标
    QVector<int> targetsList;
    auto validTargets = avm->getValidTargetIds(cardId, playerId);
    for (int t : validTargets) targetsList.append(t);

    if (targetsList.isEmpty()) {
        // 无目标牌（桃/酒/无中生有）
        avm->playCard(cardId, playerId, {});
    } else if (targetsList.size() == 1) {
        // 单目标，自动选择
        avm->playCard(cardId, playerId, {targetsList[0]});
    } else {
        // 多目标 → 进入目标选择模式（通过 ViewModel 发出信号让 View 处理）
        m_pendingCardId = cardId;
        m_pendingUserId = playerId;
        m_pendingTargetIds = targetsList;
        // GameViewModel 可以发射 targetSelectionNeeded 信号
        // 目前简化：直接选第一个目标
        avm->playCard(cardId, playerId, {targetsList[0]});
    }
}

void GameBootstrap::onRespondCardRequested(int cardId, int responderId)
{
    m_gvm->actionVM()->respondCard(cardId, responderId);
}

void GameBootstrap::onDiscardCardRequested(int cardId, int playerId)
{
    m_gvm->actionVM()->discardCard(cardId, playerId);
    // 继续检查是否还需要弃牌
    if (m_gvm->actionVM()->getDiscardCount(playerId) <= 0) {
        m_gvm->advancePhase();
    }
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

void GameBootstrap::onEndPlayRequested()
{
    m_gvm->endPlayPhase();
}

void GameBootstrap::onAdvanceRequested()
{
    m_gvm->advancePhase();
}

void GameBootstrap::onSkipRequested()
{
    ActionViewModel* avm = m_gvm->actionVM();
    GameState* state = m_gvm->gameState();
    if (!state || !state->hasPendingAction()) return;

    int responderId = state->pendingActionInfo().target->playerId();

    // 检查是否有可用响应牌
    const auto& info = state->pendingActionInfo();
    auto responseCards = avm->getResponseCardIds(responderId, info.requiredCardType);

    if (responseCards.empty() && !info.canSkip) {
        // 无牌可出且不可跳过 → 强制跳过（承担后果）
        avm->skipResponse(responderId, true);
    } else {
        avm->skipResponse(responderId, false);
    }
}
