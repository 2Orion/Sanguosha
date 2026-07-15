// 网络层 Qt Test
//   Step 1：序列化 round-trip + 帧解码（半包/粘包）
//   Step 2：ServerApp headless 启动路径（QTEST_GUILESS_MAIN 下运行，
//           进程无 QApplication，任何 QWidget 创建都会直接失败——
//           这本身就是"零 View 依赖"的验证）
//   Step 3：GameServer 连接管理与握手（同进程 loopback；注意不能用
//           waitForConnected 之类的阻塞等待，必须用 QTest::qWaitFor
//           驱动主事件循环，否则 QTcpServer 收不到连接）
#include <QtTest/QtTest>
#include <QTcpSocket>
#include "Protocol.h"
#include "MessageSerializer.h"
#include "GameServer.h"
#include "ServerApp.h"
#include "GameViewModel.h"
#include "ActionViewModel.h"
#include "GameState.h"
#include "Player.h"
#include "Card.h"

using namespace Protocol;
using MessageSerializer::Frame;

namespace {

CardData makeCardData()
{
    CardData d;
    d.cardId = 42;
    d.cardType = CardType::QinglongSaber;
    d.suit = CardSuit::Heart;
    d.number = 5;
    d.cardName = QStringLiteral("青龙偃月刀");
    d.description = QStringLiteral("攻击距离+2");
    d.color = CardColor::Red;
    d.isBasic = false;
    d.isStrategy = false;
    d.suitSymbol = QStringLiteral("♥");
    d.numberString = QStringLiteral("5");
    d.isSelected = true;
    d.isPlayable = true;
    d.isHighlighted = false;
    d.ownerId = 1;
    d.isEquipment = true;
    d.equipSlot = EquipSlot::Weapon;
    d.attackRange = 2;
    return d;
}

PlayerData makePlayerData()
{
    PlayerData d;
    d.playerId = 1;
    d.displayName = QStringLiteral("玩家2");
    d.characterName = QStringLiteral("关羽");
    d.skillName = QStringLiteral("武圣");
    d.skillDescription = QStringLiteral("红色牌可当杀使用");
    d.hp = 3;
    d.maxHp = 4;
    d.isAlive = true;
    d.isDying = false;
    d.handCardCount = 5;
    d.handCardLimit = 3;
    d.isCurrentPlayer = true;
    d.weaponCard = makeCardData();
    d.armorCard = CardData{};  // 空槽
    return d;
}

PendingActionData makePendingActionData()
{
    PendingActionData d;
    d.sourceId = 0;
    d.targetId = 1;
    d.sourceCardId = 7;
    d.requiredCardType = CardType::Dodge;
    d.description = QStringLiteral("请打出一张闪");
    d.canSkip = true;
    d.remainingTargetIds = {1, 0};
    return d;
}

void compareCardData(const CardData& a, const CardData& b)
{
    QCOMPARE(a.cardId, b.cardId);
    QCOMPARE(a.cardType, b.cardType);
    QCOMPARE(a.suit, b.suit);
    QCOMPARE(a.number, b.number);
    QCOMPARE(a.cardName, b.cardName);
    QCOMPARE(a.description, b.description);
    QCOMPARE(a.color, b.color);
    QCOMPARE(a.isBasic, b.isBasic);
    QCOMPARE(a.isStrategy, b.isStrategy);
    QCOMPARE(a.suitSymbol, b.suitSymbol);
    QCOMPARE(a.numberString, b.numberString);
    QCOMPARE(a.isSelected, b.isSelected);
    QCOMPARE(a.isPlayable, b.isPlayable);
    QCOMPARE(a.isHighlighted, b.isHighlighted);
    QCOMPARE(a.ownerId, b.ownerId);
    QCOMPARE(a.isEquipment, b.isEquipment);
    QCOMPARE(a.equipSlot, b.equipSlot);
    QCOMPARE(a.attackRange, b.attackRange);
}

/// 完整帧 encode → decodeFrames → decodePayload 的 round-trip
template <typename Msg>
Msg frameRoundTrip(MessageType type, const Msg& msg)
{
    QByteArray buffer = MessageSerializer::encode(type, msg);
    QVector<Frame> frames;
    const bool ok = MessageSerializer::decodeFrames(buffer, frames);
    Q_ASSERT(ok);
    Q_ASSERT(frames.size() == 1);
    Q_ASSERT(frames[0].type == type);
    Q_ASSERT(buffer.isEmpty());
    return MessageSerializer::decodePayload<Msg>(frames[0].payload);
}

/// 测试端的裸 TCP 客户端辅助：连接 / 收帧（全部经 QTest::qWaitFor 驱动事件循环）
struct RawClient {
    QTcpSocket socket;
    QByteArray recvBuffer;
    QVector<Frame> frames;

    bool connectTo(quint16 port, int timeoutMs = 8000)
    {
        socket.connectToHost(QHostAddress::LocalHost, port);
        return QTest::qWaitFor(
            [this] { return socket.state() == QAbstractSocket::ConnectedState; },
            timeoutMs);
    }

    template <typename Msg>
    void send(MessageType type, const Msg& msg)
    {
        socket.write(MessageSerializer::encode(type, msg));
    }

    void send(MessageType type)
    {
        socket.write(MessageSerializer::encode(type));
    }

    /// 等待累计收到至少 count 帧（新帧追加到 frames）
    bool waitFrames(int count, int timeoutMs = 8000)
    {
        return QTest::qWaitFor([this, count] {
            recvBuffer.append(socket.readAll());
            MessageSerializer::decodeFrames(recvBuffer, frames);
            return frames.size() >= count;
        }, timeoutMs);
    }

    bool waitDisconnected(int timeoutMs = 8000)
    {
        return QTest::qWaitFor(
            [this] { return socket.state() == QAbstractSocket::UnconnectedState; },
            timeoutMs);
    }

    /// 完成连接 + 握手，返回分配的 playerId（失败返回 -2）
    int handshake(quint16 port)
    {
        if (!connectTo(port))
            return -2;
        send(MessageType::Handshake, HandshakeMsg{});
        if (!waitFrames(1))
            return -2;
        if (frames[0].type != MessageType::HandshakeAck)
            return -2;
        const auto ack =
            MessageSerializer::decodePayload<HandshakeAckMsg>(frames[0].payload);
        frames.clear();
        return ack.playerId;
    }
};

} // namespace

class NetworkTest : public QObject {
    Q_OBJECT

private slots:
    // ==================== 值类型 round-trip ====================

    void cardDataRoundTrip()
    {
        const CardData src = makeCardData();
        QByteArray buf;
        {
            QDataStream out(&buf, QDataStream::WriteOnly);
            out.setVersion(MessageSerializer::kStreamVersion);
            out << src;
        }
        CardData dst;
        {
            QDataStream in(buf);
            in.setVersion(MessageSerializer::kStreamVersion);
            in >> dst;
        }
        compareCardData(src, dst);
    }

    void playerDataRoundTrip()
    {
        const PlayerData src = makePlayerData();
        QByteArray buf;
        {
            QDataStream out(&buf, QDataStream::WriteOnly);
            out.setVersion(MessageSerializer::kStreamVersion);
            out << src;
        }
        PlayerData dst;
        {
            QDataStream in(buf);
            in.setVersion(MessageSerializer::kStreamVersion);
            in >> dst;
        }
        QCOMPARE(dst.playerId, src.playerId);
        QCOMPARE(dst.displayName, src.displayName);
        QCOMPARE(dst.characterName, src.characterName);
        QCOMPARE(dst.skillName, src.skillName);
        QCOMPARE(dst.skillDescription, src.skillDescription);
        QCOMPARE(dst.hp, src.hp);
        QCOMPARE(dst.maxHp, src.maxHp);
        QCOMPARE(dst.isAlive, src.isAlive);
        QCOMPARE(dst.isDying, src.isDying);
        QCOMPARE(dst.handCardCount, src.handCardCount);
        QCOMPARE(dst.handCardLimit, src.handCardLimit);
        QCOMPARE(dst.isCurrentPlayer, src.isCurrentPlayer);
        compareCardData(dst.weaponCard, src.weaponCard);
        compareCardData(dst.armorCard, src.armorCard);
        QCOMPARE(dst.armorCard.cardId, -1);  // 空槽语义保留
    }

    void pendingActionDataRoundTrip()
    {
        const PendingActionData src = makePendingActionData();
        QByteArray buf;
        {
            QDataStream out(&buf, QDataStream::WriteOnly);
            out.setVersion(MessageSerializer::kStreamVersion);
            out << src;
        }
        PendingActionData dst;
        {
            QDataStream in(buf);
            in.setVersion(MessageSerializer::kStreamVersion);
            in >> dst;
        }
        QCOMPARE(dst.sourceId, src.sourceId);
        QCOMPARE(dst.targetId, src.targetId);
        QCOMPARE(dst.sourceCardId, src.sourceCardId);
        QCOMPARE(dst.requiredCardType, src.requiredCardType);
        QCOMPARE(dst.description, src.description);
        QCOMPARE(dst.canSkip, src.canSkip);
        QCOMPARE(dst.remainingTargetIds, src.remainingTargetIds);
    }

    // ==================== 全部消息类型 帧级 round-trip ====================

    void handshakeRoundTrip()
    {
        HandshakeMsg src;
        src.protocolVersion = kProtocolVersion;
        const auto dst = frameRoundTrip(MessageType::Handshake, src);
        QCOMPARE(dst.protocolVersion, kProtocolVersion);
    }

    void handshakeAckRoundTrip()
    {
        HandshakeAckMsg src;
        src.playerId = 1;
        src.reason = QStringLiteral("");
        auto dst = frameRoundTrip(MessageType::HandshakeAck, src);
        QCOMPARE(dst.protocolVersion, kProtocolVersion);
        QCOMPARE(dst.playerId, 1);
        QVERIFY(dst.reason.isEmpty());

        // 拒绝分支
        src.playerId = -1;
        src.reason = QStringLiteral("房间已满");
        dst = frameRoundTrip(MessageType::HandshakeAck, src);
        QCOMPARE(dst.playerId, -1);
        QCOMPARE(dst.reason, QStringLiteral("房间已满"));
    }

    void selectCharacterRoundTrip()
    {
        SelectCharacterMsg src;
        src.characterId = 2;
        const auto dst = frameRoundTrip(MessageType::SelectCharacter, src);
        QCOMPARE(dst.characterId, 2);
    }

    void gameStartedRoundTrip()
    {
        GameStartedMsg src;
        src.characterId0 = 0;
        src.characterId1 = 3;
        const auto dst = frameRoundTrip(MessageType::GameStarted, src);
        QCOMPARE(dst.characterId0, 0);
        QCOMPARE(dst.characterId1, 3);
    }

    void playCardRoundTrip()
    {
        PlayCardMsg src;
        src.cardId = 12;
        src.playerId = 0;
        const auto dst = frameRoundTrip(MessageType::PlayCardRequested, src);
        QCOMPARE(dst.cardId, 12);
        QCOMPARE(dst.playerId, 0);
    }

    void respondCardRoundTrip()
    {
        RespondCardMsg src;
        src.cardId = 8;
        src.responderId = 1;
        const auto dst = frameRoundTrip(MessageType::RespondCardRequested, src);
        QCOMPARE(dst.cardId, 8);
        QCOMPARE(dst.responderId, 1);
    }

    void targetSelectedRoundTrip()
    {
        TargetSelectedMsg src;
        src.playerIndex = 1;
        const auto dst = frameRoundTrip(MessageType::TargetSelected, src);
        QCOMPARE(dst.playerIndex, 1);
    }

    void discardCardRoundTrip()
    {
        DiscardCardMsg src;
        src.cardId = 20;
        src.playerId = 1;
        const auto dst = frameRoundTrip(MessageType::DiscardCardRequested, src);
        QCOMPARE(dst.cardId, 20);
        QCOMPARE(dst.playerId, 1);
    }

    void phaseChangedRoundTrip()
    {
        PhaseChangedMsg src;
        src.phase = PhaseType::Play;
        const auto dst = frameRoundTrip(MessageType::PhaseChanged, src);
        QCOMPARE(dst.phase, PhaseType::Play);
    }

    void playerDataMsgRoundTrip()
    {
        PlayerDataMsg src;
        src.playerId = 1;
        src.data = makePlayerData();
        const auto dst = frameRoundTrip(MessageType::PlayerDataUpdated, src);
        QCOMPARE(dst.playerId, 1);
        QCOMPARE(dst.data.characterName, src.data.characterName);
        compareCardData(dst.data.weaponCard, src.data.weaponCard);
    }

    void handCardsMsgRoundTrip()
    {
        HandCardsMsg src;
        src.playerId = 0;
        src.cards = {makeCardData(), CardData{}};
        const auto dst = frameRoundTrip(MessageType::HandCardsUpdated, src);
        QCOMPARE(dst.playerId, 0);
        QCOMPARE(dst.cards.size(), 2);
        compareCardData(dst.cards[0], src.cards[0]);
        compareCardData(dst.cards[1], src.cards[1]);
    }

    void pendingActionMsgRoundTrip()
    {
        PendingActionMsg src;
        src.info = makePendingActionData();
        const auto dst = frameRoundTrip(MessageType::PendingActionCreated, src);
        QCOMPARE(dst.info.requiredCardType, CardType::Dodge);
        QCOMPARE(dst.info.remainingTargetIds, src.info.remainingTargetIds);
    }

    void gameOverRoundTrip()
    {
        GameOverMsg src;
        src.winnerId = 0;
        const auto dst = frameRoundTrip(MessageType::GameOver, src);
        QCOMPARE(dst.winnerId, 0);
    }

    void logMessageRoundTrip()
    {
        LogMessageMsg src;
        src.message = QStringLiteral("玩家1 使用了【杀】");
        const auto dst = frameRoundTrip(MessageType::LogMessage, src);
        QCOMPARE(dst.message, src.message);
    }

    void targetSelectionStartedRoundTrip()
    {
        TargetSelectionStartedMsg src;
        src.targetIds = {0, 1};
        const auto dst = frameRoundTrip(MessageType::TargetSelectionStarted, src);
        QCOMPARE(dst.targetIds, src.targetIds);
    }

    void payloadlessMessagesRoundTrip()
    {
        const MessageType types[] = {
            MessageType::EndPlayRequested, MessageType::AdvanceRequested,
            MessageType::SkipRequested,    MessageType::PendingActionCleared,
            MessageType::TargetSelectionFinished,
            MessageType::Ping,             MessageType::Pong,
        };
        for (MessageType t : types) {
            QByteArray buffer = MessageSerializer::encode(t);
            QVector<Frame> frames;
            QVERIFY(MessageSerializer::decodeFrames(buffer, frames));
            QCOMPARE(frames.size(), 1);
            QCOMPARE(frames[0].type, t);
            QVERIFY(frames[0].payload.isEmpty());
            QVERIFY(buffer.isEmpty());
        }
    }

    // ==================== 半包 / 粘包 ====================

    void partialFrameDecoding()
    {
        PlayCardMsg msg;
        msg.cardId = 5;
        msg.playerId = 0;
        const QByteArray full = MessageSerializer::encode(
            MessageType::PlayCardRequested, msg);

        // 按 1 字节逐步喂入，直到最后一字节前都不应解出帧
        QByteArray buffer;
        QVector<Frame> frames;
        for (int i = 0; i < full.size() - 1; ++i) {
            buffer.append(full.at(i));
            QVERIFY(MessageSerializer::decodeFrames(buffer, frames));
            QCOMPARE(frames.size(), 0);
        }
        buffer.append(full.at(full.size() - 1));
        QVERIFY(MessageSerializer::decodeFrames(buffer, frames));
        QCOMPARE(frames.size(), 1);
        QCOMPARE(frames[0].type, MessageType::PlayCardRequested);
        const auto decoded =
            MessageSerializer::decodePayload<PlayCardMsg>(frames[0].payload);
        QCOMPARE(decoded.cardId, 5);
        QVERIFY(buffer.isEmpty());
    }

    void stickyFramesDecoding()
    {
        // 三帧一次性到达（粘包）+ 第四帧只到一半
        PlayCardMsg play;
        play.cardId = 3;
        play.playerId = 1;
        LogMessageMsg log;
        log.message = QStringLiteral("粘包测试");

        QByteArray buffer;
        buffer.append(MessageSerializer::encode(MessageType::PlayCardRequested, play));
        buffer.append(MessageSerializer::encode(MessageType::Ping));
        buffer.append(MessageSerializer::encode(MessageType::LogMessage, log));
        const QByteArray fourth =
            MessageSerializer::encode(MessageType::SkipRequested);
        buffer.append(fourth.left(2));  // 第四帧半包

        QVector<Frame> frames;
        QVERIFY(MessageSerializer::decodeFrames(buffer, frames));
        QCOMPARE(frames.size(), 3);
        QCOMPARE(frames[0].type, MessageType::PlayCardRequested);
        QCOMPARE(frames[1].type, MessageType::Ping);
        QCOMPARE(frames[2].type, MessageType::LogMessage);
        QCOMPARE(MessageSerializer::decodePayload<PlayCardMsg>(frames[0].payload).cardId, 3);
        QCOMPARE(MessageSerializer::decodePayload<LogMessageMsg>(frames[2].payload).message,
                 log.message);
        QCOMPARE(buffer.size(), 2);  // 半包残留

        // 补齐第四帧
        buffer.append(fourth.mid(2));
        frames.clear();
        QVERIFY(MessageSerializer::decodeFrames(buffer, frames));
        QCOMPARE(frames.size(), 1);
        QCOMPARE(frames[0].type, MessageType::SkipRequested);
        QVERIFY(buffer.isEmpty());
    }

    // ==================== Step 2：ServerApp headless ====================

    void serverAppStartsHeadlessGame()
    {
        ServerApp app;
        QVERIFY(!app.isGameRunning());
        QVERIFY(app.actionViewModel() == nullptr);

        // 必须在 startHeadlessGame 之前拿不到 VM，之后立刻拿得到，
        // 所以信号 spy 只能在启动后对首批推送做不到监听——改用
        // 分两段：先启动，再断言状态 + 用第二局验证信号活性。
        app.startHeadlessGame(0, 1);  // 曹操 vs 关羽

        QVERIFY(app.isGameRunning());
        auto* gvm = app.gameViewModel();
        QVERIFY(gvm != nullptr);
        QVERIFY(app.actionViewModel() != nullptr);
        QVERIFY(gvm->gameState() != nullptr);

        // startGame 内部已完成初始发牌和阶段推进
        QCOMPARE(gvm->currentPlayerId(), 0);
    }

    void serverAppEmitsVmSignalsWithoutView()
    {
        ServerApp app;
        app.startHeadlessGame(0, 1);
        auto* gvm = app.gameViewModel();

        // 监听 VM 信号后驱动阶段推进，断言 headless 下信号正常发出。
        // startGame 后停在 Prepare 阶段，等 advanceRequested 推进
        // （与 View 模式下用户点击"继续"一致）。
        QSignalSpy phaseSpy(gvm, &GameViewModel::phaseChanged);
        QSignalSpy handSpy(gvm, &GameViewModel::handCardsUpdated);
        QSignalSpy playerSpy(gvm, &GameViewModel::playerDataUpdated);

        gvm->onAdvanceRequested();  // Prepare → Judge
        QVERIFY(phaseSpy.count() > 0);
        gvm->onAdvanceRequested();  // Judge → Draw（摸牌，触发手牌推送）
        QVERIFY(handSpy.count() > 0);
        QVERIFY(playerSpy.count() > 0);

        gvm->onAdvanceRequested();  // Draw → Play
        gvm->onEndPlayRequested();  // Play → Discard（无需弃牌则自动到 End）
        QCOMPARE(gvm->currentPlayerId(), 0);  // 回合尚未切换前仍是玩家 0
    }

    void serverAppFullTurnCycleHeadless()
    {
        // 完整回合循环：驱动到玩家 1 的回合，验证 headless 全程无卡点
        ServerApp app;
        app.startHeadlessGame(0, 1);
        auto* gvm = app.gameViewModel();

        gvm->onAdvanceRequested();  // Prepare → Judge
        gvm->onAdvanceRequested();  // Judge → Draw（摸 2 张，手牌 6 > 上限 4）
        gvm->onAdvanceRequested();  // Draw → Play
        gvm->onEndPlayRequested();  // Play → Discard（手牌超限，不自动跳过）
        QCOMPARE(gvm->gameState()->currentPhase(), PhaseType::Discard);

        // 弃到手牌不超限（onDiscardCardRequested 在弃满后自动推进到 End）
        auto* avm = app.actionViewModel();
        while (avm->getDiscardCount(0) > 0) {
            // 弃牌阶段任取一张手牌弃置
            const auto* state = gvm->gameState();
            const auto& hand = state->player(0)->handCards();
            QVERIFY(!hand.empty());
            gvm->onDiscardCardRequested(hand.front()->id(), 0);
        }
        QCOMPARE(gvm->gameState()->currentPhase(), PhaseType::End);

        gvm->onAdvanceRequested();  // End → 切换到玩家 1 的 Prepare
        QCOMPARE(gvm->currentPlayerId(), 1);
    }

    void serverAppRestartReleasesPreviousGame()
    {
        ServerApp app;
        app.startHeadlessGame(0, 1);
        auto* firstGvm = app.gameViewModel();
        QVERIFY(firstGvm != nullptr);

        app.startHeadlessGame(2, 3);  // 张飞 vs 赵云
        auto* secondGvm = app.gameViewModel();
        QVERIFY(secondGvm != nullptr);
        QVERIFY(secondGvm != firstGvm);
        QVERIFY(app.isGameRunning());

        // 新局 VM 信号正常
        QSignalSpy phaseSpy(secondGvm, &GameViewModel::phaseChanged);
        secondGvm->onAdvanceRequested();  // Prepare → Judge
        QVERIFY(phaseSpy.count() > 0);
    }

    // ==================== Step 3：GameServer 握手 ====================

    void handshakeAssignsPlayerIds()
    {
        GameServer server;
        QVERIFY(server.listen(0, true));  // 随机端口，避免测试环境端口冲突
        const quint16 port = server.serverPort();
        QSignalSpy connectedSpy(&server, &GameServer::clientConnected);

        RawClient c0, c1;
        QCOMPARE(c0.handshake(port), 0);
        QCOMPARE(c1.handshake(port), 1);
        QCOMPARE(server.connectedClientCount(), 2);
        QVERIFY(server.isPlayerConnected(0));
        QVERIFY(server.isPlayerConnected(1));
        QCOMPARE(connectedSpy.count(), 2);
        QCOMPARE(connectedSpy.at(0).at(0).toInt(), 0);
        QCOMPARE(connectedSpy.at(1).at(0).toInt(), 1);
    }

    void thirdConnectionRejected()
    {
        GameServer server;
        QVERIFY(server.listen(0, true));
        const quint16 port = server.serverPort();

        RawClient c0, c1;
        QCOMPARE(c0.handshake(port), 0);
        QCOMPARE(c1.handshake(port), 1);

        // 第三个连接：连上即收到拒绝 Ack（playerId=-1），随后被断开
        RawClient c2;
        QVERIFY(c2.connectTo(port));
        QVERIFY(c2.waitFrames(1));
        QCOMPARE(c2.frames[0].type, MessageType::HandshakeAck);
        const auto ack = MessageSerializer::decodePayload<HandshakeAckMsg>(
            c2.frames[0].payload);
        QCOMPARE(ack.playerId, -1);
        QCOMPARE(ack.reason, QStringLiteral("房间已满"));
        QVERIFY(c2.waitDisconnected());

        // 原有两个连接不受影响
        QCOMPARE(server.connectedClientCount(), 2);
    }

    void versionMismatchRejected()
    {
        GameServer server;
        QVERIFY(server.listen(0, true));

        RawClient c;
        QVERIFY(c.connectTo(server.serverPort()));
        HandshakeMsg msg;
        msg.protocolVersion = 999;
        c.send(MessageType::Handshake, msg);
        QVERIFY(c.waitFrames(1));
        const auto ack = MessageSerializer::decodePayload<HandshakeAckMsg>(
            c.frames[0].payload);
        QCOMPARE(ack.playerId, -1);
        QVERIFY(ack.reason.contains(QStringLiteral("协议版本")));
        QVERIFY(c.waitDisconnected());
        QCOMPARE(server.connectedClientCount(), 0);
    }

    void selectCharacterTriggersReady()
    {
        GameServer server;
        QVERIFY(server.listen(0, true));
        const quint16 port = server.serverPort();
        QSignalSpy readySpy(&server, &GameServer::bothPlayersReady);

        RawClient c0, c1;
        QCOMPARE(c0.handshake(port), 0);
        QCOMPARE(c1.handshake(port), 1);

        SelectCharacterMsg sel0;
        sel0.characterId = 0;  // 曹操
        c0.send(MessageType::SelectCharacter, sel0);
        QTest::qWait(50);
        QCOMPARE(readySpy.count(), 0);  // 只有一方选将，不触发
        QCOMPARE(server.selectedCharacter(0), 0);

        SelectCharacterMsg sel1;
        sel1.characterId = 1;  // 关羽
        c1.send(MessageType::SelectCharacter, sel1);
        QVERIFY(QTest::qWaitFor([&] { return readySpy.count() > 0; }, 8000));
        QCOMPARE(readySpy.count(), 1);
        QCOMPARE(readySpy.at(0).at(0).toInt(), 0);
        QCOMPARE(readySpy.at(0).at(1).toInt(), 1);
    }

    void disconnectFreesSlotForReconnect()
    {
        GameServer server;
        QVERIFY(server.listen(0, true));
        const quint16 port = server.serverPort();
        QSignalSpy disconnectedSpy(&server, &GameServer::clientDisconnected);

        RawClient c0;
        auto* c1 = new RawClient;
        QCOMPARE(c0.handshake(port), 0);
        QCOMPARE(c1->handshake(port), 1);

        c1->socket.disconnectFromHost();
        QVERIFY(QTest::qWaitFor(
            [&] { return disconnectedSpy.count() > 0; }, 8000));
        QCOMPARE(disconnectedSpy.at(0).at(0).toInt(), 1);
        QCOMPARE(server.connectedClientCount(), 1);
        delete c1;

        // 新客户端拿到被释放的 playerId 1
        RawClient c2;
        QCOMPARE(c2.handshake(port), 1);
        QCOMPARE(server.connectedClientCount(), 2);
    }

    void preHandshakeCommandsIgnored()
    {
        GameServer server;
        QVERIFY(server.listen(0, true));
        QSignalSpy commandSpy(&server, &GameServer::clientCommandReceived);
        QSignalSpy readySpy(&server, &GameServer::bothPlayersReady);

        RawClient c;
        QVERIFY(c.connectTo(server.serverPort()));
        // 未握手直接发命令/选将：全部忽略，不崩溃、不转发
        PlayCardMsg play;
        play.cardId = 1;
        play.playerId = 0;
        c.send(MessageType::PlayCardRequested, play);
        SelectCharacterMsg sel;
        sel.characterId = 0;
        c.send(MessageType::SelectCharacter, sel);
        QTest::qWait(100);
        QCOMPARE(commandSpy.count(), 0);
        QCOMPARE(readySpy.count(), 0);
        QCOMPARE(server.selectedCharacter(0), -1);
    }

    void corruptedStreamDropsClient()
    {
        GameServer server;
        QVERIFY(server.listen(0, true));
        RawClient c;
        QCOMPARE(c.handshake(server.serverPort()), 0);

        // 直接写超限长度前缀 → 服务器判定流损坏并断开
        QByteArray garbage;
        {
            QDataStream out(&garbage, QDataStream::WriteOnly);
            out.setVersion(MessageSerializer::kStreamVersion);
            out << quint32(Protocol::kMaxFrameSize + 1);
        }
        c.socket.write(garbage);
        QVERIFY(c.waitDisconnected());
        QCOMPARE(server.connectedClientCount(), 0);
    }

    void corruptedLengthPrefixRejected()
    {
        // 长度前缀超过 kMaxFrameSize → 流损坏
        QByteArray buffer;
        {
            QDataStream out(&buffer, QDataStream::WriteOnly);
            out.setVersion(MessageSerializer::kStreamVersion);
            out << quint32(Protocol::kMaxFrameSize + 1);
        }
        buffer.append(char(0));
        QVector<Frame> frames;
        QVERIFY(!MessageSerializer::decodeFrames(buffer, frames));
        QCOMPARE(frames.size(), 0);

        // 长度为 0（连 type 字节都没有）也视为损坏
        QByteArray zeroBuffer;
        {
            QDataStream out(&zeroBuffer, QDataStream::WriteOnly);
            out.setVersion(MessageSerializer::kStreamVersion);
            out << quint32(0);
        }
        frames.clear();
        QVERIFY(!MessageSerializer::decodeFrames(zeroBuffer, frames));
    }
};

QTEST_GUILESS_MAIN(NetworkTest)
#include "network_test.moc"
