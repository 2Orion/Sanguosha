#ifndef SGSAPP_H
#define SGSAPP_H

#include <QObject>
#include <QMainWindow>

class GameViewModel;
class GameBoardWidget;
class ServerApp;
class ClientApp;

/// 组合根 — 创建并持有所有顶层对象，绑定 View ↔ ViewModel 信号
class SGSApp : public QObject {
    Q_OBJECT
public:
    explicit SGSApp(QObject* parent = nullptr);

    /// 主窗口（main.cpp 只需 show()，不需要知道 MainWindow 的具体类型）
    QMainWindow* mainWindow() const { return m_mainWindow; }

    /// 开始一局新游戏（本地模式）
    void startLocalGame(int charId1, int charId2);

private slots:
    void onGameFinished();
    void onCreateRoom();
    void onJoinRoom(const QString& host, quint16 port);
    void onCharacterConfirmed(int charId);
    void onClientGameStarted(int charId0, int charId1);
    void onNetworkGameOver(int winnerId);
    void onConnectionError(const QString& error);

private:
    void releaseLocalGame();
    void releaseNetworkGame();
    void releaseClient();

    QMainWindow*      m_mainWindow = nullptr;

    // 本地模式
    GameViewModel*    m_gvm = nullptr;
    GameBoardWidget*  m_board = nullptr;

    // 联网模式
    ServerApp*        m_serverApp = nullptr;
    ClientApp*        m_clientApp = nullptr;
    int               m_myPlayerId = -1;
};

#endif // SGSAPP_H
