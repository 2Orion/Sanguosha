#ifndef PLAYER_H
#define PLAYER_H

#include <vector>
#include <string>
#include <QObject>
#include "CommonTypes.h"

class Card;
class Character;
class GameState;

class Player : public QObject {
    Q_OBJECT
public:
    explicit Player(QObject* parent = nullptr);
    ~Player() override;

    // ==================== 身份标识 ====================

    int playerId() const;
    void setPlayerId(int id);
    QString displayName() const;
    void setDisplayName(const QString& name);

    // ==================== 武将管理 ====================

    void setCharacter(Character* character);
    Character* character() const;
    QString characterName() const;
    bool hasCharacter() const;

    // ==================== 体力管理 ====================

    int hp() const;
    void setHp(int value);
    int maxHp() const;
    void damage(int value);
    void heal(int value);
    bool isAlive() const;
    bool isDying() const;
    void setDying(bool dying);
    void markDead();
    bool isFullHp() const;
    double hpRatio() const;
    int handCardLimit() const;

    // ==================== 手牌管理 ====================

    const std::vector<Card*>& handCards() const;
    int handCardCount() const;
    bool hasCard(const Card* card) const;
    void addHandCard(Card* card);
    void removeHandCard(Card* card);
    bool hasHandCards() const;
    Card* getRandomHandCard() const;

    // ==================== 装备区 ====================

    const std::vector<Card*>& equipCards() const;
    bool hasEquipCards() const;
    void addEquipCard(Card* card);
    void removeEquipCard(Card* card);

    // ==================== 判定区（预留） ====================

    const std::vector<Card*>& judgmentCards() const;
    bool hasJudgmentCards() const;
    void addJudgmentCard(Card* card);
    void removeJudgmentCard(Card* card);

    // ==================== 回合状态 ====================

    void resetTurnState();
    bool hasUsedKillThisTurn() const;
    void setUsedKillThisTurn(bool used);
    bool isWineEnhanced() const;
    void setWineEnhanced(bool enhanced);

    // ==================== 工具 ====================

    std::vector<Card*> allSelectableCards() const;

signals:
    void hpChanged(int hp);
    void maxHpChanged(int maxHp);
    void dying(int playerId);
    void died(int playerId);
    void revived(int playerId);
    void handCardAdded(int cardId);
    void handCardRemoved(int cardId);
    void handCardsChanged();
    void characterChanged(const QString& charName);
    void stateChanged();

private:
    int m_playerId = -1;
    QString m_displayName;
    Character* m_character = nullptr;

    int m_hp = 0;
    bool m_dying = false;
    bool m_deathNotified = false;

    std::vector<Card*> m_handCards;
    std::vector<Card*> m_equipCards;
    std::vector<Card*> m_judgmentCards;

    bool m_usedKillThisTurn = false;
    bool m_wineEnhanced = false;
};

#endif // PLAYER_H
