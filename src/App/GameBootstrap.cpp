#include "GameBootstrap.h"
#include "GameViewModel.h"
#include "GameBoardWidget.h"

GameBootstrap::GameBootstrap(QWidget* boardParent, QObject* parent)
    : QObject(parent)
    , m_boardParent(boardParent)
{
}

GameBootstrap::~GameBootstrap()
{
    // GameVM 和 GameBoard 继承 QObject，由 Qt 父子关系自动清理
}

void GameBootstrap::startLocalGame(int charId1, int charId2)
{
    // 1. 创建 ViewModel（Qt 父子关系管理生命期）
    m_gvm = new GameViewModel(this);

    // 2. 创建 View（GameBoardWidget 内部通过 connectViewModel 建立信号连接）
    m_board = new GameBoardWidget(m_gvm, m_boardParent);

    // 3. 转发游戏结束信号
    m_board->connect(m_board, &GameBoardWidget::gameFinished,
                     this, &GameBootstrap::gameFinished);

    // 4. 启动游戏
    m_gvm->startGame(charId1, charId2);
}
