#include "Player.h"
#include "Character.h"
#include "Card.h"
#include <QRandomGenerator>
#include <algorithm>

Player::Player(QObject* parent)
    : QObject(parent)
{
}

Player::~Player() = default;

// ==================== 身份标识 ====================

int Player::playerId() const { return m_playerId; }

void Player::setPlayerId(int id) { m_playerId = id; }

QString Player::displayName() const { return m_displayName; }

void Player::setDisplayName(const QString& name) { m_displayName = name; }

// ==================== 武将管理 ====================

void Player::setCharacter(Character* character)
{
    m_character = character;
    if (m_character) {
        m_hp = m_character->maxHp();
        emit maxHpChanged(m_character->maxHp());
        emit hpChanged(m_hp);
    }
    emit characterChanged(character ? QString::fromStdString(character->characterName()) : QString());
}

Character* Player::character() const { return m_character; }

QString Player::characterName() const
{
    return m_character ? QString::fromStdString(m_character->characterName()) : QString();
}

bool Player::hasCharacter() const { return m_character != nullptr; }

// ==================== 体力管理 ====================

int Player::hp() const { return m_hp; }

void Player::setHp(int value)
{
    int oldHp = m_hp;
    m_hp = std::max(0, value);
    if (m_hp > 0 && oldHp <= 0) {
        m_deathNotified = false;
    }
    if (m_hp != oldHp) {
        emit hpChanged(m_hp);
        emit stateChanged();
    }
}

int Player::maxHp() const { return m_character ? m_character->maxHp() : 0; }

void Player::damage(int value)
{
    if (value > 0) setHp(m_hp - value);
}

void Player::heal(int value)
{
    if (value > 0) {
        int maxHpValue = maxHp();
        int newHp = std::min(m_hp + value, maxHpValue);
        setHp(newHp);
        if (m_dying && m_hp > 0) {
            m_dying = false;
            emit revived(m_playerId);
        }
    }
}

bool Player::isAlive() const { return m_hp > 0; }
bool Player::isDying() const { return m_dying; }

void Player::setDying(bool dying_)
{
    if (m_dying != dying_) {
        m_dying = dying_;
        if (dying_) {
            emit dying(m_playerId);
        } else if (m_hp > 0) {
            emit revived(m_playerId);
        }
    }
}

void Player::markDead()
{
    if (m_hp > 0 || m_deathNotified) return;

    m_deathNotified = true;
    emit died(m_playerId);
}

bool Player::isFullHp() const { return m_hp >= maxHp(); }

double Player::hpRatio() const
{
    int maxHpValue = maxHp();
    return maxHpValue > 0 ? static_cast<double>(m_hp) / maxHpValue : 0.0;
}

int Player::handCardLimit() const { return std::max(0, m_hp); }

// ==================== 手牌管理 ====================

const std::vector<Card*>& Player::handCards() const { return m_handCards; }

int Player::handCardCount() const { return static_cast<int>(m_handCards.size()); }

bool Player::hasCard(const Card* card) const
{
    return std::find(m_handCards.begin(), m_handCards.end(), card) != m_handCards.end();
}

void Player::addHandCard(Card* card)
{
    if (card && std::find(m_handCards.begin(), m_handCards.end(), card) == m_handCards.end()) {
        m_handCards.push_back(card);
        emit handCardAdded(card->id());
        emit handCardsChanged();
        emit stateChanged();
    }
}

void Player::removeHandCard(Card* card)
{
    if (!card) return;
    auto it = std::find(m_handCards.begin(), m_handCards.end(), card);
    if (it != m_handCards.end()) {
        m_handCards.erase(it);
        emit handCardRemoved(card->id());
        emit handCardsChanged();
        emit stateChanged();
    }
}

bool Player::hasHandCards() const { return !m_handCards.empty(); }

Card* Player::getRandomHandCard() const
{
    if (m_handCards.empty()) return nullptr;
    int index = QRandomGenerator::global()->bounded(static_cast<int>(m_handCards.size()));
    return m_handCards[index];
}

// ==================== 装备区 ====================

void Player::rebuildEquipCardsVector() const
{
    m_equipCardsCache.clear();
    for (int i = 0; i < 4; ++i) {
        if (m_equipSlots[i]) {
            m_equipCardsCache.push_back(m_equipSlots[i]);
        }
    }
}

EquipmentCard* Player::equippedAt(EquipSlot slot) const
{
    int idx = static_cast<int>(slot);
    return (idx >= 0 && idx < 4) ? m_equipSlots[idx] : nullptr;
}

void Player::equipCard(EquipmentCard* card)
{
    if (!card) return;
    int idx = static_cast<int>(card->equipSlot());
    if (idx < 0 || idx >= 4) return;

    // 替换同槽旧装备
    EquipmentCard* old = m_equipSlots[idx];
    m_equipSlots[idx] = card;

    emit equipmentChanged(card->equipSlot());
    emit stateChanged();

    // 旧装备由调用方处理（进弃牌堆）
    (void)old;
}

void Player::unequipSlot(EquipSlot slot)
{
    int idx = static_cast<int>(slot);
    if (idx < 0 || idx >= 4) return;
    if (m_equipSlots[idx]) {
        m_equipSlots[idx] = nullptr;
        emit equipmentChanged(slot);
        emit stateChanged();
    }
}

int Player::attackRange() const
{
    int range = 1;  // 基础攻击距离
    EquipmentCard* weapon = equippedAt(EquipSlot::Weapon);
    if (weapon) {
        range += weapon->attackRangeBonus();
    }
    return range;
}

bool Player::hasArmor() const
{
    return equippedAt(EquipSlot::Armor) != nullptr;
}

bool Player::hasCrossbow() const
{
    EquipmentCard* weapon = equippedAt(EquipSlot::Weapon);
    return weapon && weapon->canExtraKill();
}

// 旧接口兼容

const std::vector<Card*>& Player::equipCards() const
{
    rebuildEquipCardsVector();
    return m_equipCardsCache;
}

bool Player::hasEquipCards() const
{
    for (int i = 0; i < 4; ++i) {
        if (m_equipSlots[i]) return true;
    }
    return false;
}

void Player::addEquipCard(Card* card)
{
    EquipmentCard* eq = dynamic_cast<EquipmentCard*>(card);
    if (eq) {
        equipCard(eq);
    }
}

void Player::removeEquipCard(Card* card)
{
    for (int i = 0; i < 4; ++i) {
        if (m_equipSlots[i] == card) {
            m_equipSlots[i] = nullptr;
            emit equipmentChanged(static_cast<EquipSlot>(i));
            emit stateChanged();
            return;
        }
    }
}

// ==================== 判定区 ====================

const std::vector<Card*>& Player::judgmentCards() const { return m_judgmentCards; }

bool Player::hasJudgmentCards() const { return !m_judgmentCards.empty(); }

void Player::addJudgmentCard(Card* card)
{
    if (card && std::find(m_judgmentCards.begin(), m_judgmentCards.end(), card) == m_judgmentCards.end()) {
        m_judgmentCards.push_back(card);
        emit stateChanged();
    }
}

void Player::removeJudgmentCard(Card* card)
{
    if (card) {
        auto it = std::find(m_judgmentCards.begin(), m_judgmentCards.end(), card);
        if (it != m_judgmentCards.end()) m_judgmentCards.erase(it);
        emit stateChanged();
    }
}

// ==================== 回合状态 ====================

void Player::resetTurnState()
{
    const bool changed = m_usedKillThisTurn || m_usedActiveSkillThisTurn || m_wineEnhanced;
    m_usedKillThisTurn = false;
    m_usedActiveSkillThisTurn = false;
    m_wineEnhanced = false;
    if (changed) emit stateChanged();
}

bool Player::hasUsedKillThisTurn() const { return m_usedKillThisTurn; }

void Player::setUsedKillThisTurn(bool used) { m_usedKillThisTurn = used; }

bool Player::hasUsedActiveSkillThisTurn() const { return m_usedActiveSkillThisTurn; }

void Player::setUsedActiveSkillThisTurn(bool used)
{
    if (m_usedActiveSkillThisTurn == used) return;
    m_usedActiveSkillThisTurn = used;
    emit stateChanged();
}

bool Player::isWineEnhanced() const { return m_wineEnhanced; }

void Player::setWineEnhanced(bool enhanced) { m_wineEnhanced = enhanced; }

// ==================== 工具 ====================

std::vector<Card*> Player::allSelectableCards() const
{
    std::vector<Card*> result;
    result.insert(result.end(), m_handCards.begin(), m_handCards.end());
    for (int i = 0; i < 4; ++i) {
        if (m_equipSlots[i]) {
            result.push_back(m_equipSlots[i]);
        }
    }
    return result;
}
