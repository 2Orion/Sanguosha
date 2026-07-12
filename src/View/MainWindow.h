#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QButtonGroup>
#include <QLabel>
#include <QGroupBox>
#include <memory>

class GameViewModel;
class GameBoardWidget;

/// 主窗口 — 选将界面 ↔ 游戏界面切换
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onStartGame();
    void onGameFinished();
    void onP1CharChanged(int id);
    void onP2CharChanged(int id);

private:
    void setupSelectionPage();
    void setupGamePage();

    /// 更新开始按钮状态（同武将不可重复选）
    void updateStartButton();

    // ---- 页面 ----
    QStackedWidget* m_centralStack;

    // ---- 选将界面 ----
    QWidget*    m_selectionPage;
    QButtonGroup* m_p1Group;
    QButtonGroup* m_p2Group;
    QPushButton*  m_startBtn;
    QLabel*       m_p1SelectedLabel;
    QLabel*       m_p2SelectedLabel;

    // ---- 游戏界面 ----
    GameBoardWidget* m_gameBoard;
    GameViewModel*   m_gvm;  // 手动管理生命周期
};

#endif // MAINWINDOW_H
