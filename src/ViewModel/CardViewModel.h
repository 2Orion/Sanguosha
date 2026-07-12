#ifndef CARDVIEWMODEL_H
#define CARDVIEWMODEL_H

#include <string>
#include "CommonTypes.h"
#include "Event.h"

class Card;

/// 卡牌 ViewModel — 在 Card 基础上增加 UI 展示状态
/// （是否选中、是否可打出、是否高亮）
class CardViewModel {
public:
    explicit CardViewModel(Card* card);

    // ==================== Card 只读属性（透传） ====================

    int id() const;
    CardType cardType() const;
    CardSuit suit() const;
    int number() const;
    std::string cardName() const;
    std::string description() const;
    CardColor color() const;
    bool isBasic() const;
    bool isStrategy() const;
    std::string suitSymbol() const;
    std::string numberString() const;

    // ==================== UI 状态 ====================

    bool isSelected() const;
    void setSelected(bool selected);

    bool isPlayable() const;
    void setPlayable(bool playable);

    bool isHighlighted() const;
    void setHighlighted(bool highlighted);

    /// 是否为技能转化牌（如关羽红牌当杀、赵云杀闪互转）
    bool isTransform() const;
    void setIsTransform(bool isTransform);

    // 重置所有 UI 状态
    void resetUIState();

    // ==================== 原始指针访问 ====================

    Card* card() const;

    // ==================== 事件 ====================

    EventListener<bool> selectedChanged;
    EventListener<bool> playableChanged;
    EventListener<bool> highlightedChanged;

private:
    Card* m_card;
    bool m_selected    = false;
    bool m_playable    = false;
    bool m_highlighted = false;
    bool m_isTransform = false;
};

#endif // CARDVIEWMODEL_H
