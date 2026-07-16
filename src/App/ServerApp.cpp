#include "ServerApp.h"
#include "GameViewModel.h"
#include "ActionViewModel.h"
#include "GameServer.h"
#include "MessageSerializer.h"

using namespace Protocol;

ServerApp::ServerApp(QObject* parent)
    : QObject(parent)
    , m_server(new GameServer(this))
{
    connect(m_server, &GameServer::bothPlayersReady,
            this, &ServerApp::onBothPlayersReady);
    connect(m_server, &GameServer::clientCommandReceived,
            this, &ServerApp::onClientCommandReceived);
}

bool ServerApp::listen(quint16 port, bool localOnly)
{
    return m_server->listen(port, localOnly);
}

void ServerApp::startHeadlessGame(int charId1, int charId2)
{
    // 与 SGSApp::startLocalGame 一致：重复开局先释放上一局对象图
    if (m_gvm) {
        m_gvm->deleteLater();
        m_gvm = nullptr;
    }

    m_gvm = new GameViewModel(this);

    connect(m_gvm, &GameViewModel::gameOver,
            this, &ServerApp::onGameOver);

    // 必须在 startGame() 之前完成广播接线：startGame 内部会同步 emit
    // 初始的 phaseChanged/handCardsUpdated/playerDataUpdated，若接线滞后
    // 这些信号会在无人监听的情况下被直接丢弃，客户端永远收不到开局状态。
    wireViewModelBroadcasts();

    if (!m_gvm->startGame(charId1, charId2)) {
        // characterId 非法（客户端可任意填 SelectCharacterMsg::characterId，
        // 服务器未做范围校验）：回滚，isGameRunning() 恢复 false，
        // 避免留下一个"m_gvm 非空但从未真正 initGame"的半开局僵尸状态。
        m_gvm->deleteLater();
        m_gvm = nullptr;
    }
}

ActionViewModel* ServerApp::actionViewModel() const
{
    return m_gvm ? m_gvm->actionVM() : nullptr;
}

void ServerApp::onGameOver(int winnerId)
{
    emit gameFinished(winnerId);
}

void ServerApp::onBothPlayersReady(int charId0, int charId1)
{
    // 防重放：GameServer::ClientSlot::selectedCharacterId 在对局开始后
    // 不会被清空，若已连接客户端在对局进行中重发一条 SelectCharacter
    // （无论是否伪造身份、无论内容是否和之前相同），bothPlayersReady 会
    // 被再次 emit。若不在这里挡住，startHeadlessGame 会无条件丢弃当前
    // 正在进行的整局（手牌/血量/回合进度）重新开局。
    if (isGameRunning()) return;

    // SelectCharacterMsg::characterId 是客户端可任意填的字段：非法 id 若等到
    // startHeadlessGame 内部才失败，GameStarted 已经广播出去了——客户端会
    // 误以为对局已开始，之后却永远收不到任何 PhaseChanged/HandCardsUpdated。
    // 在广播前先校验，非法 id 直接不吭声地拒绝（协议暂无专门的拒绝消息类型，
    // 客户端只能表现为"迟迟收不到 GameStarted"，留给后续协议版本补显式反馈）。
    if (!GameViewModel::isValidCharacterId(charId0) ||
        !GameViewModel::isValidCharacterId(charId1)) {
        return;
    }

    // 先广播 GameStarted，再开局——保证客户端收到的消息顺序是
    // GameStarted → PhaseChanged → HandCardsUpdated → ...
    // （startHeadlessGame 内部同步 emit 初始状态，晚广播 GameStarted
    // 会导致它排在这些状态更新之后，客户端收到顺序就反了）
    GameStartedMsg msg;
    msg.characterId0 = charId0;
    msg.characterId1 = charId1;
    m_server->broadcast(MessageType::GameStarted,
                        MessageSerializer::encodePayload(msg));

    startHeadlessGame(charId0, charId1);
}

void ServerApp::wireViewModelBroadcasts()
{
    auto* avm = m_gvm->actionVM();

    connect(m_gvm, &GameViewModel::phaseChanged, this, [this](PhaseType phase) {
        PhaseChangedMsg msg;
        msg.phase = phase;
        m_server->broadcast(MessageType::PhaseChanged,
                            MessageSerializer::encodePayload(msg));
    });
    connect(m_gvm, &GameViewModel::playerDataUpdated, this,
            [this](int playerId, const PlayerData& data) {
        PlayerDataMsg msg;
        msg.playerId = playerId;
        msg.data = data;
        m_server->broadcast(MessageType::PlayerDataUpdated,
                            MessageSerializer::encodePayload(msg));
    });
    connect(m_gvm, &GameViewModel::handCardsUpdated, this,
            [this](int playerId, const CardList& cards) {
        // 按接收方裁剪：所有者本人收完整牌面，对手只收 cardId/ownerId，
        // 牌面字段（cardType/cardName/description 等）已被 redactCardList 置为占位值。
        // 两条广播的消息体都保留 msg.playerId = 所有者 id（谁的手牌，而非收件人），
        // 不能用 GameServer::broadcast 统一广播同一份 payload。
        HandCardsMsg fullMsg;
        fullMsg.playerId = playerId;
        fullMsg.cards = cards;
        m_server->sendTo(playerId, MessageType::HandCardsUpdated,
                         MessageSerializer::encodePayload(fullMsg));

        HandCardsMsg redactedMsg;
        redactedMsg.playerId = playerId;
        redactedMsg.cards = redactCardList(cards);
        const int opponentId = 1 - playerId;
        m_server->sendTo(opponentId, MessageType::HandCardsUpdated,
                         MessageSerializer::encodePayload(redactedMsg));
    });
    connect(m_gvm, &GameViewModel::pendingActionCreated, this,
            [this](const PendingActionData& info) {
        PendingActionMsg msg;
        msg.info = info;
        m_server->broadcast(MessageType::PendingActionCreated,
                            MessageSerializer::encodePayload(msg));
    });
    connect(m_gvm, &GameViewModel::pendingActionCleared, this, [this]() {
        m_server->broadcast(MessageType::PendingActionCleared);
    });
    connect(m_gvm, &GameViewModel::gameOver, this, [this](int winnerId) {
        GameOverMsg msg;
        msg.winnerId = winnerId;
        m_server->broadcast(MessageType::GameOver,
                            MessageSerializer::encodePayload(msg));
    });
    connect(m_gvm, &GameViewModel::logMessage, this, [this](const QString& text) {
        LogMessageMsg msg;
        msg.message = text;
        m_server->broadcast(MessageType::LogMessage,
                            MessageSerializer::encodePayload(msg));
    });
    connect(m_gvm, &GameViewModel::judgmentPerformed, this,
            [this](const CardData& judgeCard, const QString& resultText, bool effective) {
        JudgmentPerformedMsg msg;
        msg.judgeCard = judgeCard;
        msg.resultText = resultText;
        msg.effective = effective;
        m_server->broadcast(MessageType::JudgmentPerformed,
                            MessageSerializer::encodePayload(msg));
    });
    connect(avm, &ActionViewModel::targetSelectionStarted, this,
            [this](const QVector<int>& ids) {
        TargetSelectionStartedMsg msg;
        msg.targetIds = ids;
        m_server->broadcast(MessageType::TargetSelectionStarted,
                            MessageSerializer::encodePayload(msg));
    });
    connect(avm, &ActionViewModel::targetSelectionFinished, this, [this]() {
        m_server->broadcast(MessageType::TargetSelectionFinished);
    });
}

void ServerApp::onClientCommandReceived(int playerId, MessageType type,
                                        const QByteArray& payload)
{
    if (!m_gvm)
        return;  // 游戏尚未开始（理论上不应发生：GameServer 只在握手后转发）
    auto* avm = m_gvm->actionVM();

    switch (type) {
    case MessageType::PlayCardRequested: {
        const auto msg = MessageSerializer::decodePayload<PlayCardMsg>(payload);
        avm->onPlayCardRequested(msg.cardId, playerId);
        break;
    }
    case MessageType::RespondCardRequested: {
        const auto msg = MessageSerializer::decodePayload<RespondCardMsg>(payload);
        avm->onRespondCardRequested(msg.cardId, playerId);
        break;
    }
    case MessageType::TargetSelected: {
        // 已知局限（留给 Step 9 / M4 处理）：ActionViewModel::onTargetSelected
        // 不接收 playerId，本身依赖内部 m_pendingUserId 单例状态而非按连接
        // 校验调用者身份。网络层目前无法在这里补上"只有目标选择发起者能
        // 调用"的校验，除非改 ActionViewModel 的槽签名（会影响本地 SGSApp）。
        const auto msg = MessageSerializer::decodePayload<TargetSelectedMsg>(payload);
        avm->onTargetSelected(msg.playerIndex);
        break;
    }
    case MessageType::DiscardCardRequested: {
        const auto msg = MessageSerializer::decodePayload<DiscardCardMsg>(payload);
        m_gvm->onDiscardCardRequested(msg.cardId, playerId);
        break;
    }
    case MessageType::SkillRequested: {
        const auto msg = MessageSerializer::decodePayload<SkillRequestedMsg>(payload);
        avm->onSkillRequested(msg.cardIds, playerId);
        break;
    }
    case MessageType::EndPlayRequested:
        // GameViewModel::onEndPlayRequested 本身不接受/校验 playerId
        // （本地模式下只会被当前回合玩家的 View 触发），网络模式下必须
        // 在这里补上"发起方必须是当前回合玩家"的校验，否则任意已连接
        // 客户端都能提前结束对方的出牌阶段。
        if (playerId == m_gvm->currentPlayerId())
            m_gvm->onEndPlayRequested();
        break;
    case MessageType::AdvanceRequested:
        // 同上——且 ClientApp（Step 7）实现后每个客户端会各自持有一份
        // GameBoardWidget 的 AutoAdvanceTimer，非当前玩家一侧的计时器
        // 触发的 AdvanceRequested 必须被这里挡住，否则会越权驱动阶段推进。
        if (playerId == m_gvm->currentPlayerId())
            m_gvm->onAdvanceRequested();
        break;
    case MessageType::SkipRequested:
        // GameViewModel::onSkipRequested 内部用 pendingResponder() 从游戏
        // 状态反推真正的响应者、无条件对其生效，不检查请求实际来自哪个
        // 连接——网络模式下必须校验发起方就是真正的响应者，否则任意一方
        // 都能替对方强制放弃响应（即使对方手里有牌可用）。
        if (playerId == m_gvm->pendingResponderId())
            m_gvm->onSkipRequested();
        break;
    default:
        break;  // 非法/不适用的命令类型：忽略，不崩溃
    }
}
