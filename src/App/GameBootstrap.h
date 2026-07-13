#ifndef GAMEBOOTSTRAP_H
#define GAMEBOOTSTRAP_H

#include <QObject>
#include <QWidget>

class GameViewModel;
class GameBoardWidget;

/// 组合根（Composition Root）— 创建并连接所有 MVVM 层对象
/// 单一职责：将 GameViewModel ↔ GameBoardWidget 通过 Qt 信号槽连接
class GameBootstrap : public QObject {
    Q_OBJECT
public:
    explicit GameBootstrap(QWidget* boardParent = nullptr, QObject* parent = nullptr);
    ~GameBootstrap() override;

    /// 开始一局新游戏
    void startLocalGame(int charId1, int charId2);

    /// 获取游戏面板（用于插入到主窗口布局）
    GameBoardWidget* boardWidget() const { return m_board; }

    /// 获取 ViewModel（供 GameBoardWidget 构造等场景）
    GameViewModel* viewModel() const { return m_gvm; }

signals:
    /// 游戏结束信号（转发 GameBoardWidget::gameFinished）
    void gameFinished();

private:
    void wireViewModelToBoard();

    GameViewModel*   m_gvm = nullptr;
    GameBoardWidget* m_board = nullptr;
    QWidget*         m_boardParent = nullptr;
};

#endif // GAMEBOOTSTRAP_H
