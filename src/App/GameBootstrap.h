#ifndef GAMEBOOTSTRAP_H
#define GAMEBOOTSTRAP_H

#include <QObject>
#include <QVector>
#include <QWidget>

class GameViewModel;
class GameBoardWidget;
class GameState;

/// 组合根 — 唯一知道 ViewModel 和 View 具体类型的模块
/// 负责连接 View 信号 ↔ ViewModel 方法，以及 ViewModel 信号 ↔ View 槽
class GameBootstrap : public QObject {
    Q_OBJECT
public:
    explicit GameBootstrap(QWidget* boardParent, QObject* parent = nullptr);

    void startLocalGame(int charId1, int charId2);
    GameBoardWidget* boardWidget() const { return m_board; }

signals:
    void gameFinished();

private slots:
    // View → ViewModel 路由
    void onPlayCardRequested(int cardId, int playerId);
    void onRespondCardRequested(int cardId, int responderId);
    void onDiscardCardRequested(int cardId, int playerId);
    void onTargetSelected(int playerIndex);
    void onEndPlayRequested();
    void onAdvanceRequested();
    void onSkipRequested();

private:
    void wireAll();

    GameViewModel*   m_gvm = nullptr;
    GameBoardWidget* m_board = nullptr;
    QWidget*         m_boardParent = nullptr;

    // 目标选择暂存
    int m_pendingCardId = -1;
    int m_pendingUserId = -1;
    QVector<int> m_pendingTargetIds;
};

#endif // GAMEBOOTSTRAP_H
