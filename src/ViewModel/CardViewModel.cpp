#include "CardViewModel.h"
#include "Card.h"

CardViewModel::CardViewModel(Card* card)
    : m_card(card)
{
}

// ==================== Card 只读属性（透传） ====================

int CardViewModel::id() const
{
    return m_card ? m_card->id() : -1;
}

CardType CardViewModel::cardType() const
{
    return m_card ? m_card->cardType() : CardType::Kill;
}

CardSuit CardViewModel::suit() const
{
    return m_card ? m_card->suit() : CardSuit::Spade;
}

int CardViewModel::number() const
{
    return m_card ? m_card->number() : 0;
}

std::string CardViewModel::cardName() const
{
    return m_card ? m_card->cardName() : std::string();
}

std::string CardViewModel::description() const
{
    return m_card ? m_card->description() : std::string();
}

CardColor CardViewModel::color() const
{
    return m_card ? m_card->color() : CardColor::Black;
}

bool CardViewModel::isBasic() const
{
    return m_card ? m_card->isBasic() : false;
}

bool CardViewModel::isStrategy() const
{
    return m_card ? m_card->isStrategy() : false;
}

std::string CardViewModel::suitSymbol() const
{
    return m_card ? m_card->suitSymbol() : std::string();
}

std::string CardViewModel::numberString() const
{
    return m_card ? m_card->numberString() : std::string();
}

// ==================== UI 状态 ====================

bool CardViewModel::isSelected() const
{
    return m_selected;
}

void CardViewModel::setSelected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        selectedChanged.notify(selected);
    }
}

bool CardViewModel::isPlayable() const
{
    return m_playable;
}

void CardViewModel::setPlayable(bool playable)
{
    if (m_playable != playable) {
        m_playable = playable;
        playableChanged.notify(playable);
    }
}

bool CardViewModel::isHighlighted() const
{
    return m_highlighted;
}

void CardViewModel::setHighlighted(bool highlighted)
{
    if (m_highlighted != highlighted) {
        m_highlighted = highlighted;
        highlightedChanged.notify(highlighted);
    }
}

void CardViewModel::resetUIState()
{
    setSelected(false);
    setPlayable(false);
    setHighlighted(false);
}

Card* CardViewModel::card() const
{
    return m_card;
}
