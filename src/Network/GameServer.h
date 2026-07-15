#ifndef NETWORK_GAMESERVER_H
#define NETWORK_GAMESERVER_H

#include <QObject>
#include <QByteArray>
#include <array>
#include "Protocol.h"
#include "MessageSerializer.h"

class QTcpServer;
class QTcpSocket;

/// 服务器网络层 — "网络化的 View"。
/// Step 3 范围：QTcpServer 监听、最多 2 连接、playerId 0/1 分配、
/// 帧解码、Handshake/HandshakeAck/SelectCharacter 流程、超员拒绝。
/// VM 信号广播槽与客户端命令分发在 Step 4 接入。
/// 约束：不持有/传递任何 Model 指针，只经手 Common 值类型与协议消息。
class GameServer : public QObject {
    Q_OBJECT
public:
    explicit GameServer(QObject* parent = nullptr);

    /// 开始监听。port = 0 时由系统分配空闲端口（测试用），
    /// 实际端口用 serverPort() 查询。
    /// localOnly = true 只绑 127.0.0.1（loopback 测试用，不触发
    /// Windows 防火墙放行弹窗）；局域网对战用默认 false（绑 Any）。
    bool listen(quint16 port = Protocol::kDefaultPort, bool localOnly = false);
    void close();
    quint16 serverPort() const;

    int connectedClientCount() const;
    bool isPlayerConnected(int playerId) const;
    /// 玩家已选的武将 id（未选返回 -1）
    int selectedCharacter(int playerId) const;

signals:
    void clientConnected(int playerId);
    void clientDisconnected(int playerId);
    /// 双方都完成 SelectCharacter（Step 4 由 ServerApp 接到 startHeadlessGame）
    void bothPlayersReady(int charId0, int charId1);
    /// 收到客户端命令消息（握手/选将之外的类型；Step 4 中由 ServerApp
    /// 按类型分发到 VM public slots）
    void clientCommandReceived(int playerId, Protocol::MessageType type,
                               const QByteArray& payload);

public:
    /// 定向发送 / 广播（Step 4 的广播槽也走这两个出口）
    void sendTo(int playerId, Protocol::MessageType type,
                const QByteArray& payload = {});
    void broadcast(Protocol::MessageType type, const QByteArray& payload = {});

private slots:
    void onNewConnection();
    void onSocketReadyRead();
    void onSocketDisconnected();

private:
    struct ClientSlot {
        QTcpSocket* socket = nullptr;
        QByteArray recvBuffer;
        bool handshaken = false;       // 已完成 Handshake/Ack
        int selectedCharacterId = -1;  // SelectCharacter 结果
    };

    int slotIndexOf(const QTcpSocket* socket) const;
    void handleFrame(int playerId, const MessageSerializer::Frame& frame);
    void rejectConnection(QTcpSocket* socket, const QString& reason);
    void dropClient(int playerId);

    QTcpServer* m_server = nullptr;
    std::array<ClientSlot, 2> m_clients;
};

#endif // NETWORK_GAMESERVER_H
