#include "GameViewModel.h"
#include "ActionViewModel.h"
#include "GameRule.h"
#include "CardManager.h"
#include "Player.h"
#include "Card.h"
#include "Character.h"

namespace {

Player* pendingResponder(const PendingActionInfo& info)
{
    return info.requiredCardType == CardType::Peach ? info.source : info.target;
}

} // namespace

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

bool GameViewModel::startGame(int characterId1, int characterId2)
{
    Character* char1 = createCharacterById(characterId1);
    Character* char2 = createCharacterById(characterId2);
    if (!char1 || !char2) {
        delete char1;
        delete char2;
        return false;
    }
    initGame(char1, char2);
    return true;
}

void GameViewModel::advancePhase()
{
    if (m_state->isGameOver()) return;
    switch (m_state->currentPhase()) {
    case PhaseType::Prepare: executePhasePrepare(); setNextPhase(PhaseType::Judge); break;
    case PhaseType::Judge:   executePhaseJudge();   setNextPhase(PhaseType::Draw); break;
    case PhaseType::Draw:    executePhaseDraw();    setNextPhase(PhaseType::Play); break;
    case PhaseType::Play:
        // 与 endPlayPhase() 保持一致：待定动作（如杀等待闪）尚未结算时
        // 不能离开 Play 阶段，否则 pendingAction 会悬空、后续响应/跳过会
        // 针对一个已经"过期"的待定动作生效。本地模式下 AutoAdvanceTimer
        // 从不在 Play 阶段启动（见 GameBoardWidget::onPhaseChanged），这条
        // 分支从未被触达；网络模式下当前回合玩家可以在出牌后立刻发
        // AdvanceRequested，必须在这里补上同样的保护。
        if (m_state->hasPendingAction()) return;
        setNextPhase(PhaseType::Discard);
        break;
    case PhaseType::Discard: setNextPhase(PhaseType::End); break;
    case PhaseType::End:     executePhaseEnd(); break;
    }
}

void GameViewModel::endPlayPhase()
{
    if (m_state->isGameOver()) return;
    if (m_state->currentPhase() != PhaseType::Play) return;
    if (m_state->hasPendingAction()) return;
    setNextPhase(PhaseType::Discard);
}

ActionViewModel* GameViewModel::actionVM() const { return m_actionVM.get(); }
GameState* GameViewModel::gameState() const { return m_state.get(); }

// ==================== 初始化 ====================

void GameViewModel::initGame(Character* char1, Character* char2)
{
    m_cardManager->initialize();

    auto* p1 = new Player(m_state.get());
    p1->setPlayerId(0); p1->setDisplayName(QStringLiteral("玩家1"));
    p1->setCharacter(char1);
    auto* p2 = new Player(m_state.get());
    p2->setPlayerId(1); p2->setDisplayName(QStringLiteral("玩家2"));
    p2->setCharacter(char2);
    m_state->addPlayer(p1);
    m_state->addPlayer(p2);

    for (int i = 0; i < GameRule::INITIAL_HAND_COUNT; ++i) {
        Card* c1 = m_cardManager->drawCard();
        Card* c2 = m_cardManager->drawCard();
        if (c1) p1->addHandCard(c1);
        if (c2) p2->addHandCard(c2);
    }

    // 监听 Model 信号
    m_modelConnections.push_back(
        connect(m_state.get(), &GameState::phaseChanged, this, &GameViewModel::phaseChanged));
    m_modelConnections.push_back(
        connect(m_state.get(), &GameState::currentPlayerChanged, this, &GameViewModel::currentPlayerChanged));
    m_modelConnections.push_back(
        connect(m_state.get(), &GameState::pendingActionCreated, this, &GameViewModel::onModelPendingActionCreated));
    m_modelConnections.push_back(
        connect(m_state.get(), &GameState::pendingActionCleared, this, &GameViewModel::onModelPendingActionCleared));
    m_modelConnections.push_back(
        connect(m_state.get(), &GameState::gameOver, this, &GameViewModel::onModelGameOver));
    m_modelConnections.push_back(
        connect(m_state.get(), &GameState::stateRefreshed, this, &GameViewModel::pushAllData));

    // 监听 Player 信号 → 推送数据
    for (Player* p : m_state->allPlayers()) {
        if (!p) continue;
        m_modelConnections.push_back(
            connect(p, &Player::handCardsChanged, this, [this, p]() { pushHandCards(p->playerId()); }));
        m_modelConnections.push_back(
            connect(p, &Player::stateChanged, this, [this, p]() { pushPlayerData(p->playerId()); }));
        m_modelConnections.push_back(
            connect(p, &Player::died, this, &GameViewModel::onModelPlayerDied));
    }

    m_state->setCurrentPhase(PhaseType::Prepare);
    emitLog(QStringLiteral("游戏开始！") + p1->displayName()
            + QStringLiteral("（") + p1->characterName() + QStringLiteral("）vs ")
            + p2->displayName() + QStringLiteral("（") + p2->characterName() + QStringLiteral("）"));

    // 强制发射初始阶段信号（GameState 默认已是 Prepare，setCurrentPhase 不会触发）
    emit phaseChanged(PhaseType::Prepare);
    pushAllData();
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

bool GameViewModel::isValidCharacterId(int id)
{
    return id >= 0 && id <= 3;
}

// ==================== 阶段执行 ====================

void GameViewModel::executePhasePrepare()
{
    Player* player = currentPlayer();
    if (!player) return;
    player->resetTurnState();
    if (player->character() &&
        player->character()->triggerCondition(GameEvent::OnTurnStart, m_state.get(), player)) {
        player->character()->triggerSkill(m_state.get(), player);
    }
}

void GameViewModel::executePhaseJudge() {}

void GameViewModel::executePhaseDraw()
{
    Player* player = currentPlayer();
    if (!player) return;
    auto cards = m_cardManager->drawCards(GameRule::DRAW_PHASE_COUNT);
    for (Card* c : cards) if (c) player->addHandCard(c);
    if (player->character() &&
        player->character()->triggerCondition(GameEvent::OnDrawPhase, m_state.get(), player)) {
        player->character()->triggerSkill(m_state.get(), player);
    }
    emitLog(player->displayName() + QStringLiteral(" 摸了 ")
            + QString::number(static_cast<int>(cards.size())) + QStringLiteral(" 张牌"));
}

void GameViewModel::executePhaseEnd()
{
    Player* player = currentPlayer();
    if (!player) return;
    emitLog(player->displayName() + QStringLiteral(" 结束回合"));
    switchToNextPlayer();
}

void GameViewModel::setNextPhase(PhaseType phase)
{
    // 自动跳过 Discard 阶段（当前玩家无需弃牌）
    if (phase == PhaseType::Discard) {
        Player* cur = currentPlayer();
        if (cur && GameRule::getDiscardCount(cur) <= 0) {
            setNextPhase(PhaseType::End);
            return;
        }
    }

    m_state->setCurrentPhase(phase);
    emitLog(QStringLiteral("→ ") + GameViewModel::phaseName(phase));
    emit stateChanged();
    pushAllData();
}

void GameViewModel::switchToNextPlayer()
{
    if (m_state->isGameOver()) return;
    int count = m_state->playerCount();
    int cur = m_state->currentPlayerIndex();
    for (int i = 1; i <= count; ++i) {
        int next = (cur + i) % count;
        Player* p = m_state->player(next);
        if (p && p->isAlive()) {
            m_state->setCurrentPlayerIndex(next);
            m_state->incrementTurn();
            setNextPhase(PhaseType::Prepare);
            emitLog(QStringLiteral("--- 轮到 ") + p->displayName()
                    + QStringLiteral("（") + p->characterName()
                    + QStringLiteral("）的回合 ---"));
            return;
        }
    }
}

void GameViewModel::emitLog(const QString& msg) { emit logMessage(msg); }

// ==================== 数据推送（View 消费） ====================

void GameViewModel::pushPlayerData(int playerId)
{
    Player* p = playerByIndex(playerId);
    if (!p) return;
    PlayerData d;
    d.playerId = p->playerId();
    d.displayName = p->displayName();
    d.characterName = p->characterName();
    d.hp = p->hp();
    d.maxHp = p->maxHp();
    d.isAlive = p->isAlive();
    d.isDying = p->isDying();
    d.handCardCount = p->handCardCount();
    d.handCardLimit = p->handCardLimit();
    d.isCurrentPlayer = (p->playerId() == currentPlayerId());
    if (p->character()) {
        d.skillName = QString::fromStdString(p->character()->skillName());
        d.skillDescription = QString::fromStdString(p->character()->skillDescription());
    }
    emit playerDataUpdated(playerId, d);
}

void GameViewModel::pushHandCards(int playerId)
{
    Player* p = playerByIndex(playerId);
    if (!p) return;
    Player* cur = currentPlayer();
    std::vector<int> playableIds;
    if (cur && cur->playerId() == playerId && m_state->currentPhase() == PhaseType::Play) {
        playableIds = m_actionVM->getPlayableCardIds(playerId);
    }

    QVector<CardData> cards;
    for (Card* card : p->handCards()) {
        if (!card) continue;
        CardData d;
        d.cardId = card->id();
        d.cardType = card->cardType();
        d.suit = card->suit();
        d.number = card->number();
        d.cardName = QString::fromStdString(card->cardName());
        d.description = QString::fromStdString(card->description());
        d.color = card->color();
        d.isBasic = card->isBasic();
        d.isStrategy = card->isStrategy();
        d.suitSymbol = QString::fromStdString(card->suitSymbol());
        d.numberString = QString::fromStdString(card->numberString());
        d.isPlayable = (std::find(playableIds.begin(), playableIds.end(), card->id()) != playableIds.end());
        d.ownerId = playerId;
        cards.append(d);
    }
    emit handCardsUpdated(playerId, cards);
}

void GameViewModel::pushAllData()
{
    for (int i = 0; i < m_state->playerCount(); ++i) {
        pushPlayerData(i);
        pushHandCards(i);
    }
}

// ==================== Model 事件回调 ====================

void GameViewModel::onModelPendingActionCreated(const PendingActionInfo& info)
{
    PendingActionData vm;
    vm.sourceId = info.source ? info.source->playerId() : -1;
    vm.targetId = info.target ? info.target->playerId() : -1;
    vm.sourceCardId = info.sourceCard ? info.sourceCard->id() : -1;
    vm.requiredCardType = info.requiredCardType;
    vm.description = QString::fromStdString(info.description);
    vm.canSkip = info.canSkip;
    for (Player* p : info.remainingTargets)
        if (p) vm.remainingTargetIds.push_back(p->playerId());

    // 普通响应由 target 负责，濒死救援由 source（当前救援者）负责。
    Player* responder = pendingResponder(info);
    int responderId = responder ? responder->playerId() : -1;

    // 自动跳过：当前响应者没有可用响应牌
    auto responseCards = m_actionVM->getResponseCardIds(responderId, vm.requiredCardType);
    if (responseCards.empty()) {
        m_actionVM->skipResponse(responderId, true);
        return;
    }

    emit pendingActionCreated(vm);
    emitLog(QString::fromStdString(info.description));
    emit stateChanged();
    pushAllData();
}

void GameViewModel::onModelPendingActionCleared()
{
    emit pendingActionCleared();
    emit stateChanged();
    pushAllData();
}

void GameViewModel::onModelGameOver(int winnerPlayerId)
{
    Player* w = (winnerPlayerId >= 0) ? playerByIndex(winnerPlayerId) : nullptr;
    if (w) emitLog(QStringLiteral("游戏结束！") + w->displayName() + QStringLiteral("（") + w->characterName() + QStringLiteral("）获胜！"));
    else emitLog(QStringLiteral("游戏结束！平局！"));
    emit gameOver(w ? w->playerId() : -1);
}

void GameViewModel::onModelPlayerDied(int playerId)
{
    Player* p = playerByIndex(playerId);
    if (!p) return;
    emitLog(p->displayName() + QStringLiteral("（") + p->characterName() + QStringLiteral("）阵亡！"));
}

// ==================== View 命令槽（从 SGSApp 迁入） ====================

void GameViewModel::onDiscardCardRequested(int cardId, int playerId)
{
    m_actionVM->discardCard(cardId, playerId);
    // 只有"当前回合玩家"的弃牌才能推进阶段——本地模式下 playerId 恒等于
    // currentPlayerId()（View 只在该玩家的弃牌阶段允许操作其手牌区），
    // 但网络模式下任意已连接玩家都可能发来这条命令；若不加这层校验，
    // 对方随手发一条自己的（无效）弃牌请求就会让 getDiscardCount(playerId)
    // 恰好 <= 0，从而提前结束当前玩家本该进行的弃牌阶段。
    if (playerId == currentPlayerId() && m_actionVM->getDiscardCount(playerId) <= 0)
        advancePhase();
}

void GameViewModel::onEndPlayRequested() { endPlayPhase(); }

void GameViewModel::onAdvanceRequested() { advancePhase(); }

void GameViewModel::onSkipRequested()
{
    if (!m_state || !m_state->hasPendingAction()) return;
    Player* responder = pendingResponder(m_state->pendingActionInfo());
    if (!responder) return;
    int responderId = responder->playerId();
    m_actionVM->skipResponse(responderId, true);
}

// ==================== 内部 ====================

int GameViewModel::currentPlayerId() const { auto* p = currentPlayer(); return p ? p->playerId() : -1; }

int GameViewModel::pendingResponderId() const
{
    if (!m_state || !m_state->hasPendingAction()) return -1;
    Player* responder = pendingResponder(m_state->pendingActionInfo());
    return responder ? responder->playerId() : -1;
}

Player* GameViewModel::currentPlayer() const { return m_state->currentPlayer(); }
Player* GameViewModel::opponentPlayer() const { auto* c = currentPlayer(); if (!c) return nullptr; for (auto* p : m_state->allPlayers()) { if (p && p != c && p->isAlive()) return p; } return nullptr; }
Player* GameViewModel::playerByIndex(int idx) const { return m_state->player(idx); }
Card* GameViewModel::findCard(int) const { return nullptr; }

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
    return {};
}
