#include "GameBoardWidget.h"
#include "PlayerInfoWidget.h"
#include "HandCardAreaWidget.h"
#include "ActionPanelWidget.h"
#include "Theme.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFont>
#include <QPainter>
#include <QLinearGradient>
#include <QRadialGradient>

GameBoardWidget::GameBoardWidget(QWidget* parent) : QWidget(parent)
{
    m_autoAdvanceTimer = new QTimer(this);
    m_autoAdvanceTimer->setSingleShot(true);
    connect(m_autoAdvanceTimer, &QTimer::timeout, this, &GameBoardWidget::advanceRequested);

    m_logRevertTimer = new QTimer(this);
    m_logRevertTimer->setSingleShot(true);
    connect(m_logRevertTimer, &QTimer::timeout, this, &GameBoardWidget::revertLogToPhase);

    m_logPlayTimer = new QTimer(this);
    m_logPlayTimer->setSingleShot(true);
    connect(m_logPlayTimer, &QTimer::timeout, this, &GameBoardWidget::onPlayNextLog);

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
    connect(m_actionPanel, &ActionPanelWidget::skillRequested, this, &GameBoardWidget::onSkillClicked);
}

GameBoardWidget::~GameBoardWidget() = default;

void GameBoardWidget::setupLayout()
{
    auto* main = new QVBoxLayout(this);
    main->setContentsMargins(8, 4, 8, 4); main->setSpacing(4);

    m_topPlayerInfo = new PlayerInfoWidget(this); main->addWidget(m_topPlayerInfo);
    m_topHandArea = new HandCardAreaWidget(this); m_topHandArea->setMinimumHeight(120); main->addWidget(m_topHandArea);

    m_logLabel = new QLabel(this);
    m_logLabel->setObjectName(QStringLiteral("logLabel"));
    m_logLabel->setWordWrap(true); m_logLabel->setAlignment(Qt::AlignCenter); m_logLabel->setMinimumHeight(32);
    m_logLabel->setStyleSheet(Theme::hintBar(12));
    main->addWidget(m_logLabel);

    m_bottomHandArea = new HandCardAreaWidget(this); m_bottomHandArea->setMinimumHeight(120); main->addWidget(m_bottomHandArea);
    m_actionPanel = new ActionPanelWidget(this); main->addWidget(m_actionPanel);
    m_bottomPlayerInfo = new PlayerInfoWidget(this); main->addWidget(m_bottomPlayerInfo);

    main->setStretchFactor(m_topHandArea, 1);
    main->setStretchFactor(m_bottomHandArea, 1);
}

// ==================== 牌桌背景 ====================

void GameBoardWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 深绿绒面渐变（左上偏亮 → 右下偏暗）
    QLinearGradient felt(rect().topLeft(), rect().bottomRight());
    felt.setColorAt(0.0, Theme::TableGreenLight);
    felt.setColorAt(1.0, Theme::TableGreenDark);
    painter.fillRect(rect(), felt);

    // 暗角 vignette：中央通透、四周压暗，突出牌桌中心
    QRadialGradient vignette(rect().center(), qMax(width(), height()) * 0.75);
    vignette.setColorAt(0.0, QColor(0, 0, 0, 0));
    vignette.setColorAt(0.7, QColor(0, 0, 0, 0));
    vignette.setColorAt(1.0, Theme::TableVignette);
    painter.fillRect(rect(), vignette);

    // 内侧细金线描边，古风镶边感
    painter.setPen(QPen(QColor(212, 175, 55, 70), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(rect().adjusted(4, 4, -5, -5), 8, 8);
}

// ==================== 状态槽 ====================

void GameBoardWidget::onPhaseChanged(PhaseType phase)
{
    exitSkillSelection();
    m_currentPhase = phase;
    m_actionPanel->updateForPhase(phase, false);

    // 阶段切换：结算 log 已在此之前播完/失去意义，清空队列直接显示阶段提示
    clearLogQueue();
    m_logLabel->setStyleSheet(Theme::hintBar(12));
    m_logLabel->setText(phaseLogText(phase));

    // 先停掉上阶段的自动推进定时器，再按需重启。
    // 若 Draw 的 300ms 定时器在 Play 到达后仍触发，会把 Play 误推进到 Discard，
    // 网络 e2e 测试里表现为"点击杀无响应/手牌未移除"。
    m_autoAdvanceTimer->stop();

    switch (phase) {
    case PhaseType::Prepare:
    case PhaseType::Judge:
    case PhaseType::Draw:
    case PhaseType::End:
        m_autoAdvanceTimer->start(300);
        break;
    case PhaseType::Play:
        m_state = State::Idle;
        m_actionPanel->setSkillAvailable(
            m_skillAvailable && canControlPlayer(m_currentPlayerId));
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
    if (data.isCurrentPlayer) {
        m_currentPlayerId = playerId;
        m_skillAvailable = data.canUseActiveSkill;
        if (m_state == State::SelectingSkill && !m_skillAvailable) {
            exitSkillSelection();
        }
        if (m_currentPhase == PhaseType::Play && m_state == State::Idle) {
            m_actionPanel->setSkillAvailable(
                m_skillAvailable && canControlPlayer(m_currentPlayerId));
        }
    }
    if (data.isDying) {
        auto* w = (playerId == m_localPlayerId) ? m_bottomPlayerInfo : m_topPlayerInfo;
        w->setStyleSheet(Theme::panelDying());
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
    exitSkillSelection();
    onTargetSelectionFinished();
    m_state = State::Responding;
    m_responderId = (info.requiredCardType == CardType::Peach)
            ? info.sourceId : info.targetId;
    m_actionPanel->updateForPendingAction(info);
    // 响应提示常驻，直到有结算 log 或阶段切换：清空残留结算 log 队列，立即显示响应提示
    clearLogQueue();
    m_autoAdvanceTimer->stop();  // 响应中禁止自动推进
    m_logLabel->setStyleSheet(Theme::hintBar(12));
    m_logLabel->setText(info.description);
}

void GameBoardWidget::onPendingActionCleared()
{
    onTargetSelectionFinished();
    m_state = State::Idle;
    m_responderId = -1;
    // 恢复操作面板到当前阶段应有的按钮状态
    m_actionPanel->updateForPhase(m_currentPhase, false);
    if (m_currentPhase == PhaseType::Play) {
        m_actionPanel->setSkillAvailable(
            m_skillAvailable && canControlPlayer(m_currentPlayerId));
    }
    // 若队列中仍有结算 log 在播放，交给 onPlayNextLog 播完后自然回退；
    // 否则（无 log 播放且队列空）2s 后回退到阶段提示
    if (!m_logPlaying && m_logQueue.isEmpty() && !m_logRevertTimer->isActive()) {
        m_logRevertTimer->start(2000);
    }
}

void GameBoardWidget::onGameOver(int winnerId)
{
    exitSkillSelection();
    m_autoAdvanceTimer->stop();
    clearLogQueue();
    m_state = State::Idle;
    QString msg;
    if (winnerId < 0) {
        msg = QStringLiteral("游戏结束！平局！");
    } else if (m_restrictToLocalPlayer) {
        msg = (winnerId == m_localPlayerId)
            ? QStringLiteral("游戏结束！你获胜！")
            : QStringLiteral("游戏结束！你失败了。");
    } else {
        msg = QStringLiteral("游戏结束！玩家%1获胜！").arg(winnerId + 1);
    }
    m_logLabel->setText(msg);
    m_actionPanel->setHint(msg);
    QMessageBox::information(this, QStringLiteral("游戏结束"), msg);
    emit gameFinished();
}

void GameBoardWidget::onSkillClicked()
{
    if (m_state == State::Idle) {
        if (m_currentPhase != PhaseType::Play || !m_skillAvailable ||
            !canControlPlayer(m_currentPlayerId)) {
            return;
        }

        auto* area = handAreaForPlayer(m_currentPlayerId);
        area->clearSelection();
        area->setMultiSelectMode(true);
        m_state = State::SelectingSkill;
        m_actionPanel->setSkillSelectionMode(true);
        m_actionPanel->setHint(QStringLiteral("请选择要用于技能的手牌，再点击确认技能"));
        return;
    }

    if (m_state != State::SelectingSkill) return;

    auto* area = handAreaForPlayer(m_currentPlayerId);
    const QVector<int> selectedIds = area->selectedCardIds();
    if (selectedIds.isEmpty()) {
        exitSkillSelection();
        return;
    }

    const int playerId = m_currentPlayerId;
    exitSkillSelection();
    emit skillRequested(selectedIds, playerId);
}

void GameBoardWidget::onLogMessage(const QString& msg)
{
    // 结算类 log 入队顺序播放，避免同一调用栈内多条 log 互相覆盖
    enqueueLog({msg, QString()});
}

void GameBoardWidget::onJudgmentPerformed(const CardData& judgeCard, const QString& resultText, bool effective)
{
    // 在日志区域显示判定结果，格式如: "判定牌: ♠7  【乐不思蜀】判定: 非♥，跳过出牌阶段"
    QString display = QStringLiteral("判定牌: ") + judgeCard.suitSymbol + judgeCard.numberString
                    + QStringLiteral("  ") + judgeCard.cardName
                    + QStringLiteral("  →  ") + resultText;
    // 生效时高亮日志区域（红色），否则绿色（深色主题变体）
    enqueueLog({display, Theme::judgmentBar(effective)});
}

void GameBoardWidget::enqueueLog(const LogEntry& entry)
{
    m_logQueue.enqueue(entry);
    // 若当前没有 log 正在停留展示，立即开始播放队首
    if (!m_logPlaying) {
        onPlayNextLog();
    }
}

void GameBoardWidget::onPlayNextLog()
{
    if (m_logQueue.isEmpty()) {
        // 队列播放完毕：短暂停留后回退到当前阶段提示
        m_logPlaying = false;
        m_logRevertTimer->start(2000);
        return;
    }

    const LogEntry entry = m_logQueue.dequeue();
    m_logLabel->setStyleSheet(entry.style.isEmpty() ? Theme::hintBar(12) : entry.style);
    m_logLabel->setText(entry.text);
    m_logPlaying = true;
    m_logRevertTimer->stop();       // 展示期间不回退
    m_logPlayTimer->start(1200);    // 每条至少停留约 1.2s
}

void GameBoardWidget::clearLogQueue()
{
    m_logQueue.clear();
    m_logPlaying = false;
    m_logPlayTimer->stop();
    m_logRevertTimer->stop();
}

void GameBoardWidget::revertLogToPhase()
{
    m_logLabel->setStyleSheet(Theme::hintBar(12));
    m_logLabel->setText(phaseLogText(m_currentPhase));
}

QString GameBoardWidget::phaseLogText(PhaseType phase)
{
    switch (phase) {
    case PhaseType::Prepare:
        return QStringLiteral("准备阶段");
    case PhaseType::Judge:
        return QStringLiteral("判定阶段");
    case PhaseType::Draw:
        return QStringLiteral("摸牌阶段");
    case PhaseType::Play:
        return QStringLiteral("出牌阶段");
    case PhaseType::Discard:
        return QStringLiteral("弃牌阶段");
    case PhaseType::End:
        return QStringLiteral("结束阶段");
    }
    return QStringLiteral("准备中...");
}

void GameBoardWidget::refreshDisplay() {}

// ==================== 手牌区定位 ====================

HandCardAreaWidget* GameBoardWidget::handAreaForPlayer(int playerId) const
{
    return (playerId == m_localPlayerId) ? m_bottomHandArea : m_topHandArea;
}

bool GameBoardWidget::canControlPlayer(int playerId) const
{
    return !m_restrictToLocalPlayer || playerId == m_localPlayerId;
}

void GameBoardWidget::exitSkillSelection()
{
    if (m_state != State::SelectingSkill) return;

    auto* area = handAreaForPlayer(m_currentPlayerId);
    area->clearSelection();
    area->setMultiSelectMode(false);
    m_state = State::Idle;
    m_actionPanel->setSkillSelectionMode(false);
    m_actionPanel->updateForPhase(m_currentPhase, false);
    if (m_currentPhase == PhaseType::Play) {
        m_actionPanel->setSkillAvailable(
            m_skillAvailable && canControlPlayer(m_currentPlayerId));
    }
}

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
        handAreaForPlayer(m_currentPlayerId)->setSelection(cardId, true);
        break;
    case State::SelectingSkill:
        // HandCardAreaWidget 已在多选模式中切换了该牌的选中状态。
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
        auto* area = handAreaForPlayer(m_currentPlayerId);
        int selId = area->selectedCardId();
        if (selId >= 0) {
            emit discardCardRequested(selId, m_currentPlayerId);
            area->clearSelection();
        }
    }
}
