#ifndef CLIENTAPP_H
#define CLIENTAPP_H

#include <QObject>
#include <QString>
#include <QtGlobal>

class GameClient;
class GameBoardWidget;

/// 客户端组合根 —— 创建 GameBoardWidget + GameClient，按 SGSApp::startLocalGame()
/// 同样的形状建立双向信号连接。GameBoardWidget 全程零改动：它本来就不知道
/// 对面接的是本地 ViewModel 还是网络 GameClient，因为两者对外信号/槽形状完全一致。
class ClientApp : public QObject {
    Q_OBJECT
public:
    explicit ClientApp(QObject* parent = nullptr);

    /// 游戏桌面（供调用方嵌入到自己的窗口容器里；ClientApp 不创建 MainWindow）
    GameBoardWidget* boardWidget() const { return m_board; }

    /// 网络生命周期信号来源（connected/connectionRejected/connectionError/
    /// disconnected/gameStarted），供调用方接入登录/等待界面
    GameClient* gameClient() const { return m_client; }

    void connectToServer(const QString& host, quint16 port);
    void selectCharacter(int characterId);

private:
    GameClient*      m_client = nullptr;
    GameBoardWidget* m_board = nullptr;
};

#endif // CLIENTAPP_H
