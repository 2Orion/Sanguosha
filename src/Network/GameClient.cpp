#include "GameClient.h"

using namespace Protocol;
using MessageSerializer::Frame;

GameClient::GameClient(QObject* parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::connected, this, &GameClient::onSocketConnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &GameClient::onSocketReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &GameClient::onSocketDisconnected);
    connect(m_socket, &QAbstractSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError) {
        emit connectionError(m_socket->errorString());
    });
}

void GameClient::connectToServer(const QString& host, quint16 port)
{
    m_recvBuffer.clear();
    m_localPlayerId = -1;
    m_socket->connectToHost(host, port);
}

void GameClient::disconnectFromServer()
{
    m_socket->disconnectFromHost();
}

bool GameClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void GameClient::selectCharacter(int characterId)
{
    SelectCharacterMsg msg;
    msg.characterId = characterId;
    send(MessageType::SelectCharacter, msg);
}

void GameClient::playCard(int cardId, int playerId)
{
    PlayCardMsg msg;
    msg.cardId = cardId;
    msg.playerId = playerId;
    send(MessageType::PlayCardRequested, msg);
}

void GameClient::respondCard(int cardId, int responderId)
{
    RespondCardMsg msg;
    msg.cardId = cardId;
    msg.responderId = responderId;
    send(MessageType::RespondCardRequested, msg);
}

void GameClient::selectTarget(int playerIndex)
{
    TargetSelectedMsg msg;
    msg.playerIndex = playerIndex;
    send(MessageType::TargetSelected, msg);
}

void GameClient::discardCard(int cardId, int playerId)
{
    DiscardCardMsg msg;
    msg.cardId = cardId;
    msg.playerId = playerId;
    send(MessageType::DiscardCardRequested, msg);
}

void GameClient::endPlayPhase() { send(MessageType::EndPlayRequested); }
void GameClient::advancePhase() { send(MessageType::AdvanceRequested); }
void GameClient::skipResponse() { send(MessageType::SkipRequested); }

void GameClient::send(MessageType type)
{
    m_socket->write(MessageSerializer::encode(type));
}

void GameClient::onSocketConnected()
{
    send(MessageType::Handshake, HandshakeMsg{});
}

void GameClient::onSocketReadyRead()
{
    m_recvBuffer.append(m_socket->readAll());

    QVector<Frame> frames;
    if (!MessageSerializer::decodeFrames(m_recvBuffer, frames)) {
        // 流损坏（长度前缀超过 kMaxFrameSize）：断开，避免继续解析垃圾数据
        m_socket->disconnectFromHost();
        return;
    }
    for (const Frame& frame : frames)
        dispatchMessage(frame.type, frame.payload);
}

void GameClient::onSocketDisconnected()
{
    m_localPlayerId = -1;
    emit disconnected();
}

void GameClient::dispatchMessage(MessageType type, const QByteArray& payload)
{
    switch (type) {
    case MessageType::HandshakeAck: {
        const auto ack = MessageSerializer::decodePayload<HandshakeAckMsg>(payload);
        if (ack.playerId < 0) {
            emit connectionRejected(ack.reason);
        } else {
            m_localPlayerId = ack.playerId;
            emit connected(ack.playerId);
        }
        return;
    }
    case MessageType::GameStarted: {
        const auto msg = MessageSerializer::decodePayload<GameStartedMsg>(payload);
        emit gameStarted(msg.characterId0, msg.characterId1);
        return;
    }
    case MessageType::PhaseChanged: {
        const auto msg = MessageSerializer::decodePayload<PhaseChangedMsg>(payload);
        emit phaseChanged(msg.phase);
        return;
    }
    case MessageType::PlayerDataUpdated: {
        const auto msg = MessageSerializer::decodePayload<PlayerDataMsg>(payload);
        emit playerDataUpdated(msg.playerId, msg.data);
        return;
    }
    case MessageType::HandCardsUpdated: {
        const auto msg = MessageSerializer::decodePayload<HandCardsMsg>(payload);
        emit handCardsUpdated(msg.playerId, msg.cards);
        return;
    }
    case MessageType::PendingActionCreated: {
        const auto msg = MessageSerializer::decodePayload<PendingActionMsg>(payload);
        emit pendingActionCreated(msg.info);
        return;
    }
    case MessageType::PendingActionCleared:
        emit pendingActionCleared();
        return;
    case MessageType::GameOver: {
        const auto msg = MessageSerializer::decodePayload<GameOverMsg>(payload);
        emit gameOver(msg.winnerId);
        return;
    }
    case MessageType::LogMessage: {
        const auto msg = MessageSerializer::decodePayload<LogMessageMsg>(payload);
        emit logMessage(msg.message);
        return;
    }
    case MessageType::TargetSelectionStarted: {
        const auto msg = MessageSerializer::decodePayload<TargetSelectionStartedMsg>(payload);
        emit targetSelectionStarted(msg.targetIds);
        return;
    }
    case MessageType::TargetSelectionFinished:
        emit targetSelectionFinished();
        return;
    case MessageType::Ping:
        send(MessageType::Pong);
        return;
    default:
        return;  // Handshake/SelectCharacter（客户端不会收到）等忽略
    }
}
