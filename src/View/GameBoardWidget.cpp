#include "GameBoardWidget.h"
#include "PlayerInfoWidget.h"
#include "HandCardAreaWidget.h"
#include "ActionPanelWidget.h"

#include "GameViewModel.h"
#include "PlayerViewModel.h"
#include "CardViewModel.h"
#include "ActionViewModel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QMessageBox>
#include <QMetaObject>
#include <QFont>

// ==================== 构造 / 析构 ====================

GameBoardWidget::GameBoardWidget(GameViewModel* gvm, QWidget* parent)
    : QWidget(parent)
    , m_gvm(gvm)
{
    setupLayout();
    connectViewModel();

    // 自动推进计时器
    m_autoAdvanceTimer = new QTimer(this);
    m_autoAdvanceTimer->setSingleShot(true);
    connect(m_autoAdvanceTimer, &QTimer::timeout, this, &GameBoardWidget::autoAdvancePhase);

    // 初始刷新
    refreshDisplay();

    // 立即推进到出牌阶段，不卡在自动阶段
    QMetaObject::invokeMethod(this, [this]() {
        m_gvm->advanceToInteractivePhase();
        showLog(QStringLiteral("出牌阶段 — 双击卡牌快速打出"));
        refreshDisplay();
    }, Qt::QueuedConnection);
}

GameBoardWidget::~GameBoardWidget()
{
    disconnectViewModel();
}

// ==================== 布局构建 ====================

void GameBoardWidget::setupLayout()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 4, 8, 4);
    mainLayout->setSpacing(4);

    // ====== 顶栏：对手信息 + 对手手牌 ======

    m_topPlayerInfo = new PlayerInfoWidget(this);
    mainLayout->addWidget(m_topPlayerInfo);

    m_topHandArea = new HandCardAreaWidget(this);
    m_topHandArea->setMinimumHeight(120);
    mainLayout->addWidget(m_topHandArea);

    // ====== 中央区域：日志文字 ======

    m_logLabel = new QLabel(this);
    m_logLabel->setWordWrap(true);
    m_logLabel->setAlignment(Qt::AlignCenter);
    m_logLabel->setMinimumHeight(32);
    m_logLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 12px; color: #444;"
        "  padding: 6px 12px;"
        "  background: #F5F5F5;"
        "  border: 1px solid #E0E0E0;"
        "  border-radius: 6px;"
        "}"
    );
    mainLayout->addWidget(m_logLabel);

    // ====== 己方手牌区 ======

    m_bottomHandArea = new HandCardAreaWidget(this);
    m_bottomHandArea->setMinimumHeight(120);
    mainLayout->addWidget(m_bottomHandArea);

    // ====== 操作面板 ======

    m_actionPanel = new ActionPanelWidget(this);
    mainLayout->addWidget(m_actionPanel);

    // ====== 己方信息 ======

    m_bottomPlayerInfo = new PlayerInfoWidget(this);
    mainLayout->addWidget(m_bottomPlayerInfo);

    // 主布局弹性伸缩
    mainLayout->setStretchFactor(m_topHandArea, 1);
    mainLayout->setStretchFactor(m_bottomHandArea, 1);
}

// ==================== 事件桥接 ====================

void GameBoardWidget::connectViewModel()
{
    if (!m_gvm) return;

    // phaseChanged → 阶段切换
    auto id1 = m_gvm->phaseChanged.connect([this](PhaseType phase) {
        QMetaObject::invokeMethod(this, [this, phase]() {
            onPhaseChanged(phase);
        }, Qt::QueuedConnection);
    });
    m_connectionIds.push_back(id1);

    // currentPlayerChanged → 玩家切换（int playerIndex）
    auto id2 = m_gvm->currentPlayerChanged.connect([this](int idx) {
        QMetaObject::invokeMethod(this, [this, idx]() {
            onPlayerChanged(idx);
        }, Qt::QueuedConnection);
    });
    m_connectionIds.push_back(id2);

    // gameOver → 游戏结束（int winnerId）
    auto id3 = m_gvm->gameOver.connect([this](int winnerId) {
        QMetaObject::invokeMethod(this, [this, winnerId]() {
            onGameOver(winnerId);
        }, Qt::QueuedConnection);
    });
    m_connectionIds.push_back(id3);

    // stateChanged → 全面刷新
    auto id4 = m_gvm->stateChanged.connect([this]() {
        QMetaObject::invokeMethod(this, [this]() {
            refreshDisplay();
        }, Qt::QueuedConnection);
    });
    m_connectionIds.push_back(id4);

    // logMessage → 日志显示
    auto id5 = m_gvm->logMessage.connect([this](const std::string& msg) {
        QMetaObject::invokeMethod(this, [this, msg]() {
            showLog(QString::fromStdString(msg));
        }, Qt::QueuedConnection);
    });
    m_connectionIds.push_back(id5);

    // pendingActionCreated → 响应模式（PendingActionVM）
    auto id6 = m_gvm->pendingActionCreated.connect(
        [this](const PendingActionVM& info) {
            QMetaObject::invokeMethod(this, [this, info]() {
                onPendingActionCreated(info);
            }, Qt::QueuedConnection);
        });
    m_connectionIds.push_back(id6);

    // pendingActionCleared → 退出响应模式
    auto id7 = m_gvm->pendingActionCleared.connect(
        [this]() {
            QMetaObject::invokeMethod(this, [this]() {
                onPendingActionCleared();
            }, Qt::QueuedConnection);
        });
    m_connectionIds.push_back(id7);

    // 子控件信号
    connect(m_bottomHandArea, &HandCardAreaWidget::cardClicked,
            this, &GameBoardWidget::onCardClicked);
    connect(m_bottomHandArea, &HandCardAreaWidget::cardDoubleClicked,
            this, &GameBoardWidget::onCardDoubleClicked);

    // 对手手牌也可点击（双人同屏模式）
    connect(m_topHandArea, &HandCardAreaWidget::cardClicked,
            this, &GameBoardWidget::onCardClicked);
    connect(m_topHandArea, &HandCardAreaWidget::cardDoubleClicked,
            this, &GameBoardWidget::onCardDoubleClicked);

    connect(m_topPlayerInfo, &PlayerInfoWidget::clicked,
            this, &GameBoardWidget::onTargetClicked);
    connect(m_bottomPlayerInfo, &PlayerInfoWidget::clicked,
            this, &GameBoardWidget::onTargetClicked);

    connect(m_actionPanel, &ActionPanelWidget::playPhaseEnded,
            this, &GameBoardWidget::onPlayPhaseEnded);
    connect(m_actionPanel, &ActionPanelWidget::respondSkipped,
            this, &GameBoardWidget::onResponseSkipped);
    connect(m_actionPanel, &ActionPanelWidget::discardConfirmed,
            this, &GameBoardWidget::onDiscardConfirmed);
}

void GameBoardWidget::disconnectViewModel()
{
    if (!m_gvm) return;

    for (auto id : m_connectionIds) {
        m_gvm->phaseChanged.disconnect(id);
        m_gvm->currentPlayerChanged.disconnect(id);
        m_gvm->gameOver.disconnect(id);
        m_gvm->stateChanged.disconnect(id);
        m_gvm->logMessage.disconnect(id);
        m_gvm->pendingActionCreated.disconnect(id);
        m_gvm->pendingActionCleared.disconnect(id);
    }
    m_connectionIds.clear();
}

// ==================== 阶段切换 ====================

void GameBoardWidget::onPhaseChanged(PhaseType phase)
{
    // 如果游戏状态已经越过此阶段（初始批处理的残留事件），跳过自动推进
    PhaseType currentPhase = m_gvm ? m_gvm->currentPhase() : phase;
    if (currentPhase != phase) {
        refreshDisplay();
        return;
    }

    m_actionPanel->updateForPhase(phase, m_gvm->hasPendingAction());

    // 非交互阶段自动推进（使用极短延迟让事件循环先处理完当前事件）
    switch (phase) {
    case PhaseType::Prepare:
    case PhaseType::Judge:
    case PhaseType::Draw:
    case PhaseType::End:
        QMetaObject::invokeMethod(this, "autoAdvancePhase", Qt::QueuedConnection);
        break;

    case PhaseType::Play:
        m_state = State::Idle;
        break;

    case PhaseType::Discard:
    {
        // 检查是否需要弃牌
        ActionViewModel* avm = m_gvm->actionVM();
        int curId = m_gvm->currentPlayerId();
        int discardCount = avm->getDiscardCount(curId);
        if (discardCount <= 0) {
            // 不需要弃牌，自动跳过
            QMetaObject::invokeMethod(this, "autoAdvancePhase", Qt::QueuedConnection);
        } else {
            m_state = State::Discarding;
            m_requiredDiscardCount = discardCount;
            m_actionPanel->setHint(
                QString(QStringLiteral("请选择 %1 张牌弃置")).arg(discardCount));
        }
        break;
    }
    }

    refreshDisplay();
}

void GameBoardWidget::onPlayerChanged(int /*playerIndex*/)
{
    // 刷新两个玩家信息面板
    if (m_gvm->playerVM(0)) m_bottomPlayerInfo->bindViewModel(m_gvm->playerVM(0));
    if (m_gvm->playerVM(1)) m_topPlayerInfo->bindViewModel(m_gvm->playerVM(1));

    // 确定当前玩家，设置对应的面板标记
    int curId = m_gvm->currentPlayerId();
    m_bottomPlayerInfo->setCurrentPlayer(curId == 0);
    m_topPlayerInfo->setCurrentPlayer(curId == 1);

    // 刷新手牌
    refreshHandCards();
    refreshDisplay();
}

// ==================== 待定动作处理 ====================

void GameBoardWidget::onPendingActionCreated(const PendingActionVM& info)
{
    m_state = State::Responding;

    // 更新操作面板
    m_actionPanel->updateForPendingAction(info);

    // 刷新响应方可用的手牌
    ActionViewModel* avm = m_gvm->actionVM();

    // 获取可响应的牌 ID 列表
    std::vector<int> responseCardIds =
        avm->getResponseCardIds(info.targetId, info.requiredCardType);

    // 如果没有可用的响应牌，自动跳过/扣血推进游戏
    if (responseCardIds.empty()) {
        // 通过 ViewModel 获取显示名
        std::string name = m_gvm->playerDisplayName(info.targetId);
        showLog(QString::fromStdString(name + " 无牌可响应，自动推进"));
        avm->skipResponse(info.targetId, true);  // forceNoCard=true
        return;
    }

    // 获取 target 的手牌 CardViewModel 并标记可响应的
    auto cardVMs = m_gvm->getPlayerCardVMs(info.targetId);
    for (auto& cvm : cardVMs) {
        if (avm->isValidResponseCard(cvm->id(), info.targetId, info.requiredCardType)) {
            cvm->setPlayable(true);
        }
    }

    // 显示到对应玩家的手牌区
    if (info.targetId == 0) {
        m_bottomHandArea->setCards(std::move(cardVMs), false);
        m_bottomHandArea->refreshPlayableState();
    } else {
        m_topHandArea->setCards(std::move(cardVMs), false);
    }

    showLog(QString::fromStdString(info.description));
}

void GameBoardWidget::onPendingActionCleared()
{
    m_state = State::Idle;

    // 恢复正常显示
    m_actionPanel->updateForPhase(m_gvm->currentPhase(), false);

    // 检查是否有后续待定动作
    if (m_gvm->hasPendingAction()) {
        onPendingActionCreated(m_gvm->pendingActionVM());
    }

    refreshDisplay();
}

// ==================== 交互处理 ====================

void GameBoardWidget::onCardClicked(int cardId)
{
    if (cardId < 0 || !m_gvm) return;

    int curPlayerId = m_gvm->currentPlayerId();

    switch (m_state) {
    case State::Idle:
    {
        // 出牌阶段 — 尝试使用牌
        if (m_gvm->currentPhase() != PhaseType::Play) return;
        if (m_gvm->hasPendingAction()) return;

        // 只能使用当前玩家自己的手牌
        ActionViewModel* avm = m_gvm->actionVM();
        if (!avm->isOwnCard(cardId, curPlayerId)) {
            showLog(QStringLiteral("不能使用对方的牌"));
            return;
        }

        // 综合检查：卡牌自身可用性 + 游戏规则
        if (!avm->canPlayCard(cardId, curPlayerId)) {
            showLog(QStringLiteral("这张牌不能使用"));
            return;
        }

        // 获取合法目标（int IDs）
        std::vector<int> targetIds = avm->getValidTargetIds(cardId, curPlayerId);

        // 获取卡牌名用于日志（通过 CardViewModel）
        CardViewModel* cvm = findCardVM(cardId);
        QString cardName = cvm ? QString::fromStdString(cvm->cardName())
                               : QStringLiteral("未知");

        if (targetIds.empty()) {
            // 无目标，直接使用（桃/酒/无中生有）
            showLog(QStringLiteral("使用: ") + cardName);
            avm->playCard(cardId, curPlayerId, {});
            refreshDisplay();
        } else if (targetIds.size() == 1) {
            // 单一目标，自动选择
            std::string targetName = m_gvm->playerDisplayName(targetIds[0]);
            showLog(QStringLiteral("使用: ") + cardName
                    + QStringLiteral(" → ") + QString::fromStdString(targetName));
            avm->playCard(cardId, curPlayerId, {targetIds[0]});
            refreshDisplay();
        } else {
            // 多目标，进入目标选择模式
            enterTargetSelection(cardId, curPlayerId, targetIds);
        }
        break;
    }

    case State::Responding:
    {
        // 响应模式 — 打出这张牌作为响应
        if (!m_gvm->hasPendingAction()) return;

        const PendingActionVM& info = m_gvm->pendingActionVM();
        ActionViewModel* avm = m_gvm->actionVM();
        int responderId = info.targetId;

        // 使用 ViewModel 方法检查响应是否有效
        if (!avm->isValidResponseCard(cardId, responderId, info.requiredCardType)) {
            showLog(QStringLiteral("这张牌不能用于响应"));
            return;
        }

        avm->respondCard(cardId, responderId);
        break;
    }

    case State::Discarding:
    {
        // 弃牌阶段 — 切换选牌状态
        // 已选中的牌会通过 CardWidget 的 selected 状态显示
        // 确认弃牌时收集所有选中的牌一起弃置
        break;
    }

    case State::SelectingTarget:
        break;
    }
}

void GameBoardWidget::onCardDoubleClicked(int cardId)
{
    // 双击等同于单击出牌
    onCardClicked(cardId);
}

void GameBoardWidget::onTargetClicked(int playerIndex)
{
    if (m_state != State::SelectingTarget || m_pendingCardId < 0) return;

    // 检查点击的目标是否在合法列表中
    for (int tid : m_pendingTargetIds) {
        if (tid == playerIndex) {
            // 找到目标，执行出牌
            ActionViewModel* avm = m_gvm->actionVM();
            CardViewModel* cvm = findCardVM(m_pendingCardId);
            QString cardName = cvm ? QString::fromStdString(cvm->cardName())
                                   : QStringLiteral("未知");
            std::string targetName = m_gvm->playerDisplayName(tid);
            showLog(QStringLiteral("使用: ") + cardName
                    + QStringLiteral(" → ") + QString::fromStdString(targetName));
            avm->playCard(m_pendingCardId, m_pendingCardUserId, {tid});
            exitTargetSelection();
            refreshDisplay();
            return;
        }
    }

    // 点击了非目标玩家，提示
    showLog(QStringLiteral("请选择高亮的目标"));
}

void GameBoardWidget::onPlayPhaseEnded()
{
    if (m_gvm->hasPendingAction()) return;
    if (m_gvm->currentPhase() != PhaseType::Play) return;

    m_gvm->endPlayPhase();
}

void GameBoardWidget::onResponseSkipped()
{
    if (!m_gvm->hasPendingAction()) return;

    const PendingActionVM& info = m_gvm->pendingActionVM();
    if (!info.canSkip) return;

    ActionViewModel* avm = m_gvm->actionVM();
    avm->skipResponse(info.targetId);
}

void GameBoardWidget::onDiscardConfirmed()
{
    if (m_state != State::Discarding) return;
    if (!m_gvm) return;

    int curId = m_gvm->currentPlayerId();
    ActionViewModel* avm = m_gvm->actionVM();

    // 收集当前所有选中的牌（通过 cardId）
    int selectedId = m_bottomHandArea->selectedCardId();
    if (selectedId < 0) {
        showLog(QStringLiteral("请先选择要弃置的牌"));
        return;
    }

    avm->discardCard(selectedId, curId);
    refreshHandCards();

    // 检查是否还需要弃牌
    int remaining = avm->getDiscardCount(curId);
    if (remaining <= 0) {
        showLog(QStringLiteral("弃牌完成"));
        m_gvm->advancePhase();  // 进入结束阶段
    } else {
        m_actionPanel->setHint(
            QString(QStringLiteral("还需弃置 %1 张")).arg(remaining));
    }
}

void GameBoardWidget::onGameOver(int winnerId)
{
    m_autoAdvanceTimer->stop();
    m_state = State::Idle;

    QString msg;
    if (winnerId >= 0) {
        // 通过 PlayerViewModel 获取显示信息
        PlayerViewModel* pvm = m_gvm->playerVM(winnerId);
        if (pvm) {
            msg = QStringLiteral("游戏结束！%1（%2）获胜！")
                  .arg(QString::fromStdString(pvm->displayName()))
                  .arg(QString::fromStdString(pvm->characterName()));
        }
    } else {
        msg = QStringLiteral("游戏结束！平局！");
    }

    m_logLabel->setText(msg);
    m_actionPanel->setHint(msg);

    QMessageBox::information(this, QStringLiteral("游戏结束"), msg);
    emit gameFinished();
}

// ==================== 辅助方法 ====================

void GameBoardWidget::autoAdvancePhase()
{
    if (!m_gvm || m_gvm->isGameOver()) return;
    if (m_gvm->hasPendingAction()) return;

    m_gvm->advancePhase();
}

void GameBoardWidget::showLog(const QString& msg)
{
    m_logLabel->setText(msg);
}

CardViewModel* GameBoardWidget::findCardVM(int cardId) const
{
    // 先在己方手牌区查找
    if (auto* cvm = m_bottomHandArea->cardVM(cardId)) return cvm;
    // 再在对手手牌区查找
    return m_topHandArea->cardVM(cardId);
}

void GameBoardWidget::enterTargetSelection(
    int cardId, int userId, const std::vector<int>& targetIds)
{
    m_state = State::SelectingTarget;
    m_pendingCardId = cardId;
    m_pendingCardUserId = userId;
    m_pendingTargetIds = targetIds;

    // 高亮可点击的目标
    for (int tid : targetIds) {
        if (tid == 0) m_bottomPlayerInfo->setTargetable(true);
        if (tid == 1) m_topPlayerInfo->setTargetable(true);
    }

    m_actionPanel->setHint(QStringLiteral("请选择目标"));
    showLog(QStringLiteral("请点击高亮的目标玩家"));
}

void GameBoardWidget::exitTargetSelection()
{
    m_state = State::Idle;
    m_pendingCardId = -1;
    m_pendingCardUserId = -1;
    m_pendingTargetIds.clear();

    // 取消高亮
    m_bottomPlayerInfo->setTargetable(false);
    m_topPlayerInfo->setTargetable(false);
}

void GameBoardWidget::refreshHandCards()
{
    if (!m_gvm) return;

    // 己方手牌（正面朝上）
    auto ownCards = m_gvm->getCurrentPlayerCardVMs();
    m_bottomHandArea->setCards(std::move(ownCards), false);

    // 对手手牌（正面朝上，双人本地模式双方可见）
    int oppId = m_gvm->opponentPlayerId();
    if (oppId >= 0) {
        auto oppCards = m_gvm->getPlayerCardVMs(oppId);
        m_topHandArea->setCards(std::move(oppCards), false);
    }
}

void GameBoardWidget::refreshDisplay()
{
    if (!m_gvm) return;

    // 刷新玩家信息
    auto* pvm0 = m_gvm->playerVM(0);
    auto* pvm1 = m_gvm->playerVM(1);
    if (pvm0) m_bottomPlayerInfo->bindViewModel(pvm0);
    if (pvm1) m_topPlayerInfo->bindViewModel(pvm1);

    // 确认当前玩家
    int curId = m_gvm->currentPlayerId();
    m_bottomPlayerInfo->setCurrentPlayer(curId == 0);
    m_topPlayerInfo->setCurrentPlayer(curId == 1);

    // 刷新手牌
    refreshHandCards();
}
