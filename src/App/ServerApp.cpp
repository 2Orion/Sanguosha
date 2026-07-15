#include "ServerApp.h"
#include "GameViewModel.h"
#include "ActionViewModel.h"

ServerApp::ServerApp(QObject* parent)
    : QObject(parent)
{
}

void ServerApp::startHeadlessGame(int charId1, int charId2)
{
    // 与 SGSApp::startLocalGame 一致：重复开局先释放上一局对象图
    if (m_gvm) {
        m_gvm->deleteLater();
        m_gvm = nullptr;
    }

    m_gvm = new GameViewModel(this);

    // 生命周期：对局结束后通知外部（GameServer 广播 GameOver 由 Step 4 接线）。
    // 这里不立即销毁 VM——最后一批状态推送（gameOver 前后的 playerDataUpdated）
    // 需要在事件循环中送达；VM 在下一次开局或 ServerApp 析构时释放。
    connect(m_gvm, &GameViewModel::gameOver,
            this, &ServerApp::onGameOver);

    m_gvm->startGame(charId1, charId2);
}

ActionViewModel* ServerApp::actionViewModel() const
{
    return m_gvm ? m_gvm->actionVM() : nullptr;
}

void ServerApp::onGameOver(int winnerId)
{
    emit gameFinished(winnerId);
}
