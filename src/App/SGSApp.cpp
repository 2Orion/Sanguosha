#include "SGSApp.h"
#include "GameViewModel.h"
#include "ActionViewModel.h"
#include "GameBoardWidget.h"
#include "MainWindow.h"
#include "ServerApp.h"
#include "ClientApp.h"
#include "GameClient.h"
#include <QMessageBox>
#include <QNetworkInterface>

SGSApp::SGSApp(QObject* parent)
    : QObject(parent)
{
    // 1. 创建主窗口
    auto* mainWin = new MainWindow();
    mainWin->setAttribute(Qt::WA_DeleteOnClose, false);
    m_mainWindow = mainWin;

    // 2. 本地模式选将信号
    connect(mainWin, &MainWindow::startGameRequested,
            this, &SGSApp::startLocalGame);

    // 3. 联网模式信号
    connect(mainWin, &MainWindow::createRoomRequested,
            this, &SGSApp::onCreateRoom);
    connect(mainWin, &MainWindow::joinRoomRequested,
            this, &SGSApp::onJoinRoom);
    connect(mainWin, &MainWindow::characterConfirmed,
            this, &SGSApp::onCharacterConfirmed);
}

// ==================== 本地模式 ====================

void SGSApp::startLocalGame(int charId1, int charId2)
{
    releaseLocalGame();

    m_gvm = new GameViewModel(this);
    m_board = new GameBoardWidget();

    auto* avm = m_gvm->actionVM();

    // View → ViewModel
    connect(m_board, &GameBoardWidget::playCardRequested,
            avm, &ActionViewModel::onPlayCardRequested);
    connect(m_board, &GameBoardWidget::respondCardRequested,
            avm, &ActionViewModel::onRespondCardRequested);
    connect(m_board, &GameBoardWidget::targetPlayerSelected,
            avm, &ActionViewModel::onTargetSelected);
    connect(m_board, &GameBoardWidget::discardCardRequested,
            m_gvm, &GameViewModel::onDiscardCardRequested);
    connect(m_board, &GameBoardWidget::endPlayRequested,
            m_gvm, &GameViewModel::onEndPlayRequested);
    connect(m_board, &GameBoardWidget::advanceRequested,
            m_gvm, &GameViewModel::onAdvanceRequested);
    connect(m_board, &GameBoardWidget::skipRequested,
            m_gvm, &GameViewModel::onSkipRequested);
    connect(m_board, &GameBoardWidget::skillRequested,
            avm, &ActionViewModel::onSkillRequested);

    // ViewModel → View
    connect(m_gvm, &GameViewModel::phaseChanged,
            m_board, &GameBoardWidget::onPhaseChanged);
    connect(m_gvm, &GameViewModel::playerDataUpdated,
            m_board, &GameBoardWidget::onPlayerDataUpdated);
    connect(m_gvm, &GameViewModel::handCardsUpdated,
            m_board, &GameBoardWidget::onHandCardsUpdated);
    connect(avm, &ActionViewModel::targetSelectionStarted,
            m_board, &GameBoardWidget::onTargetSelectionStarted);
    connect(avm, &ActionViewModel::targetSelectionFinished,
            m_board, &GameBoardWidget::onTargetSelectionFinished);
    connect(m_gvm, &GameViewModel::pendingActionCreated,
            m_board, &GameBoardWidget::onPendingActionCreated);
    connect(m_gvm, &GameViewModel::pendingActionCleared,
            m_board, &GameBoardWidget::onPendingActionCleared);
    connect(m_gvm, &GameViewModel::gameOver,
            m_board, &GameBoardWidget::onGameOver);
    connect(m_gvm, &GameViewModel::logMessage,
            m_board, &GameBoardWidget::onLogMessage);
    connect(m_gvm, &GameViewModel::judgmentPerformed,
            m_board, &GameBoardWidget::onJudgmentPerformed);

    // 生命周期
    connect(m_board, &GameBoardWidget::gameFinished,
            this, &SGSApp::onGameFinished);

    m_gvm->startGame(charId1, charId2);
    static_cast<MainWindow*>(m_mainWindow)->showGamePage(m_board);
}

void SGSApp::releaseLocalGame()
{
    if (m_board) { m_board->deleteLater(); m_board = nullptr; }
    if (m_gvm)   { m_gvm->deleteLater();   m_gvm = nullptr; }
}

void SGSApp::onGameFinished()
{
    releaseLocalGame();
    static_cast<MainWindow*>(m_mainWindow)->showSelectionPage();
}

// ==================== 联网模式 ====================

namespace {

/// 收集所有非回环 IPv4 地址（可能有多个网卡：Wi-Fi、有线、虚拟网卡等）
QStringList localIpAddresses()
{
    QStringList ips;
    for (const QHostAddress& addr : QNetworkInterface::allAddresses()) {
        if (addr != QHostAddress::LocalHost
            && addr.protocol() == QAbstractSocket::IPv4Protocol) {
            ips.append(addr.toString());
        }
    }
    if (ips.isEmpty())
        ips.append(QStringLiteral("127.0.0.1"));
    return ips;
}

} // namespace

void SGSApp::onCreateRoom()
{
    releaseNetworkGame();

    // 1. 启动服务器（监听 9527，所有网络接口 — 局域网其他机器可连接）
    m_serverApp = new ServerApp(this);
    if (!m_serverApp->listen(Protocol::kDefaultPort, /*localOnly=*/false)) {
        static_cast<MainWindow*>(m_mainWindow)->resetNetworkState();
        QMessageBox::warning(static_cast<QWidget*>(m_mainWindow),
            QStringLiteral("创建房间失败"),
            QStringLiteral("无法监听端口 %1，请检查是否有其他实例在运行。")
                .arg(Protocol::kDefaultPort));
        m_serverApp->deleteLater();
        m_serverApp = nullptr;
        return;
    }

    // 2. 显示本机所有可用 IP（连接成功后才切换到选将页）
    QStringList ips = localIpAddresses();
    QString ipText = ips.join(QStringLiteral(", "));
    static_cast<MainWindow*>(m_mainWindow)->setNetworkStatusText(
        QStringLiteral("本机 IP: %1   端口: %2\n等待对手连接...")
            .arg(ipText).arg(Protocol::kDefaultPort));

    // 3. 创建本地客户端并连接服务器
    m_clientApp = new ClientApp(this);
    connect(m_clientApp->gameClient(), &GameClient::connected,
            this, [this, ipText](int playerId) {
        m_myPlayerId = playerId;
        m_clientApp->boardWidget()->setLocalPlayerId(playerId);
        // TCP 连接完成后再显示选将页，确保 SelectCharacter 不会排在 Handshake 之前
        static_cast<MainWindow*>(m_mainWindow)->showCharacterSelection(
            playerId, QStringLiteral("房主"),
            QStringLiteral("本机 IP: %1   端口: %2\n等待对手连接...")
                .arg(ipText).arg(Protocol::kDefaultPort));
    });
    connect(m_clientApp->gameClient(), &GameClient::gameStarted,
            this, &SGSApp::onClientGameStarted);
    connect(m_clientApp->gameClient(), &GameClient::connectionError,
            this, &SGSApp::onConnectionError);
    connect(m_clientApp->gameClient(), &GameClient::disconnected,
            this, [this]() { onConnectionError(QStringLiteral("与服务器断开连接")); });

    m_clientApp->connectToServer(QStringLiteral("127.0.0.1"), Protocol::kDefaultPort);
}

void SGSApp::onJoinRoom(const QString& host, quint16 port)
{
    releaseNetworkGame();

    // 1. 创建客户端连接远程服务器
    m_clientApp = new ClientApp(this);
    connect(m_clientApp->gameClient(), &GameClient::connected,
            this, [this](int playerId) {
        m_myPlayerId = playerId;
        m_clientApp->boardWidget()->setLocalPlayerId(playerId);
        // 连接成功后再显示选将页（确保 TCP 连接已完成，SelectCharacter 不会排在 Handshake 之前）
        static_cast<MainWindow*>(m_mainWindow)->showCharacterSelection(
            playerId, QStringLiteral("加入者"));
    });
    connect(m_clientApp->gameClient(), &GameClient::gameStarted,
            this, &SGSApp::onClientGameStarted);
    connect(m_clientApp->gameClient(), &GameClient::connectionError,
            this, &SGSApp::onConnectionError);
    connect(m_clientApp->gameClient(), &GameClient::connectionRejected,
            this, [this](const QString& reason) {
        onConnectionError(QStringLiteral("连接被拒绝: ") + reason);
    });
    connect(m_clientApp->gameClient(), &GameClient::disconnected,
            this, [this]() { onConnectionError(QStringLiteral("与服务器断开连接")); });

    m_clientApp->connectToServer(host, port);
}

void SGSApp::onCharacterConfirmed(int charId)
{
    if (!m_clientApp) return;

    // 发送选将消息到服务器
    m_clientApp->selectCharacter(charId);

    // 显示等待对手界面
    static_cast<MainWindow*>(m_mainWindow)->showWaitingForOpponent();
}

void SGSApp::onClientGameStarted(int /*charId0*/, int /*charId1*/)
{
    if (!m_clientApp) return;

    // 游戏已开始，用 ClientApp 的 GameBoardWidget 替换当前页面
    auto* mainWin = static_cast<MainWindow*>(m_mainWindow);
    mainWin->showGamePage(m_clientApp->boardWidget());

    // 联网对局结束信号
    connect(m_clientApp->gameClient(), &GameClient::gameOver,
            this, &SGSApp::onNetworkGameOver);
}

void SGSApp::onConnectionError(const QString& error)
{
    auto* mainWin = static_cast<MainWindow*>(m_mainWindow);
    mainWin->resetNetworkState();
    mainWin->showSelectionPage();

    // 清理网络对象
    releaseNetworkGame();

    QMessageBox::warning(static_cast<QWidget*>(m_mainWindow),
        QStringLiteral("连接错误"), error);
}

void SGSApp::onNetworkGameOver(int winnerId)
{
    if (m_clientApp) {
        // 断开连接（但保留 board 以便玩家看到结局画面）
        m_clientApp->gameClient()->disconnectFromServer();
    }

    // 清理网络对象
    releaseNetworkGame();

    auto* mainWin = static_cast<MainWindow*>(m_mainWindow);
    mainWin->showSelectionPage();
}

void SGSApp::releaseNetworkGame()
{
    releaseClient();
    if (m_serverApp) {
        m_serverApp->deleteLater();
        m_serverApp = nullptr;
    }
}

void SGSApp::releaseClient()
{
    if (m_clientApp) {
        // 断开网络连接
        m_clientApp->gameClient()->disconnectFromServer();
        m_clientApp->deleteLater();
        m_clientApp = nullptr;
    }
    m_myPlayerId = -1;
}
