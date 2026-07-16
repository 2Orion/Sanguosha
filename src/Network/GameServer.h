#ifndef NETWORK_GAMESERVER_H
#define NETWORK_GAMESERVER_H

#include <QObject>
#include <QByteArray>
#include <QElapsedTimer>
#include <array>
#include "Protocol.h"
#include "MessageSerializer.h"

class QTcpServer;
class QTcpSocket;
class QTimer;

/// 服务器网络层 — "网络化的 View"。
/// Step 3 范围：QTcpServer 监听、最多 2 连接、playerId 0/1 分配、
/// 帧解码、Handshake/HandshakeAck/SelectCharacter 流程、超员拒绝。
/// VM 信号广播槽与客户端命令分发在 Step 4 接入。
/// Step 8 范围：心跳保活——按 `m_heartbeatIntervalMs` 周期给每个已握手客户端发
/// `Ping`；若某客户端连续 `m_heartbeatTimeoutMs` 未收到任何帧（含 `Pong` 及其他
/// 命令帧，均视为存活证据），判定其失联并踢出（同 `onSocketDisconnected` 一样
/// `dropClient` + `emit clientDisconnected`）。断线重连不做（首版明确砍掉）。
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

    /// 心跳参数（默认 3000ms/10000ms，对齐 plan2.0.md §2 Step 8）。
    /// 必须在 listen() 之前调用才能影响首次心跳周期；listen() 之后调用会立即
    /// 生效于运行中的定时器（供测试用短超时参数快速复现无响应客户端被踢场景）。
    void setHeartbeatIntervalMs(int ms);
    void setHeartbeatTimeoutMs(int ms) { m_heartbeatTimeoutMs = ms; }

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
    void onHeartbeatTick();

private:
    struct ClientSlot {
        QTcpSocket* socket = nullptr;
        QByteArray recvBuffer;
        bool handshaken = false;       // 已完成 Handshake/Ack
        int selectedCharacterId = -1;  // SelectCharacter 结果
        QElapsedTimer lastSeenTimer;   // 最近一次收到该客户端任意帧的时间（心跳依据）
    };

    int slotIndexOf(const QTcpSocket* socket) const;
    void handleFrame(int playerId, const MessageSerializer::Frame& frame);
    void rejectConnection(QTcpSocket* socket, const QString& reason);
    void dropClient(int playerId);

    QTcpServer* m_server = nullptr;
    QTimer* m_heartbeatTimer = nullptr;
    int m_heartbeatIntervalMs = 3000;
    int m_heartbeatTimeoutMs = 10000;
    std::array<ClientSlot, 2> m_clients;
};

#endif // NETWORK_GAMESERVER_H
