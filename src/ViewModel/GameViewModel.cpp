#include "GameViewModel.h"
#include "ActionViewModel.h"
#include "GameRule.h"
#include "CardManager.h"
#include "Player.h"
#include "Card.h"
#include "Character.h"
#include <algorithm>

namespace {

Player* pendingResponder(const PendingActionInfo& info)
{
    if (info.isSkillChoice) return info.target;
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

    // 判定展示定时器（1.5s 后自动推进）
    m_judgeTimer = new QTimer(this);
    m_judgeTimer->setSingleShot(true);
    connect(m_judgeTimer, &QTimer::timeout, this, &GameViewModel::onJudgeTimerFired);
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
    // 判定展示期间禁止外部推进，避免与 m_judgeTimer 竞态把阶段打乱
    if (m_judgePending) return;
    switch (m_state->currentPhase()) {
    case PhaseType::Prepare: executePhasePrepare(); setNextPhase(PhaseType::Judge); break;
    case PhaseType::Judge:
        executePhaseJudge();
        // 如有判定进行中，由判定定时器控制后续推进
        if (!m_judgePending) setNextPhase(PhaseType::Draw);
        break;
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
        m_modelConnections.push_back(
            connect(p, &Player::equipmentChanged, this, [this, p](EquipSlot) { pushPlayerData(p->playerId()); }));
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
    case 4: return new SunQuan(this);
    case 5: return new ZhouYu(this);
    case 6: return new LvBu(this);
    case 7: return new DaQiao(this);
    case 8: return new SiMaYi(this);
    default: return nullptr;
    }
}

bool GameViewModel::isValidCharacterId(int id)
{
    return id >= 0 && id <= 8;
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

void GameViewModel::executePhaseJudge()
{
    Player* player = currentPlayer();
    if (!player) return;

    const auto& cards = player->judgmentCards();
    if (cards.empty()) return; // 无判定牌，正常推进

    // 判定区按后置入先判定处理。
    m_judgeTargetPlayerId = player->playerId();
    Card* judgeCard = cards.back();
    if (!judgeCard) return;

    // 从牌堆顶摸判定牌
    Card* resultCard = m_cardManager->drawCard();
    if (!resultCard) return;

    // 构建判定结果数据
    CardData resultData;
    resultData.cardId = resultCard->id();
    resultData.cardType = resultCard->cardType();
    resultData.suit = resultCard->suit();
    resultData.number = resultCard->number();
    resultData.cardName = QString::fromStdString(resultCard->cardName());
    resultData.suitSymbol = QString::fromStdString(resultCard->suitSymbol());
    resultData.numberString = QString::fromStdString(resultCard->numberString());
    resultData.color = resultCard->color();

    // 判定牌进弃牌堆
    m_cardManager->discard(resultCard);

    // 根据判定牌类型处理效果
    bool effective = false;
    QString resultText;
    CardType judgeType = judgeCard->cardType();

    switch (judgeType) {
    case CardType::Happy: // 乐不思蜀：非红桃则跳过出牌阶段
        if (resultCard->suit() == CardSuit::Heart) {
            resultText = player->displayName() + QStringLiteral("【乐不思蜀】判定: ♥，判定通过");
            effective = false;
        } else {
            resultText = player->displayName() + QStringLiteral("【乐不思蜀】判定: 非♥，跳过出牌阶段");
            effective = true;
        }
        break;

    case CardType::Famine: // 兵粮寸断：非梅花则跳过摸牌阶段
        if (resultCard->suit() == CardSuit::Club) {
            resultText = player->displayName() + QStringLiteral("【兵粮寸断】判定: ♣，判定通过");
            effective = false;
        } else {
            resultText = player->displayName() + QStringLiteral("【兵粮寸断】判定: 非♣，跳过摸牌阶段");
            effective = true;
        }
        break;

    case CardType::Lightning: // 闪电：黑桃2-9则3点伤害
        if (resultCard->suit() == CardSuit::Spade && resultCard->number() >= 2 && resultCard->number() <= 9) {
            resultText = player->displayName() + QStringLiteral("【闪电】判定: ♠")
                         + QString::number(resultCard->number()) + QStringLiteral("，受到3点雷电伤害！");
            effective = true;
        } else {
            resultText = player->displayName() + QStringLiteral("【闪电】判定: 不中，移至下家");
            effective = false;
        }
        break;

    default:
        resultText = player->displayName() + QStringLiteral(" 判定完成");
        break;
    }

    // 移除判定牌，并按结算结果放入弃牌堆或移动到下家。
    player->removeJudgmentCard(judgeCard);

    bool judgmentCardMoved = false;
    if (judgeType == CardType::Lightning && !effective) {
        int count = m_state->playerCount();
        int curIdx = player->playerId();
        for (int i = 1; i <= count; ++i) {
            int nextIdx = (curIdx + i) % count;
            Player* next = m_state->player(nextIdx);
            if (next && next->isAlive() &&
                !next->hasJudgmentCard(CardType::Lightning)) {
                next->addJudgmentCard(judgeCard);
                judgmentCardMoved = true;
                emitLog(QStringLiteral("【闪电】移至 ") + next->displayName());
                break;
            }
        }
    }

    if (!judgmentCardMoved && m_cardManager) {
        m_cardManager->discard(judgeCard);
    }

    if (judgeType == CardType::Lightning && effective) {
        GameRule::dealDamage(m_state.get(), player, 3, nullptr, judgeCard);
    }

    // 发射判定信号（onJudgmentPerformed 会带判定色入队展示，无需再 emitLog 重复）
    emit judgmentPerformed(resultData, resultText, effective);

    // 设置阶段跳过标记（由 setNextPhase 检查）
    m_skipDrawPhase = m_skipDrawPhase || (judgeType == CardType::Famine && effective);
    m_skipPlayPhase = m_skipPlayPhase || (judgeType == CardType::Happy && effective);

    pushAllData();

    // 启动1.5s定时器展示判定结果，之后推进阶段
    m_judgePending = true;
    m_judgeTimerRetries = 0;
    m_judgeTimer->start(1500);
}

Card* GameViewModel::drawJudgeCard()
{
    return m_cardManager ? m_cardManager->drawCard() : nullptr;
}

void GameViewModel::onJudgeTimerFired()
{
    // 闪电伤害可能开启濒死响应；先等该响应链结束，再继续判定阶段。
    if (m_state && m_state->hasPendingAction()) {
        if (++m_judgeTimerRetries <= JUDGE_TIMER_MAX_RETRIES) {
            m_judgeTimer->start(100);
            return;
        }
        // 兜底：pending 异常残留超过上限，强制清除后继续推进，避免判定阶段卡死。
        emitLog(QStringLiteral("[诊断] 判定后待定动作长时间未清除，已强制清除"));
        m_state->clearPendingAction();
    }

    m_judgePending = false;
    m_judgeTimerRetries = 0;
    m_judgeTargetPlayerId = -1;

    // 判定展示结束，推进阶段
    if (m_state && !m_state->isGameOver()) {
        advancePhase();
    }
}

void GameViewModel::executePhaseDraw()
{
    Player* player = currentPlayer();
    if (!player) return;

    int drawCount = GameRule::DRAW_PHASE_COUNT;
    // 周瑜英姿：多摸一张
    if (player->character()) {
        drawCount += player->character()->onDrawPhaseBonus();
    }

    auto cards = m_cardManager->drawCards(drawCount);
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
    // 清除本回合的判定跳过标记
    m_skipDrawPhase = false;
    m_skipPlayPhase = false;

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

    // 兵粮寸断生效：跳过摸牌阶段
    if (phase == PhaseType::Draw && m_skipDrawPhase) {
        m_skipDrawPhase = false;
        emitLog(currentPlayer() ? currentPlayer()->displayName() + QStringLiteral(" 被【兵粮寸断】，跳过摸牌阶段") : QString());
        setNextPhase(PhaseType::Play);
        return;
    }

    // 乐不思蜀生效：跳过出牌阶段
    if (phase == PhaseType::Play && m_skipPlayPhase) {
        m_skipPlayPhase = false;
        emitLog(currentPlayer() ? currentPlayer()->displayName() + QStringLiteral(" 被【乐不思蜀】，跳过出牌阶段") : QString());
        setNextPhase(PhaseType::Discard);
        return;
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
    d.canUseActiveSkill = m_actionVM->canUseActiveSkill(playerId);
    d.attackRange = p->attackRange();
    if (p->character()) {
        d.skillName = QString::fromStdString(p->character()->skillName());
        d.skillDescription = QString::fromStdString(p->character()->skillDescription());
    }

    // 填充装备区数据
    d.equipCards.clear();
    for (int i = 0; i < 4; ++i) {
        EquipSlot slot = static_cast<EquipSlot>(i);
        EquipmentCard* eq = p->equippedAt(slot);
        if (eq) {
            CardData cd;
            cd.cardId = eq->id();
            cd.cardType = eq->cardType();
            cd.suit = eq->suit();
            cd.number = eq->number();
            cd.cardName = QString::fromStdString(eq->cardName());
            cd.description = QString::fromStdString(eq->description());
            cd.color = eq->color();
            cd.isBasic = eq->isBasic();
            cd.isStrategy = eq->isStrategy();
            cd.isEquipment = true;
            cd.suitSymbol = QString::fromStdString(eq->suitSymbol());
            cd.numberString = QString::fromStdString(eq->numberString());
            cd.isPlayable = false;
            cd.ownerId = playerId;
            cd.equipSlot = i;
            d.equipCards.append(cd);
        }
    }

    // 填充判定区数据
    d.judgmentCards.clear();
    for (Card* jc : p->judgmentCards()) {
        if (!jc) continue;
        CardData cd;
        cd.cardId = jc->id();
        cd.cardType = jc->cardType();
        cd.suit = jc->suit();
        cd.number = jc->number();
        cd.cardName = QString::fromStdString(jc->cardName());
        cd.description = QString::fromStdString(jc->description());
        cd.color = jc->color();
        cd.isBasic = jc->isBasic();
        cd.isStrategy = true; // 延时锦囊也是锦囊
        cd.suitSymbol = QString::fromStdString(jc->suitSymbol());
        cd.numberString = QString::fromStdString(jc->numberString());
        cd.isPlayable = false;
        cd.ownerId = playerId;
        d.judgmentCards.append(cd);
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
        d.isEquipment = card->isEquipment();
        d.suitSymbol = QString::fromStdString(card->suitSymbol());
        d.numberString = QString::fromStdString(card->numberString());
        d.isPlayable = (std::find(playableIds.begin(), playableIds.end(), card->id()) != playableIds.end());
        d.ownerId = playerId;
        // 装备牌填 equipSlot
        EquipmentCard* eq = dynamic_cast<EquipmentCard*>(card);
        d.equipSlot = eq ? static_cast<int>(eq->equipSlot()) : -1;
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
    vm.isSkillChoice = info.isSkillChoice;
    for (Player* p : info.remainingTargets)
        if (p) vm.remainingTargetIds.push_back(p->playerId());

    // 普通响应由 target 负责，濒死救援由 source（当前救援者）负责。
    Player* responder = pendingResponder(info);
    int responderId = responder ? responder->playerId() : -1;

    // 自动跳过：当前响应者没有可用响应牌
    auto responseCards = m_actionVM->getResponseCardIds(responderId, vm.requiredCardType);
    if (!info.isSkillChoice && responseCards.empty()) {
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
