#ifndef CARDMANAGER_H
#define CARDMANAGER_H

#include <vector>
#include "CommonTypes.h"
#include "Event.h"

class Card;

class CardManager {
public:
    CardManager();
    ~CardManager();

    // ==================== 初始化与洗牌 ====================

    /// 创建所有卡牌并洗牌（简化版牌堆，共约 48 张）
    /// 杀×15 闪×10 桃×6 酒×3
    /// 过河拆桥×3 顺手牵羊×3 无中生有×3
    /// 南蛮入侵×2 万箭齐发×2 桃园结义×1
    void initialize();

    /// 洗牌（Fisher-Yates）
    void shuffle();

    // ==================== 摸牌 ====================

    /// 摸一张（牌堆空时自动回收弃牌堆）
    Card* drawCard();

    /// 摸多张
    std::vector<Card*> drawCards(int count);

    /// 剩余张数
    int remainingCount() const;

    /// 总张数
    int totalCardCount() const;

    // ==================== 弃牌 ====================

    void discard(Card* card);
    void discardMultiple(const std::vector<Card*>& cards);
    int discardPileCount() const;

    // ==================== 回收 ====================

    void reshuffleDiscardPile();

    // ==================== 查找 ====================

    Card* findCardById(int id) const;

    // ==================== 事件通知 ====================
    EventListener<> drawPileEmpty;
    EventListener<> reshuffled;
    EventListener<Card*> cardDiscarded;

    // just for test
    const std::vector<Card*>& getDrawPile() const {
        return this->m_drawPile;
    }

private:
    Card* createCard(CardType type, CardSuit suit, int number);
    CardSuit randomSuit() const;

    std::vector<Card*> m_drawPile;
    std::vector<Card*> m_discardPile;
    std::vector<Card*> m_allCards;  // 全量卡牌（管理生命周期）
    int m_nextCardId = 1;
};

#endif // CARDMANAGER_H
