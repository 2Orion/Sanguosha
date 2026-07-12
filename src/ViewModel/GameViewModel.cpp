#include "GameViewModel.h"
#include "PlayerViewModel.h"
#include "ActionViewModel.h"
#include "CardViewModel.h"

#include "GameState.h"
#include "GameRule.h"
#include "CardManager.h"
#include "Player.h"
#include "Card.h"
#include "Character.h"

// ==================== 构造 / 析构 ====================

GameViewModel::GameViewModel()
{
    m_state = std::make_unique<GameState>();
    m_cardManager = std::make_unique<CardManager>();
    m_actionVM = std::make_unique<ActionViewModel>();

    m_state->setCardManager(m_cardManager.get());
    m_actionVM->setGameState(m_state.get());
}

GameViewModel::~GameViewModel()
{
    // 断开 Model 事件
    if (m_state) {
        m_state->phaseChanged.disconnect(m_modelConn.phaseChangedId);
        m_state->currentPlayerChanged.disconnect(m_modelConn.currentPlayerChangedId);
        m_state->pendingActionCreated.disconnect(m_modelConn.pendingActionCreatedId);
        m_state->pendingActionCleared.disconnect(m_modelConn.pendingActionClearedId);
        m_state->gameOver.disconnect(m_modelConn.gameOverId);
        m_state->stateRefreshed.disconnect(m_modelConn.stateRefreshedId);
    }
}

// ==================== 游戏生命周期 ====================

void GameViewModel::startGame(int characterId1, int characterId2)
{
    // 1. 创建武将
    Character* char1 = createCharacterById(characterId1);
    Character* char2 = createCharacterById(characterId2);
    if (!char1 || !char2) return;

    m_characters.push_back(std::unique_ptr<Character>(char1));
    m_characters.push_back(std::unique_ptr<Character>(char2));

    initGame(char1, char2);
}

void GameViewModel::advancePhase()
{
    if (m_state->isGameOver()) return;

    PhaseType current = m_state->currentPhase();

    switch (current) {
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
        // 出牌阶段由 View 驱动，advancePhase 表示"出牌结束"
        setNextPhase(PhaseType::Discard);
        break;

    case PhaseType::Discard:
        // 弃牌由 View 处理（View 应检查 getDiscardCount 并在弃够牌后调用此方法）
        setNextPhase(PhaseType::End);
        break;

    case PhaseType::End:
        executePhaseEnd();
        // executePhaseEnd 会切换到下家并回到 Prepare
        break;
    }
}

void GameViewModel::endPlayPhase()
{
    if (m_state->isGameOver()) return;
    if (m_state->currentPhase() != PhaseType::Play) return;

    // 确保没有待定动作
    if (m_state->hasPendingAction()) return;

    setNextPhase(PhaseType::Discard);
}

// ==================== 状态查询 ====================

GameState* GameViewModel::gameState() const
{
    return m_state.get();
}

Player* GameViewModel::currentPlayer() const
{
    return m_state->currentPlayer();
}

Player* GameViewModel::opponentPlayer() const
{
    Player* cur = currentPlayer();
    if (!cur) return nullptr;
    for (Player* p : m_state->allPlayers()) {
        if (p && p != cur && p->isAlive()) return p;
    }
    return nullptr;
}

PhaseType GameViewModel::currentPhase() const
{
    return m_state->currentPhase();
}

int GameViewModel::turnCount() const
{
    return m_state->turnCount();
}

bool GameViewModel::isGameOver() const
{
    return m_state->isGameOver();
}

Player* GameViewModel::winner() const
{
    return m_state->winner();
}

std::string GameViewModel::phaseName(PhaseType phase)
{
    switch (phase) {
    case PhaseType::Prepare: return "准备阶段";
    case PhaseType::Judge:   return "判定阶段";
    case PhaseType::Draw:    return "摸牌阶段";
    case PhaseType::Play:    return "出牌阶段";
    case PhaseType::Discard: return "弃牌阶段";
    case PhaseType::End:     return "结束阶段";
    }
    return "未知";
}

// ==================== 子 ViewModel ====================

PlayerViewModel* GameViewModel::playerVM(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_playerVMs.size())) {
        return m_playerVMs[index].get();
    }
    return nullptr;
}

ActionViewModel* GameViewModel::actionVM() const
{
    return m_actionVM.get();
}

// ==================== 手牌展示辅助 ====================

std::vector<std::unique_ptr<CardViewModel>> GameViewModel::getCurrentPlayerCardVMs() const
{
    return getPlayerCardVMs(currentPlayer());
}

std::vector<std::unique_ptr<CardViewModel>> GameViewModel::getPlayerCardVMs(Player* player) const
{
    std::vector<std::unique_ptr<CardViewModel>> result;
    if (!player || !m_state) return result;

    // 获取可打出的牌列表
    std::vector<Card*> playable = m_actionVM->getPlayableCards(player);

    for (Card* card : player->handCards()) {
        if (!card) continue;
        auto cvm = std::make_unique<CardViewModel>(card);

        // 标记可打出的牌
        bool isPlayable = false;
        for (Card* pc : playable) {
            if (pc == card) {
                isPlayable = true;
                break;
            }
        }
        cvm->setPlayable(isPlayable);

        result.push_back(std::move(cvm));
    }
    return result;
}

// ==================== 初始化 ====================

void GameViewModel::initGame(Character* char1, Character* char2)
{
    // 1. 初始化牌堆
    m_cardManager->initialize();

    // 2. 创建玩家
    auto* player1 = new Player();
    player1->setPlayerId(0);
    player1->setDisplayName("玩家1");
    player1->setCharacter(char1);

    auto* player2 = new Player();
    player2->setPlayerId(1);
    player2->setDisplayName("玩家2");
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

    // 4. 创建 PlayerViewModel（需在 Player 设置完成后）
    m_playerVMs.push_back(std::make_unique<PlayerViewModel>(player1));
    m_playerVMs.push_back(std::make_unique<PlayerViewModel>(player2));

    // 5. 监听 Model 事件
    m_modelConn.phaseChangedId = m_state->phaseChanged.connect(
        [this](PhaseType p) { this->phaseChanged.notify(p); });

    m_modelConn.currentPlayerChangedId = m_state->currentPlayerChanged.connect(
        [this](int idx) { this->currentPlayerChanged.notify(idx); });

    m_modelConn.pendingActionCreatedId = m_state->pendingActionCreated.connect(
        [this](const PendingActionInfo& info) { this->onPendingActionCreated(info); });

    m_modelConn.pendingActionClearedId = m_state->pendingActionCleared.connect(
        [this]() { this->onPendingActionCleared(); });

    m_modelConn.gameOverId = m_state->gameOver.connect(
        [this](Player* w) { this->onGameOver(w); });

    m_modelConn.stateRefreshedId = m_state->stateRefreshed.connect(
        [this]() { this->stateChanged.notify(); });

    // 6. 监听玩家死亡
    for (Player* p : m_state->allPlayers()) {
        if (!p) continue;
        auto connId = p->died.connect(
            [this](Player* dead) { this->onPlayerDied(dead); });
        m_playerDiedConnections.push_back(connId);
    }

    // 7. 开始第一个回合 — 进入准备阶段
    m_state->setCurrentPhase(PhaseType::Prepare);
    emitLog("游戏开始！" + player1->displayName() + "（" +
            player1->characterName() + "）vs " +
            player2->displayName() + "（" +
            player2->characterName() + "）");
}

Character* GameViewModel::createCharacterById(int id)
{
    switch (id) {
    case 0: return new CaoCao();
    case 1: return new GuanYu();
    case 2: return new ZhangFei();
    case 3: return new ZhaoYun();
    default: return nullptr;
    }
}

// ==================== 阶段执行 ====================

void GameViewModel::executePhasePrepare()
{
    Player* player = currentPlayer();
    if (!player) return;

    // 重置回合状态
    player->resetTurnState();

    // 触发回合开始技能
    if (player->character() &&
        player->character()->triggerCondition(GameEvent::OnTurnStart,
                                               m_state.get(), player)) {
        player->character()->triggerSkill(m_state.get(), player);
        emitLog(player->displayName() + " 触发了技能【" +
                player->character()->skillName() + "】");
    }
}

void GameViewModel::executePhaseJudge()
{
    // 简单版：没有延时锦囊，跳过
}

void GameViewModel::executePhaseDraw()
{
    Player* player = currentPlayer();
    if (!player) return;

    std::vector<Card*> cards = m_cardManager->drawCards(GameRule::DRAW_PHASE_COUNT);
    for (Card* card : cards) {
        if (card) {
            player->addHandCard(card);
        }
    }

    // 触发摸牌阶段技能
    if (player->character() &&
        player->character()->triggerCondition(GameEvent::OnDrawPhase,
                                               m_state.get(), player)) {
        player->character()->triggerSkill(m_state.get(), player);
    }

    emitLog(player->displayName() + " 摸了 " +
            std::to_string(cards.size()) + " 张牌");
}

void GameViewModel::executePhaseEnd()
{
    Player* player = currentPlayer();
    if (!player) return;

    emitLog(player->displayName() + " 结束回合");

    // 切换玩家
    switchToNextPlayer();
}

// ==================== 辅助 ====================

void GameViewModel::setNextPhase(PhaseType phase)
{
    m_state->setCurrentPhase(phase);
    emitLog("→ " + phaseName(phase));
    stateChanged.notify();
}

void GameViewModel::switchToNextPlayer()
{
    if (m_state->isGameOver()) return;

    // 找到下一个存活玩家
    int count = m_state->playerCount();
    int current = m_state->currentPlayerIndex();
    for (int i = 1; i <= count; ++i) {
        int nextIdx = (current + i) % count;
        Player* next = m_state->player(nextIdx);
        if (next && next->isAlive()) {
            m_state->setCurrentPlayerIndex(nextIdx);
            m_state->incrementTurn();
            setNextPhase(PhaseType::Prepare);
            emitLog("--- 轮到 " + next->displayName() + "（" +
                    next->characterName() + "）的回合 ---");
            return;
        }
    }
    // 理论上不会到这里（checkGameOver 会在此之前触发）
}

void GameViewModel::emitLog(const std::string& msg)
{
    logMessage.notify(msg);
}

// ==================== Model 事件回调 ====================

void GameViewModel::onPendingActionCreated(const PendingActionInfo& info)
{
    // 转发描述给 View 显示
    emitLog(info.description);
    stateChanged.notify();
}

void GameViewModel::onPendingActionCleared()
{
    stateChanged.notify();
}

void GameViewModel::onGameOver(Player* winner)
{
    if (winner) {
        emitLog("游戏结束！" + winner->displayName() + "（" +
                winner->characterName() + "）获胜！");
    } else {
        emitLog("游戏结束！平局！");
    }
    gameOver.notify(winner);
}

void GameViewModel::onPlayerDied(Player* player)
{
    if (!player) return;
    emitLog(player->displayName() + "（" + player->characterName() + "）阵亡！");
}
