#include "ClientApp.h"
#include "GameClient.h"
#include "GameBoardWidget.h"

ClientApp::ClientApp(QObject* parent)
    : QObject(parent)
{
    m_client = new GameClient(this);
    m_board = new GameBoardWidget();

    // ==================== View → GameClient（直连，形状对齐 SGSApp::startLocalGame） ====================
    connect(m_board, &GameBoardWidget::playCardRequested,
            m_client, &GameClient::playCard);
    connect(m_board, &GameBoardWidget::respondCardRequested,
            m_client, &GameClient::respondCard);
    connect(m_board, &GameBoardWidget::targetPlayerSelected,
            m_client, &GameClient::selectTarget);
    connect(m_board, &GameBoardWidget::discardCardRequested,
            m_client, &GameClient::discardCard);
    connect(m_board, &GameBoardWidget::endPlayRequested,
            m_client, &GameClient::endPlayPhase);
    connect(m_board, &GameBoardWidget::advanceRequested,
            m_client, &GameClient::advancePhase);
    connect(m_board, &GameBoardWidget::skipRequested,
            m_client, &GameClient::skipResponse);
    connect(m_board, &GameBoardWidget::skillRequested,
            m_client, &GameClient::useSkill);

    // ==================== GameClient → View（直连） ====================
    connect(m_client, &GameClient::phaseChanged,
            m_board, &GameBoardWidget::onPhaseChanged);
    connect(m_client, &GameClient::playerDataUpdated,
            m_board, &GameBoardWidget::onPlayerDataUpdated);
    connect(m_client, &GameClient::handCardsUpdated,
            m_board, &GameBoardWidget::onHandCardsUpdated);
    connect(m_client, &GameClient::targetSelectionStarted,
            m_board, &GameBoardWidget::onTargetSelectionStarted);
    connect(m_client, &GameClient::targetSelectionFinished,
            m_board, &GameBoardWidget::onTargetSelectionFinished);
    connect(m_client, &GameClient::pendingActionCreated,
            m_board, &GameBoardWidget::onPendingActionCreated);
    connect(m_client, &GameClient::pendingActionCleared,
            m_board, &GameBoardWidget::onPendingActionCleared);
    connect(m_client, &GameClient::gameOver,
            m_board, &GameBoardWidget::onGameOver);
    connect(m_client, &GameClient::logMessage,
            m_board, &GameBoardWidget::onLogMessage);
    connect(m_client, &GameClient::judgmentPerformed,
            m_board, &GameBoardWidget::onJudgmentPerformed);
    connect(m_client, &GameClient::connected, this, [this](int playerId) {
        m_board->setLocalPlayerId(playerId);
    });
}

void ClientApp::connectToServer(const QString& host, quint16 port)
{
    m_client->connectToServer(host, port);
}

void ClientApp::selectCharacter(int characterId)
{
    m_client->selectCharacter(characterId);
}
