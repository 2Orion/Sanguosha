#include "PlayerViewModel.h"
#include "Player.h"
#include "Character.h"

PlayerViewModel::PlayerViewModel(Player* player, QObject* parent)
    : QObject(parent)
    , m_player(player)
{
    if (!m_player) return;

    // 直接连接 Model 的 Qt 信号 → 转发为本 VM 的信号
    connect(m_player, &Player::hpChanged, this, &PlayerViewModel::hpChanged);
    connect(m_player, &Player::maxHpChanged, this, &PlayerViewModel::maxHpChanged);
    connect(m_player, &Player::dying, this, &PlayerViewModel::dying);
    connect(m_player, &Player::died, this, &PlayerViewModel::died);
    connect(m_player, &Player::revived, this, &PlayerViewModel::revived);
    connect(m_player, &Player::handCardAdded, this, &PlayerViewModel::handCardAdded);
    connect(m_player, &Player::handCardRemoved, this, &PlayerViewModel::handCardRemoved);
    connect(m_player, &Player::handCardsChanged, this, &PlayerViewModel::handCardsChanged);
    connect(m_player, &Player::stateChanged, this, &PlayerViewModel::stateChanged);
}

PlayerViewModel::~PlayerViewModel()
{
    // Qt 信号槽在 QObject 析构时自动断开，无需手动管理
}

int PlayerViewModel::playerId() const { return m_player ? m_player->playerId() : -1; }
QString PlayerViewModel::displayName() const { return m_player ? m_player->displayName() : QString(); }
QString PlayerViewModel::characterName() const { return m_player ? m_player->characterName() : QString(); }

QString PlayerViewModel::skillName() const
{
    if (m_player && m_player->character()) {
        return QString::fromStdString(m_player->character()->skillName());
    }
    return QString();
}

QString PlayerViewModel::skillDescription() const
{
    if (m_player && m_player->character()) {
        return QString::fromStdString(m_player->character()->skillDescription());
    }
    return QString();
}

int PlayerViewModel::hp() const { return m_player ? m_player->hp() : 0; }
int PlayerViewModel::maxHp() const { return m_player ? m_player->maxHp() : 0; }
double PlayerViewModel::hpRatio() const { return m_player ? m_player->hpRatio() : 0.0; }
bool PlayerViewModel::isAlive() const { return m_player ? m_player->isAlive() : false; }
bool PlayerViewModel::isDying() const { return m_player ? m_player->isDying() : false; }
bool PlayerViewModel::isFullHp() const { return m_player ? m_player->isFullHp() : false; }
int PlayerViewModel::handCardCount() const { return m_player ? m_player->handCardCount() : 0; }
bool PlayerViewModel::hasHandCards() const { return m_player ? m_player->hasHandCards() : false; }
int PlayerViewModel::handCardLimit() const { return m_player ? m_player->handCardLimit() : 0; }
bool PlayerViewModel::hasUsedKillThisTurn() const { return m_player ? m_player->hasUsedKillThisTurn() : false; }
bool PlayerViewModel::isWineEnhanced() const { return m_player ? m_player->isWineEnhanced() : false; }

int PlayerViewModel::equipCount() const
{
    return m_player ? static_cast<int>(m_player->equipCards().size()) : 0;
}

int PlayerViewModel::judgmentCount() const
{
    return m_player ? static_cast<int>(m_player->judgmentCards().size()) : 0;
}

Player* PlayerViewModel::player() const { return m_player; }
