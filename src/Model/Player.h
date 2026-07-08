#ifndef PLAYER_H
#define PLAYER_H

#include <vector>
#include <string>
#include "CommonTypes.h"
#include "Event.h"

class Card;
class Character;
class GameState;

class Player {
public:
    Player();
    ~Player();

    // ==================== 身份标识 ====================

    int playerId() const;
    void setPlayerId(int id);
    std::string displayName() const;
    void setDisplayName(const std::string& name);

    // ==================== 武将管理 ====================

    void setCharacter(Character* character);
    Character* character() const;
    std::string characterName() const;
    bool hasCharacter() const;

    // ==================== 体力管理 ====================

    int hp() const;
    void setHp(int value);
    int maxHp() const;           // 委托给 Character
    void damage(int value);
    void heal(int value);
    bool isAlive() const;
    bool isDying() const;
    void setDying(bool dying);
    bool isFullHp() const;
    double hpRatio() const;      // hp / maxHp（用于血条显示）
    int handCardLimit() const;   // = 当前体力值

    // ==================== 手牌管理 ====================

    const std::vector<Card*>& handCards() const;
    int handCardCount() const;
    bool hasCard(const Card* card) const;
    void addHandCard(Card* card);
    void removeHandCard(Card* card);
    bool hasHandCards() const;
    Card* getRandomHandCard() const;  // 随机获取一张手牌

    // ==================== 装备区（预留） ====================

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

    void resetTurnState();              // 回合开始时调用
    bool hasUsedKillThisTurn() const;
    void setUsedKillThisTurn(bool used);
    bool isWineEnhanced() const;
    void setWineEnhanced(bool enhanced);

    // ==================== 工具 ====================

    /// 获取所有可被选中的牌（手牌 + 装备区）
    std::vector<Card*> allSelectableCards() const;

    // ==================== 事件通知 ====================
    EventListener<int> hpChanged;
    EventListener<int> maxHpChanged;
    EventListener<Player*> dying;
    EventListener<Player*> died;
    EventListener<Player*> revived;       // 从濒死救回
    EventListener<Card*> handCardAdded;
    EventListener<Card*> handCardRemoved;
    EventListener<> handCardsChanged;          // 批量手牌变化
    EventListener<Character*> characterChanged;
    EventListener<> stateChanged;              // 通用刷新

private:
    int m_playerId = -1;
    std::string m_displayName;
    Character* m_character = nullptr;

    int m_hp = 0;
    bool m_dying = false;

    std::vector<Card*> m_handCards;
    std::vector<Card*> m_equipCards;
    std::vector<Card*> m_judgmentCards;

    bool m_usedKillThisTurn = false;
    bool m_wineEnhanced = false;
};

#endif // PLAYER_H
