#include "GameServer.h"

#include <QTcpServer>
#include <QTcpSocket>

using namespace Protocol;
using MessageSerializer::Frame;

GameServer::GameServer(QObject* parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection,
            this, &GameServer::onNewConnection);
}

bool GameServer::listen(quint16 port, bool localOnly)
{
    return m_server->listen(
        localOnly ? QHostAddress::LocalHost : QHostAddress::Any, port);
}

void GameServer::close()
{
    m_server->close();
    for (int i = 0; i < int(m_clients.size()); ++i)
        dropClient(i);
}

quint16 GameServer::serverPort() const
{
    return m_server->serverPort();
}

int GameServer::connectedClientCount() const
{
    int count = 0;
    for (const auto& c : m_clients)
        if (c.socket) ++count;
    return count;
}

bool GameServer::isPlayerConnected(int playerId) const
{
    return playerId >= 0 && playerId < int(m_clients.size())
        && m_clients[playerId].socket != nullptr;
}

int GameServer::selectedCharacter(int playerId) const
{
    if (playerId < 0 || playerId >= int(m_clients.size()))
        return -1;
    return m_clients[playerId].selectedCharacterId;
}

void GameServer::sendTo(int playerId, MessageType type, const QByteArray& payload)
{
    if (!isPlayerConnected(playerId))
        return;
    // payload 已由调用方通过 MessageSerializer::encodePayload 序列化；
    // 这里只补帧头（长度前缀 + 类型）
    m_clients[playerId].socket->write(MessageSerializer::wrapFrame(type, payload));
}

void GameServer::broadcast(MessageType type, const QByteArray& payload)
{
    for (int i = 0; i < int(m_clients.size()); ++i)
        sendTo(i, type, payload);
}

void GameServer::onNewConnection()
{
    while (QTcpSocket* socket = m_server->nextPendingConnection()) {
        // 找空槽；占满则拒绝（先回 Ack(-1) 再断开，客户端可显示原因）
        int slot = -1;
        for (int i = 0; i < int(m_clients.size()); ++i) {
            if (!m_clients[i].socket) { slot = i; break; }
        }
        if (slot < 0) {
            rejectConnection(socket, QStringLiteral("房间已满"));
            continue;
        }

        m_clients[slot].socket = socket;
        m_clients[slot].recvBuffer.clear();
        m_clients[slot].handshaken = false;
        m_clients[slot].selectedCharacterId = -1;
        connect(socket, &QTcpSocket::readyRead,
                this, &GameServer::onSocketReadyRead);
        connect(socket, &QTcpSocket::disconnected,
                this, &GameServer::onSocketDisconnected);
    }
}

void GameServer::onSocketReadyRead()
{
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    const int playerId = slotIndexOf(socket);
    if (playerId < 0)
        return;

    ClientSlot& client = m_clients[playerId];
    client.recvBuffer.append(socket->readAll());

    QVector<Frame> frames;
    if (!MessageSerializer::decodeFrames(client.recvBuffer, frames)) {
        // 流损坏：断开该客户端
        dropClient(playerId);
        emit clientDisconnected(playerId);
        return;
    }
    for (const Frame& frame : frames)
        handleFrame(playerId, frame);
}

void GameServer::onSocketDisconnected()
{
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    const int playerId = slotIndexOf(socket);
    if (playerId < 0)
        return;
    const bool wasHandshaken = m_clients[playerId].handshaken;
    dropClient(playerId);
    if (wasHandshaken)
        emit clientDisconnected(playerId);
}

int GameServer::slotIndexOf(const QTcpSocket* socket) const
{
    for (int i = 0; i < int(m_clients.size()); ++i)
        if (m_clients[i].socket == socket)
            return i;
    return -1;
}

void GameServer::handleFrame(int playerId, const Frame& frame)
{
    ClientSlot& client = m_clients[playerId];

    switch (frame.type) {
    case MessageType::Handshake: {
        const auto msg =
            MessageSerializer::decodePayload<HandshakeMsg>(frame.payload);
        if (msg.protocolVersion != kProtocolVersion) {
            HandshakeAckMsg ack;
            ack.playerId = -1;
            ack.reason = QStringLiteral("协议版本不匹配（服务器 v%1，客户端 v%2）")
                             .arg(kProtocolVersion).arg(msg.protocolVersion);
            client.socket->write(MessageSerializer::encode(
                MessageType::HandshakeAck, ack));
            client.socket->disconnectFromHost();
            return;
        }
        client.handshaken = true;
        HandshakeAckMsg ack;
        ack.playerId = playerId;
        client.socket->write(MessageSerializer::encode(
            MessageType::HandshakeAck, ack));
        emit clientConnected(playerId);
        return;
    }
    case MessageType::SelectCharacter: {
        if (!client.handshaken)
            return;  // 未握手先选将：忽略
        const auto msg =
            MessageSerializer::decodePayload<SelectCharacterMsg>(frame.payload);
        client.selectedCharacterId = msg.characterId;
        if (m_clients[0].selectedCharacterId >= 0
            && m_clients[1].selectedCharacterId >= 0) {
            emit bothPlayersReady(m_clients[0].selectedCharacterId,
                                  m_clients[1].selectedCharacterId);
        }
        return;
    }
    default:
        if (!client.handshaken)
            return;  // 未握手的命令一律忽略
        emit clientCommandReceived(playerId, frame.type, frame.payload);
        return;
    }
}

void GameServer::rejectConnection(QTcpSocket* socket, const QString& reason)
{
    HandshakeAckMsg ack;
    ack.playerId = -1;
    ack.reason = reason;
    socket->write(MessageSerializer::encode(MessageType::HandshakeAck, ack));
    socket->disconnectFromHost();
    // 未进槽位的 socket：断开后自行销毁
    connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
}

void GameServer::dropClient(int playerId)
{
    ClientSlot& client = m_clients[playerId];
    if (!client.socket)
        return;
    client.socket->disconnect(this);  // 防止 dropClient 期间再次进槽处理
    client.socket->abort();
    client.socket->deleteLater();
    client.socket = nullptr;
    client.recvBuffer.clear();
    client.handshaken = false;
    client.selectedCharacterId = -1;
}
