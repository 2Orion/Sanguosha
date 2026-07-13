#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QButtonGroup>
#include <QLabel>
#include <QGroupBox>
#include <memory>

class GameBootstrap;
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
    void updateStartButton();

    QStackedWidget* m_centralStack;

    // 选将界面
    QWidget*      m_selectionPage;
    QButtonGroup* m_p1Group;
    QButtonGroup* m_p2Group;
    QPushButton*  m_startBtn;
    QLabel*       m_p1SelectedLabel;
    QLabel*       m_p2SelectedLabel;

    // 组合根（负责创建和管理 GameViewModel + GameBoardWidget）
    GameBootstrap*  m_bootstrap = nullptr;
    GameBoardWidget* m_gameBoard = nullptr;
};

#endif // MAINWINDOW_H
