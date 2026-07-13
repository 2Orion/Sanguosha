#include "GameViewModel.h"
#include "PlayerViewModel.h"
#include "ActionViewModel.h"
#include "CardViewModel.h"
#include "GameRule.h"
#include "CardManager.h"
#include "Player.h"
#include "Card.h"
#include "Character.h"

// ==================== 构造 / 析构 ====================

GameViewModel::GameViewModel(QObject* parent)
    : QObject(parent)
{
    m_state = std::make_unique<GameState>(this);
    m_cardManager = std::make_unique<CardManager>(this);
    m_actionVM = std::make_unique<ActionViewModel>(this);

    m_state->setCardManager(m_cardManager.get());
    m_actionVM->setGameState(m_state.get());
}

GameViewModel::~GameViewModel() = default;

// ==================== 游戏生命周期 ====================

void GameViewModel::startGame(int characterId1, int characterId2)
{
    Character* char1 = createCharacterById(characterId1);
    Character* char2 = createCharacterById(characterId2);
    if (!char1 || !char2) return;

    initGame(char1, char2);
}

void GameViewModel::advancePhase()
{
    if (m_state->isGameOver()) return;

    switch (m_state->currentPhase()) {
    case PhaseType::Prepare:
        executePhasePrepare();
        setNextPhase(PhaseType::Judge);
        break;
    case PhaseType::Judge:
        executePhaseJudge();
        setNextPhase(PhaseType::Draw);
        break;
    case PhaseType::Draw:
        executePhaseDraw();
        setNextPhase(PhaseType::Play);
        break;
    case PhaseType::Play:
        setNextPhase(PhaseType::Discard);
        break;
    case PhaseType::Discard:
        setNextPhase(PhaseType::End);
        break;
    case PhaseType::End:
        executePhaseEnd();
        break;
    }
}

void GameViewModel::endPlayPhase()
{
    if (m_state->isGameOver()) return;
    if (m_state->currentPhase() != PhaseType::Play) return;
    if (m_state->hasPendingAction()) return;
    setNextPhase(PhaseType::Discard);
}

// ==================== 状态查询 ====================

int GameViewModel::currentPlayerId() const
{
    Player* p = currentPlayer();
    return p ? p->playerId() : -1;
}

int GameViewModel::opponentPlayerId() const
{
    Player* cur = currentPlayer();
    if (!cur) return -1;
    for (Player* p : m_state->allPlayers()) {
        if (p && p != cur && p->isAlive()) return p->playerId();
    }
    return -1;
}

PhaseType GameViewModel::currentPhase() const { return m_state->currentPhase(); }
int GameViewModel::turnCount() const { return m_state->turnCount(); }
bool GameViewModel::isGameOver() const { return m_state->isGameOver(); }

int GameViewModel::winnerId() const
{
    Player* w = m_state->winner();
    return w ? w->playerId() : -1;
}

QString GameViewModel::phaseName(PhaseType phase)
{
    switch (phase) {
    case PhaseType::Prepare: return QStringLiteral("准备阶段");
    case PhaseType::Judge:   return QStringLiteral("判定阶段");
    case PhaseType::Draw:    return QStringLiteral("摸牌阶段");
    case PhaseType::Play:    return QStringLiteral("出牌阶段");
    case PhaseType::Discard: return QStringLiteral("弃牌阶段");
    case PhaseType::End:     return QStringLiteral("结束阶段");
    }
    return QStringLiteral("未知");
}

// ==================== 待定动作 ====================

bool GameViewModel::hasPendingAction() const
{
    return m_state ? m_state->hasPendingAction() : false;
}

PendingActionVM GameViewModel::pendingActionVM() const
{
    PendingActionVM result;
    if (!m_state || !m_state->hasPendingAction()) return result;

    const PendingActionInfo& info = m_state->pendingActionInfo();
    result.sourceId      = info.source      ? info.source->playerId()      : -1;
    result.targetId      = info.target      ? info.target->playerId()      : -1;
    result.sourceCardId  = info.sourceCard  ? info.sourceCard->id()        : -1;
    result.requiredCardType = info.requiredCardType;
    result.description   = QString::fromStdString(info.description);
    result.canSkip       = info.canSkip;
    for (Player* p : info.remainingTargets) {
        if (p) result.remainingTargetIds.push_back(p->playerId());
    }
    return result;
}

// ==================== 子 ViewModel ====================

PlayerViewModel* GameViewModel::playerVM(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_playerVMs.size())) {
        return m_playerVMs[index].get();
    }
    return nullptr;
}

ActionViewModel* GameViewModel::actionVM() const { return m_actionVM.get(); }
GameState* GameViewModel::gameState() const { return m_state.get(); }

// ==================== 手牌展示辅助 ====================

std::vector<std::unique_ptr<CardViewModel>> GameViewModel::getCurrentPlayerCardVMs() const
{
    return getPlayerCardVMs(currentPlayerId());
}

std::vector<std::unique_ptr<CardViewModel>> GameViewModel::getPlayerCardVMs(int playerIndex) const
{
    std::vector<std::unique_ptr<CardViewModel>> result;
    Player* player = playerByIndex(playerIndex);
    if (!player || !m_state) return result;

    std::vector<int> playableIds = m_actionVM->getPlayableCardIds(playerIndex);

    for (Card* card : player->handCards()) {
        if (!card) continue;
        auto cvm = std::make_unique<CardViewModel>(card);

        bool isPlayable = false;
        for (int pid : playableIds) {
            if (pid == card->id()) { isPlayable = true; break; }
        }
        cvm->setPlayable(isPlayable);
        result.push_back(std::move(cvm));
    }
    return result;
}

// ==================== 显示数据查找 ====================

QString GameViewModel::cardNameById(int cardId) const
{
    Card* card = findCard(cardId);
    return card ? QString::fromStdString(card->cardName()) : QString();
}

CardType GameViewModel::cardTypeById(int cardId) const
{
    Card* card = findCard(cardId);
    return card ? card->cardType() : CardType::Kill;
}

QString GameViewModel::playerDisplayName(int playerId) const
{
    Player* p = playerByIndex(playerId);
    return p ? p->displayName() : QString();
}

// ==================== 初始化 ====================

void GameViewModel::initGame(Character* char1, Character* char2)
{
    // 1. 初始化牌堆
    m_cardManager->initialize();

    // 2. 创建玩家（Player 现在是 QObject，设 m_state 为父对象）
    auto* player1 = new Player(m_state.get());
    player1->setPlayerId(0);
    player1->setDisplayName(QStringLiteral("玩家1"));
    player1->setCharacter(char1);

    auto* player2 = new Player(m_state.get());
    player2->setPlayerId(1);
    player2->setDisplayName(QStringLiteral("玩家2"));
    player2->setCharacter(char2);

    m_state->addPlayer(player1);
    m_state->addPlayer(player2);

    // 3. 发初始手牌
    for (int i = 0; i < GameRule::INITIAL_HAND_COUNT; ++i) {
        Card* c1 = m_cardManager->drawCard();
        Card* c2 = m_cardManager->drawCard();
        if (c1) player1->addHandCard(c1);
        if (c2) player2->addHandCard(c2);
    }

    // 4. 创建 PlayerViewModel
    m_playerVMs.push_back(std::make_unique<PlayerViewModel>(player1, this));
    m_playerVMs.push_back(std::make_unique<PlayerViewModel>(player2, this));

    // 5. 监听 Model 的 Qt 信号
    m_modelConnections.push_back(
        connect(m_state.get(), &GameState::phaseChanged,
                this, &GameViewModel::phaseChanged));
    m_modelConnections.push_back(
        connect(m_state.get(), &GameState::currentPlayerChanged,
                this, &GameViewModel::currentPlayerChanged));
    m_modelConnections.push_back(
        connect(m_state.get(), &GameState::pendingActionCreated,
                this, &GameViewModel::onModelPendingActionCreated));
    m_modelConnections.push_back(
        connect(m_state.get(), &GameState::pendingActionCleared,
                this, &GameViewModel::onModelPendingActionCleared));
    m_modelConnections.push_back(
        connect(m_state.get(), &GameState::gameOver,
                this, &GameViewModel::onModelGameOver));
    m_modelConnections.push_back(
        connect(m_state.get(), &GameState::stateRefreshed,
                this, &GameViewModel::stateChanged));

    // 6. 监听玩家死亡
    for (Player* p : m_state->allPlayers()) {
        if (!p) continue;
        m_modelConnections.push_back(
            connect(p, &Player::died,
                    this, &GameViewModel::onModelPlayerDied));
    }

    // 7. 开始第一个回合
    m_state->setCurrentPhase(PhaseType::Prepare);
    emitLog(QStringLiteral("游戏开始！") + player1->displayName() +
            QStringLiteral("（") + player1->characterName() +
            QStringLiteral("）vs ") +
            player2->displayName() + QStringLiteral("（") +
            player2->characterName() + QStringLiteral("）"));
}

Character* GameViewModel::createCharacterById(int id)
{
    switch (id) {
    case 0: return new CaoCao(this);
    case 1: return new GuanYu(this);
    case 2: return new ZhangFei(this);
    case 3: return new ZhaoYun(this);
    default: return nullptr;
    }
}

// ==================== 阶段执行 ====================

void GameViewModel::executePhasePrepare()
{
    Player* player = currentPlayer();
    if (!player) return;
    player->resetTurnState();

    if (player->character() &&
        player->character()->triggerCondition(GameEvent::OnTurnStart,
                                               m_state.get(), player)) {
        player->character()->triggerSkill(m_state.get(), player);
        emitLog(player->displayName() + QStringLiteral(" 触发了技能【") +
                QString::fromStdString(player->character()->skillName()) +
                QStringLiteral("】"));
    }
}

void GameViewModel::executePhaseJudge() {}

void GameViewModel::executePhaseDraw()
{
    Player* player = currentPlayer();
    if (!player) return;

    std::vector<Card*> cards = m_cardManager->drawCards(GameRule::DRAW_PHASE_COUNT);
    for (Card* card : cards) {
        if (card) player->addHandCard(card);
    }

    if (player->character() &&
        player->character()->triggerCondition(GameEvent::OnDrawPhase,
                                               m_state.get(), player)) {
        player->character()->triggerSkill(m_state.get(), player);
    }

    emitLog(player->displayName() + QStringLiteral(" 摸了 ") +
            QString::number(static_cast<int>(cards.size())) +
            QStringLiteral(" 张牌"));
}

void GameViewModel::executePhaseEnd()
{
    Player* player = currentPlayer();
    if (!player) return;
    emitLog(player->displayName() + QStringLiteral(" 结束回合"));
    switchToNextPlayer();
}

// ==================== 辅助 ====================

void GameViewModel::setNextPhase(PhaseType phase)
{
    m_state->setCurrentPhase(phase);
    emitLog(QStringLiteral("→ ") + phaseName(phase));
    emit stateChanged();
}

void GameViewModel::switchToNextPlayer()
{
    if (m_state->isGameOver()) return;

    int count = m_state->playerCount();
    int current = m_state->currentPlayerIndex();
    for (int i = 1; i <= count; ++i) {
        int nextIdx = (current + i) % count;
        Player* next = m_state->player(nextIdx);
        if (next && next->isAlive()) {
            m_state->setCurrentPlayerIndex(nextIdx);
            m_state->incrementTurn();
            setNextPhase(PhaseType::Prepare);
            emitLog(QStringLiteral("--- 轮到 ") + next->displayName() +
                    QStringLiteral("（") + next->characterName() +
                    QStringLiteral("）的回合 ---"));
            return;
        }
    }
}

void GameViewModel::emitLog(const QString& msg)
{
    emit logMessage(msg);
}

// ==================== Model 事件回调 ====================

void GameViewModel::onModelPendingActionCreated(const PendingActionInfo& info)
{
    PendingActionVM vm;
    vm.sourceId      = info.source      ? info.source->playerId()      : -1;
    vm.targetId      = info.target      ? info.target->playerId()      : -1;
    vm.sourceCardId  = info.sourceCard  ? info.sourceCard->id()        : -1;
    vm.requiredCardType = info.requiredCardType;
    vm.description   = QString::fromStdString(info.description);
    vm.canSkip       = info.canSkip;
    for (Player* p : info.remainingTargets) {
        if (p) vm.remainingTargetIds.push_back(p->playerId());
    }

    emit pendingActionCreated(vm);
    emitLog(QString::fromStdString(info.description));
    emit stateChanged();
}

void GameViewModel::onModelPendingActionCleared()
{
    emit pendingActionCleared();
    emit stateChanged();
}

void GameViewModel::onModelGameOver(int winnerPlayerId)
{
    Player* winner = (winnerPlayerId >= 0) ? playerByIndex(winnerPlayerId) : nullptr;
    if (winner) {
        emitLog(QStringLiteral("游戏结束！") + winner->displayName() +
                QStringLiteral("（") + winner->characterName() +
                QStringLiteral("）获胜！"));
    } else {
        emitLog(QStringLiteral("游戏结束！平局！"));
    }
    emit gameOver(winner ? winner->playerId() : -1);
}

void GameViewModel::onModelPlayerDied(int playerId)
{
    Player* player = playerByIndex(playerId);
    if (!player) return;
    emitLog(player->displayName() + QStringLiteral("（") +
            player->characterName() + QStringLiteral("）阵亡！"));
}

// ==================== 内部 Model 访问 ====================

Player* GameViewModel::currentPlayer() const { return m_state->currentPlayer(); }

Player* GameViewModel::opponentPlayer() const
{
    Player* cur = currentPlayer();
    if (!cur) return nullptr;
    for (Player* p : m_state->allPlayers()) {
        if (p && p != cur && p->isAlive()) return p;
    }
    return nullptr;
}

Player* GameViewModel::playerByIndex(int index) const { return m_state->player(index); }

Card* GameViewModel::findCard(int cardId) const
{
    if (!m_state) return nullptr;
    for (Player* p : m_state->allPlayers()) {
        if (!p) continue;
        for (Card* c : p->handCards()) {
            if (c && c->id() == cardId) return c;
        }
    }
    return nullptr;
}
