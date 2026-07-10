#include "PlayerViewModel.h"
#include "Player.h"
#include "Character.h"
#include "Card.h"

PlayerViewModel::PlayerViewModel(Player* player)
    : m_player(player)
{
    connectModelEvents();
}

PlayerViewModel::~PlayerViewModel()
{
    // 断开 Model 层连接，避免悬空回调
    if (m_player) {
        m_player->hpChanged.disconnect(m_conn.hpChangedId);
        m_player->maxHpChanged.disconnect(m_conn.maxHpChangedId);
        m_player->dying.disconnect(m_conn.dyingId);
        m_player->died.disconnect(m_conn.diedId);
        m_player->revived.disconnect(m_conn.revivedId);
        m_player->handCardAdded.disconnect(m_conn.handCardAddedId);
        m_player->handCardRemoved.disconnect(m_conn.handCardRemovedId);
        m_player->handCardsChanged.disconnect(m_conn.handCardsChangedId);
        m_player->stateChanged.disconnect(m_conn.stateChangedId);
    }

    // 清空自身监听者
    hpChanged.clear();
    maxHpChanged.clear();
    dying.clear();
    died.clear();
    revived.clear();
    handCardAdded.clear();
    handCardRemoved.clear();
    handCardsChanged.clear();
    stateChanged.clear();
}

// ==================== 身份标识 ====================

int PlayerViewModel::playerId() const
{
    return m_player ? m_player->playerId() : -1;
}

std::string PlayerViewModel::displayName() const
{
    return m_player ? m_player->displayName() : std::string();
}

std::string PlayerViewModel::characterName() const
{
    return m_player ? m_player->characterName() : std::string();
}

std::string PlayerViewModel::skillName() const
{
    if (m_player && m_player->character()) {
        return m_player->character()->skillName();
    }
    return std::string();
}

std::string PlayerViewModel::skillDescription() const
{
    if (m_player && m_player->character()) {
        return m_player->character()->skillDescription();
    }
    return std::string();
}

// ==================== 体力 ====================

int PlayerViewModel::hp() const
{
    return m_player ? m_player->hp() : 0;
}

int PlayerViewModel::maxHp() const
{
    return m_player ? m_player->maxHp() : 0;
}

double PlayerViewModel::hpRatio() const
{
    return m_player ? m_player->hpRatio() : 0.0;
}

bool PlayerViewModel::isAlive() const
{
    return m_player ? m_player->isAlive() : false;
}

bool PlayerViewModel::isDying() const
{
    return m_player ? m_player->isDying() : false;
}

bool PlayerViewModel::isFullHp() const
{
    return m_player ? m_player->isFullHp() : false;
}

// ==================== 手牌 ====================

int PlayerViewModel::handCardCount() const
{
    return m_player ? m_player->handCardCount() : 0;
}

const std::vector<Card*>& PlayerViewModel::handCards() const
{
    static std::vector<Card*> empty;
    return m_player ? m_player->handCards() : empty;
}

bool PlayerViewModel::hasHandCards() const
{
    return m_player ? m_player->hasHandCards() : false;
}

int PlayerViewModel::handCardLimit() const
{
    return m_player ? m_player->handCardLimit() : 0;
}

// ==================== 回合状态 ====================

bool PlayerViewModel::hasUsedKillThisTurn() const
{
    return m_player ? m_player->hasUsedKillThisTurn() : false;
}

bool PlayerViewModel::isWineEnhanced() const
{
    return m_player ? m_player->isWineEnhanced() : false;
}

// ==================== 装备/判定区（预留） ====================

int PlayerViewModel::equipCount() const
{
    return m_player ? static_cast<int>(m_player->equipCards().size()) : 0;
}

int PlayerViewModel::judgmentCount() const
{
    return m_player ? static_cast<int>(m_player->judgmentCards().size()) : 0;
}

// ==================== 原始指针访问 ====================

Player* PlayerViewModel::player() const
{
    return m_player;
}

// ==================== 事件转发 ====================

void PlayerViewModel::connectModelEvents()
{
    if (!m_player) return;

    m_conn.hpChangedId = m_player->hpChanged.connect(
        [this](int hp) { this->hpChanged.emit(hp); });

    m_conn.maxHpChangedId = m_player->maxHpChanged.connect(
        [this](int maxHp) { this->maxHpChanged.emit(maxHp); });

    m_conn.dyingId = m_player->dying.connect(
        [this](Player* p) { this->dying.emit(p); });

    m_conn.diedId = m_player->died.connect(
        [this](Player* p) { this->died.emit(p); });

    m_conn.revivedId = m_player->revived.connect(
        [this](Player* p) { this->revived.emit(p); });

    m_conn.handCardAddedId = m_player->handCardAdded.connect(
        [this](Card* c) { this->handCardAdded.emit(c); });

    m_conn.handCardRemovedId = m_player->handCardRemoved.connect(
        [this](Card* c) { this->handCardRemoved.emit(c); });

    m_conn.handCardsChangedId = m_player->handCardsChanged.connect(
        [this]() { this->handCardsChanged.emit(); });

    m_conn.stateChangedId = m_player->stateChanged.connect(
        [this]() { this->stateChanged.emit(); });
}
