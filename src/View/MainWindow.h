#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QButtonGroup>
#include <QLabel>
#include <QVBoxLayout>

class GameBoardWidget;

/// 主窗口 — 纯 UI，不持有任何 SGSApp/ViewModel 引用
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    /// 切换到游戏页面
    void showGamePage(GameBoardWidget* board);

    /// 切换到选将页面
    void showSelectionPage();

    /// 联网模式：显示单角色选将页面
    /// statusHint 可选的状态提示（如本机 IP），显示在页面顶部
    void showCharacterSelection(int playerId, const QString& playerName,
                                const QString& statusHint = QString());

    /// 联网模式：显示等待对手界面
    void showWaitingForOpponent();
    /// 联网模式：更新状态提示文字
    void setNetworkStatusText(const QString& text);

    /// 联网模式：恢复按钮可点击（连接失败后重置）
    void resetNetworkState();

signals:
    /// 用户选择武将后点击开始对战（本地模式）
    void startGameRequested(int charId1, int charId2);

    /// 用户点击「创建房间」
    void createRoomRequested();

    /// 用户输入服务器地址后点击「加入」
    void joinRoomRequested(const QString& host, quint16 port);

    /// 联网模式：用户确认选择了某个武将
    void characterConfirmed(int charId);

private slots:
    void onStartClicked();
    void onP1CharChanged(int id);
    void onP2CharChanged(int id);
    void onCreateRoomClicked();
    void onJoinRoomClicked();

private:
    void setupSelectionPage();
    void updateStartButton();
    void setupNetworkSection(QVBoxLayout* parent);

    QStackedWidget* m_centralStack;

    // 选将界面
    QWidget*      m_selectionPage;
    QButtonGroup* m_p1Group;
    QButtonGroup* m_p2Group;
    QPushButton*  m_startBtn;
    QLabel*       m_p1SelectedLabel;
    QLabel*       m_p2SelectedLabel;

    // 联网入口
    QPushButton* m_createRoomBtn;
    QPushButton* m_joinRoomBtn;
    QLabel*      m_networkStatusLabel;
};

#endif // MAINWINDOW_H
