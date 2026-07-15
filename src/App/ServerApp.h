#ifndef SERVERAPP_H
#define SERVERAPP_H

#include <QObject>

class GameViewModel;
class ActionViewModel;

/// 服务器组合根 — headless 运行完整的 GameViewModel/ActionViewModel/Model，
/// 不创建任何 QWidget。后续步骤（Step 4）在此接入 GameServer：
/// VM 信号 → GameServer 广播槽，客户端命令 → VM public slots。
class ServerApp : public QObject {
    Q_OBJECT
public:
    explicit ServerApp(QObject* parent = nullptr);

    /// 无 UI 启动一局游戏（与 SGSApp::startLocalGame 相同的 VM 生命周期，
    /// 但不创建 GameBoardWidget）。重复调用会先释放上一局对象图。
    void startHeadlessGame(int charId1, int charId2);

    /// 是否有对局在进行
    bool isGameRunning() const { return m_gvm != nullptr; }

    /// 供组装/测试访问（Step 4 中 GameServer 的命令分发需要）
    GameViewModel* gameViewModel() const { return m_gvm; }
    ActionViewModel* actionViewModel() const;

signals:
    /// 对局结束（winnerId 透传自 GameViewModel::gameOver）
    void gameFinished(int winnerId);

private slots:
    void onGameOver(int winnerId);

private:
    GameViewModel* m_gvm = nullptr;
};

#endif // SERVERAPP_H
