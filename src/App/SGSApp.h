#ifndef SGSAPP_H
#define SGSAPP_H

#include <QObject>
#include <QMainWindow>

class GameViewModel;
class GameBoardWidget;

/// 组合根 — 创建并持有所有顶层对象，绑定 View ↔ ViewModel 信号
class SGSApp : public QObject {
    Q_OBJECT
public:
    explicit SGSApp(QObject* parent = nullptr);

    /// 主窗口（main.cpp 只需 show()，不需要知道 MainWindow 的具体类型）
    QMainWindow* mainWindow() const { return m_mainWindow; }

    /// 开始一局新游戏
    void startLocalGame(int charId1, int charId2);

private slots:
    void onGameFinished();

private:
    QMainWindow*      m_mainWindow = nullptr;
    GameViewModel*    m_gvm = nullptr;
    GameBoardWidget*  m_board = nullptr;
};

#endif // SGSAPP_H
