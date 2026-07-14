#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QButtonGroup>
#include <QLabel>

class GameBoardWidget;

/// 主窗口 — 纯 UI，不持有任何 GameBootstrap/ViewModel 引用
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    /// 切换到游戏页面
    void showGamePage(GameBoardWidget* board);

    /// 切换到选将页面
    void showSelectionPage();

signals:
    /// 用户选择武将后点击开始对战
    void startGameRequested(int charId1, int charId2);

private slots:
    void onStartClicked();
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
};

#endif // MAINWINDOW_H
