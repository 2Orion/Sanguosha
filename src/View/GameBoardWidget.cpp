#include "GameBoardWidget.h"
#include "PlayerInfoWidget.h"
#include "HandCardAreaWidget.h"
#include "ActionPanelWidget.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFont>

GameBoardWidget::GameBoardWidget(QWidget* parent) : QWidget(parent)
{
    m_autoAdvanceTimer = new QTimer(this);
    m_autoAdvanceTimer->setSingleShot(true);
    connect(m_autoAdvanceTimer, &QTimer::timeout, this, &GameBoardWidget::advanceRequested);

    setupLayout();

    // 子控件信号
    connect(m_bottomHandArea, &HandCardAreaWidget::cardClicked, this, &GameBoardWidget::onCardClicked);
    connect(m_bottomHandArea, &HandCardAreaWidget::cardDoubleClicked, this, &GameBoardWidget::onCardDoubleClicked);
    connect(m_topHandArea, &HandCardAreaWidget::cardClicked, this, &GameBoardWidget::onCardClicked);
    connect(m_topHandArea, &HandCardAreaWidget::cardDoubleClicked, this, &GameBoardWidget::onCardDoubleClicked);
    connect(m_topPlayerInfo, &PlayerInfoWidget::clicked, this, &GameBoardWidget::onTargetClicked);
    connect(m_bottomPlayerInfo, &PlayerInfoWidget::clicked, this, &GameBoardWidget::onTargetClicked);
    connect(m_actionPanel, &ActionPanelWidget::playPhaseEnded, this, &GameBoardWidget::onPlayPhaseEnded);
    connect(m_actionPanel, &ActionPanelWidget::respondSkipped, this, &GameBoardWidget::onResponseSkipped);
    connect(m_actionPanel, &ActionPanelWidget::discardConfirmed, this, &GameBoardWidget::onDiscardConfirmed);
}

GameBoardWidget::~GameBoardWidget() = default;

void GameBoardWidget::setupLayout()
{
    auto* main = new QVBoxLayout(this);
    main->setContentsMargins(8, 4, 8, 4); main->setSpacing(4);

    m_topPlayerInfo = new PlayerInfoWidget(this); main->addWidget(m_topPlayerInfo);
    m_topHandArea = new HandCardAreaWidget(this); m_topHandArea->setMinimumHeight(120); main->addWidget(m_topHandArea);

    m_logLabel = new QLabel(this);
    m_logLabel->setWordWrap(true); m_logLabel->setAlignment(Qt::AlignCenter); m_logLabel->setMinimumHeight(32);
    m_logLabel->setStyleSheet("QLabel { font-size: 12px; color: #444; padding: 6px 12px;"
        " background: #F5F5F5; border: 1px solid #E0E0E0; border-radius: 6px; }");
    main->addWidget(m_logLabel);

    m_bottomHandArea = new HandCardAreaWidget(this); m_bottomHandArea->setMinimumHeight(120); main->addWidget(m_bottomHandArea);
    m_actionPanel = new ActionPanelWidget(this); main->addWidget(m_actionPanel);
    m_bottomPlayerInfo = new PlayerInfoWidget(this); main->addWidget(m_bottomPlayerInfo);

    main->setStretchFactor(m_topHandArea, 1);
    main->setStretchFactor(m_bottomHandArea, 1);
}

// ==================== 状态槽 ====================

void GameBoardWidget::onPhaseChanged(PhaseType phase)
{
    m_currentPhase = phase;
    m_actionPanel->updateForPhase(phase, false);

    switch (phase) {
    case PhaseType::Prepare:
    case PhaseType::Judge:
    case PhaseType::Draw:
    case PhaseType::End:
        m_autoAdvanceTimer->start(300);
        break;
    case PhaseType::Play:
        m_state = State::Idle;
        break;
    case PhaseType::Discard:
        m_state = State::Discarding;
        m_actionPanel->setHint(QStringLiteral("请选择要弃置的手牌"));
        break;
    }
}

void GameBoardWidget::onPlayerDataUpdated(int playerId, const PlayerData& data)
{
    // 本地玩家信息在下方，对手在上方
    if (playerId == m_localPlayerId)
        m_bottomPlayerInfo->setDisplayData(data);
    else
        m_topPlayerInfo->setDisplayData(data);
    if (data.isCurrentPlayer) m_currentPlayerId = playerId;
    if (data.isDying) {
        auto* w = (playerId == m_localPlayerId) ? m_bottomPlayerInfo : m_topPlayerInfo;
        w->setStyleSheet(
            "PlayerInfoWidget { border: 2px solid #D32F2F; border-radius: 8px; background-color: #FFEBEE; }");
    }
}

void GameBoardWidget::onHandCardsUpdated(int playerId, const CardList& data)
{
    // 本地玩家的手牌在下方，对手手牌在上方
    if (playerId == m_localPlayerId)
        m_bottomHandArea->setCards(data, true);
    else
        m_topHandArea->setCards(data, true);
}

void GameBoardWidget::onTargetSelectionStarted(const QVector<int>& targetIds)
{
    m_state = State::SelectingTarget;
    // 遍历所有玩家，按 playerId 设置目标态（不依赖固定 playerId→widget 映射）
    for (auto* w : { m_topPlayerInfo, m_bottomPlayerInfo }) {
        w->setTargetable(targetIds.contains(w->playerId()));
    }
    m_actionPanel->setHint(QStringLiteral("请选择目标角色"));
}

void GameBoardWidget::onTargetSelectionFinished()
{
    m_topPlayerInfo->setTargetable(false);
    m_bottomPlayerInfo->setTargetable(false);
    if (m_state == State::SelectingTarget) {
        m_state = State::Idle;
    }
}

void GameBoardWidget::onPendingActionCreated(const PendingActionData& info)
{
    onTargetSelectionFinished();
    m_state = State::Responding;
    m_responderId = (info.requiredCardType == CardType::Peach)
            ? info.sourceId : info.targetId;
    m_actionPanel->updateForPendingAction(info);
    onLogMessage(info.description);
}

void GameBoardWidget::onPendingActionCleared()
{
    onTargetSelectionFinished();
    m_state = State::Idle;
    m_responderId = -1;
    // 恢复操作面板到当前阶段应有的按钮状态
    m_actionPanel->updateForPhase(m_currentPhase, false);
}

void GameBoardWidget::onGameOver(int winnerId)
{
    m_autoAdvanceTimer->stop();
    m_state = State::Idle;
    QString msg = (winnerId >= 0)
        ? QStringLiteral("游戏结束！获胜！")
        : QStringLiteral("游戏结束！平局！");
    m_logLabel->setText(msg);
    m_actionPanel->setHint(msg);
    QMessageBox::information(this, QStringLiteral("游戏结束"), msg);
    emit gameFinished();
}

void GameBoardWidget::onLogMessage(const QString& msg) { m_logLabel->setText(msg); }
void GameBoardWidget::refreshDisplay() {}

// ==================== 交互 ====================

void GameBoardWidget::onCardClicked(int cardId)
{
    switch (m_state) {
    case State::Idle:
        emit playCardRequested(cardId, m_currentPlayerId);
        break;
    case State::Responding:
        if (m_responderId >= 0)
            emit respondCardRequested(cardId, m_responderId);
        break;
    case State::Discarding:
        m_bottomHandArea->setSelection(cardId, true);
        break;
    case State::SelectingTarget:
        break;
    }
}

void GameBoardWidget::onCardDoubleClicked(int cardId) { onCardClicked(cardId); }

void GameBoardWidget::onTargetClicked(int playerIndex)
{
    if (m_state == State::SelectingTarget)
        emit targetPlayerSelected(playerIndex);
}

void GameBoardWidget::onPlayPhaseEnded()
{
    if (m_state == State::Idle) emit endPlayRequested();
}

void GameBoardWidget::onResponseSkipped()
{
    if (m_state == State::Responding) emit skipRequested();
}

void GameBoardWidget::onDiscardConfirmed()
{
    if (m_state == State::Discarding) {
        int selId = m_bottomHandArea->selectedCardId();
        if (selId >= 0) {
            emit discardCardRequested(selId, m_currentPlayerId);
            m_bottomHandArea->clearSelection();
        }
    }
}
