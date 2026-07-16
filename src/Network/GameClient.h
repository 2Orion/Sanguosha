#ifndef NETWORK_GAMECLIENT_H
#define NETWORK_GAMECLIENT_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QTcpSocket>
#include "Protocol.h"
#include "MessageSerializer.h"

/// 客户端网络层 —— "网络化的 ViewModel"。
/// 对外暴露与 GameViewModel/ActionViewModel 完全相同形状的信号：内容全部由收到的
/// 服务器广播反序列化后直接 emit，不经过任何本地 Model/规则计算。
/// 发送方法与 GameBoardWidget 的命令信号一一对应：只把参数打包发给服务器，不做
/// 任何校验或判断——校验全部交给服务器（见 ServerApp::onClientCommandReceived 和
/// ActionViewModel 的权限校验），本类零 Model 依赖、零规则判断。
/// 约束：不持有/传递任何 Model 指针，只经手 Common 值类型与协议消息。
class GameClient : public QObject {
    Q_OBJECT
public:
    explicit GameClient(QObject* parent = nullptr);

    void connectToServer(const QString& host, quint16 port = Protocol::kDefaultPort);
    void disconnectFromServer();
    bool isConnected() const;

    /// 握手成功后分配的 playerId；未连接/未握手成功时为 -1
    int localPlayerId() const { return m_localPlayerId; }

    /// 选将（连接成功后调用一次，等待对手也完成选将触发 GameStarted）
    void selectCharacter(int characterId);

    // ==================== 与 GameBoardWidget 命令信号一一对应的发送方法 ====================
    // 命名/参数形状对齐 ActionViewModel::onPlayCardRequested 等 public slots，
    // ClientApp（Step 7）会把 GameBoardWidget 的信号直接连到这些方法上。
    void playCard(int cardId, int playerId);
    void respondCard(int cardId, int responderId);
    void selectTarget(int playerIndex);
    void discardCard(int cardId, int playerId);
    void endPlayPhase();
    void advancePhase();
    void skipResponse();

signals:
    // ==================== 网络生命周期（本地模式没有对应物） ====================
    void connected(int playerId);                    // 握手成功，分配的 playerId
    void connectionRejected(const QString& reason);   // 握手失败：版本不符/房间已满
    void connectionError(const QString& error);       // socket 层错误（如目标不可达）
    void disconnected();
    void gameStarted(int characterId0, int characterId1);

    // ==================== 与 GameViewModel/ActionViewModel 相同形状 ====================
    void phaseChanged(PhaseType phase);
    void playerDataUpdated(int playerId, const PlayerData& data);
    void handCardsUpdated(int playerId, const CardList& cards);
    void pendingActionCreated(const PendingActionData& info);
    void pendingActionCleared();
    void gameOver(int winnerId);
    void logMessage(const QString& msg);
    void targetSelectionStarted(const QVector<int>& targetIds);
    void targetSelectionFinished();

private slots:
    void onSocketConnected();
    void onSocketReadyRead();
    void onSocketDisconnected();

private:
    void dispatchMessage(Protocol::MessageType type, const QByteArray& payload);

    template <typename Msg>
    void send(Protocol::MessageType type, const Msg& msg)
    {
        m_socket->write(MessageSerializer::encode(type, msg));
    }
    void send(Protocol::MessageType type);

    QTcpSocket* m_socket;
    QByteArray m_recvBuffer;
    int m_localPlayerId = -1;
};

#endif // NETWORK_GAMECLIENT_H
