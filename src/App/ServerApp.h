#ifndef SERVERAPP_H
#define SERVERAPP_H

#include <QObject>
#include <QByteArray>
#include "Protocol.h"

class GameViewModel;
class ActionViewModel;
class GameServer;

/// 服务器组合根 — headless 运行完整的 GameViewModel/ActionViewModel/Model，
/// 不创建任何 QWidget。持有 GameServer：
///   · VM 对外信号（同 View 收到的形状，见 connection.md）→ 序列化 → GameServer::broadcast
///   · GameServer 收到的客户端命令 → 反序列化 → 调 VM 的 public slots
/// 手牌广播已按接收方脱敏（Step 5）：所有者本人收完整 CardList，对手收
/// `Protocol::redactCardList` 处理后的占位牌面（保留 cardId/ownerId）。
class ServerApp : public QObject {
    Q_OBJECT
public:
    explicit ServerApp(QObject* parent = nullptr);

    /// 无 UI 启动一局游戏（与 SGSApp::startLocalGame 相同的 VM 生命周期，
    /// 但不创建 GameBoardWidget）。重复调用会先释放上一局对象图。
    /// 内部会在调用 GameViewModel::startGame 之前完成 VM→GameServer 的信号
    /// 接线，保证开局的初始状态推送（phaseChanged/handCardsUpdated 等）
    /// 不会因为"先 startGame 后接线"而丢给客户端。
    void startHeadlessGame(int charId1, int charId2);

    /// 是否有对局在进行
    bool isGameRunning() const { return m_gvm != nullptr; }

    /// 开始监听（转发给内部持有的 GameServer）
    bool listen(quint16 port = Protocol::kDefaultPort, bool localOnly = false);

    /// 供组装/测试访问
    GameViewModel* gameViewModel() const { return m_gvm; }
    ActionViewModel* actionViewModel() const;
    GameServer* gameServer() const { return m_server; }

signals:
    /// 对局结束（winnerId 透传自 GameViewModel::gameOver）
    void gameFinished(int winnerId);

private slots:
    void onGameOver(int winnerId);
    /// GameServer::bothPlayersReady：双方选将完成，广播 GameStarted 并开局
    void onBothPlayersReady(int charId0, int charId1);
    /// GameServer::clientCommandReceived：按 MessageType 分发到 VM public slots。
    /// playerId 取自连接层（GameServer 分配、握手时确定），不信任消息体里
    /// 客户端自称的 playerId/responderId 字段——否则玩家 1 可以在消息里把
    /// playerId 填成 0，冒充玩家 0 操作对方手牌（ActionViewModel::isOwnCard
    /// 只按传入的 playerId 校验归属，不知道消息实际来自哪个网络连接）。
    void onClientCommandReceived(int playerId, Protocol::MessageType type,
                                 const QByteArray& payload);

private:
    /// 把当前 m_gvm/m_actionVM 的对外信号连到 GameServer 广播（每次新建
    /// VM 后调用一次；旧 VM 销毁时旧连接随 QObject 生命周期自动断开）
    void wireViewModelBroadcasts();

    GameViewModel* m_gvm = nullptr;
    GameServer* m_server = nullptr;
};

#endif // SERVERAPP_H
