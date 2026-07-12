#include "Player.h"
#include "Character.h"
#include "Card.h"
#include "RandomUtils.h"
#include <algorithm>

Player::Player()
{
}

Player::~Player()
{
}

// ==================== 身份标识 ====================

int Player::playerId() const
{
    return m_playerId;
}

void Player::setPlayerId(int id)
{
    m_playerId = id;
}

std::string Player::displayName() const
{
    return m_displayName;
}

void Player::setDisplayName(const std::string& name)
{
    m_displayName = name;
}

// ==================== 武将管理 ====================

void Player::setCharacter(Character* character)
{
    m_character = character;
    if (m_character) {
        m_hp = m_character->maxHp();
        maxHpChanged.notify(m_character->maxHp());
        hpChanged.notify(m_hp);
    }
    characterChanged.notify(character);
}

Character* Player::character() const
{
    return m_character;
}

std::string Player::characterName() const
{
    return m_character ? m_character->characterName() : std::string();
}

bool Player::hasCharacter() const
{
    return m_character != nullptr;
}

// ==================== 体力管理 ====================

int Player::hp() const
{
    return m_hp;
}

void Player::setHp(int value)
{
    int oldHp = m_hp;
    m_hp = std::max(0, value);
    if (m_hp != oldHp) {
        hpChanged.notify(m_hp);
        stateChanged.notify();
    }
}

int Player::maxHp() const
{
    return m_character ? m_character->maxHp() : 0;
}

void Player::damage(int value)
{
    if (value > 0) {
        setHp(m_hp - value);
    }
}

void Player::heal(int value)
{
    if (value > 0) {
        int maxHpValue = maxHp();
        int newHp = std::min(m_hp + value, maxHpValue);
        setHp(newHp);

        if (m_dying && m_hp > 0) {
            m_dying = false;
            revived.notify(this);
        }
    }
}

bool Player::isAlive() const
{
    return m_hp > 0;
}

bool Player::isDying() const
{
    return m_dying;
}

void Player::setDying(bool dying)
{
    if (m_dying != dying) {
        m_dying = dying;
        if (dying) {
            this->dying.notify(this);
        } else if (m_hp > 0) {
            revived.notify(this);
        }
    }
}

bool Player::isFullHp() const
{
    return m_hp >= maxHp();
}

double Player::hpRatio() const
{
    int maxHpValue = maxHp();
    return maxHpValue > 0 ? static_cast<double>(m_hp) / maxHpValue : 0.0;
}

int Player::handCardLimit() const
{
    return std::max(0, m_hp);
}

// ==================== 手牌管理 ====================

const std::vector<Card*>& Player::handCards() const
{
    return m_handCards;
}

int Player::handCardCount() const
{
    return static_cast<int>(m_handCards.size());
}

bool Player::hasCard(const Card* card) const
{
    return std::find(m_handCards.begin(), m_handCards.end(), const_cast<Card*>(card)) != m_handCards.end();
}

void Player::addHandCard(Card* card)
{
    if (card && std::find(m_handCards.begin(), m_handCards.end(), card) == m_handCards.end()) {
        m_handCards.push_back(card);
        handCardAdded.notify(card);
        handCardsChanged.notify();
        stateChanged.notify();
    }
}

void Player::removeHandCard(Card* card)
{
    if (!card) return;

    auto it = std::find(m_handCards.begin(), m_handCards.end(), card);
    if (it != m_handCards.end()) {
        m_handCards.erase(it);
        handCardRemoved.notify(card);
        handCardsChanged.notify();
        stateChanged.notify();
    }
}

bool Player::hasHandCards() const
{
    return !m_handCards.empty();
}

Card* Player::getRandomHandCard() const
{
    if (m_handCards.empty()) {
        return nullptr;
    }
    int index = RandomUtils::bounded(static_cast<int>(m_handCards.size()));
    return m_handCards[index];
}

// ==================== 装备区（预留） ====================

const std::vector<Card*>& Player::equipCards() const
{
    return m_equipCards;
}

bool Player::hasEquipCards() const
{
    return !m_equipCards.empty();
}

void Player::addEquipCard(Card* card)
{
    if (card && std::find(m_equipCards.begin(), m_equipCards.end(), card) == m_equipCards.end()) {
        m_equipCards.push_back(card);
        stateChanged.notify();
    }
}

void Player::removeEquipCard(Card* card)
{
    if (card) {
        auto it = std::find(m_equipCards.begin(), m_equipCards.end(), card);
        if (it != m_equipCards.end()) {
            m_equipCards.erase(it);
        }
        stateChanged.notify();
    }
}

// ==================== 判定区（预留） ====================

const std::vector<Card*>& Player::judgmentCards() const
{
    return m_judgmentCards;
}

bool Player::hasJudgmentCards() const
{
    return !m_judgmentCards.empty();
}

void Player::addJudgmentCard(Card* card)
{
    if (card && std::find(m_judgmentCards.begin(), m_judgmentCards.end(), card) == m_judgmentCards.end()) {
        m_judgmentCards.push_back(card);
        stateChanged.notify();
    }
}

void Player::removeJudgmentCard(Card* card)
{
    if (card) {
        auto it = std::find(m_judgmentCards.begin(), m_judgmentCards.end(), card);
        if (it != m_judgmentCards.end()) {
            m_judgmentCards.erase(it);
        }
        stateChanged.notify();
    }
}

// ==================== 回合状态 ====================

void Player::resetTurnState()
{
    m_usedKillThisTurn = false;
    m_wineEnhanced = false;
}

bool Player::hasUsedKillThisTurn() const
{
    return m_usedKillThisTurn;
}

void Player::setUsedKillThisTurn(bool used)
{
    m_usedKillThisTurn = used;
}

bool Player::isWineEnhanced() const
{
    return m_wineEnhanced;
}

void Player::setWineEnhanced(bool enhanced)
{
    m_wineEnhanced = enhanced;
}

// ==================== 工具 ====================

std::vector<Card*> Player::allSelectableCards() const
{
    std::vector<Card*> result;
    result.insert(result.end(), m_handCards.begin(), m_handCards.end());
    result.insert(result.end(), m_equipCards.begin(), m_equipCards.end());
    return result;
}
