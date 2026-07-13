#include "CardViewModel.h"
#include "Card.h"

CardViewModel::CardViewModel(Card* card, QObject* parent)
    : QObject(parent)
    , m_card(card)
{
}

int CardViewModel::id() const { return m_card ? m_card->id() : -1; }
CardType CardViewModel::cardType() const { return m_card ? m_card->cardType() : CardType::Kill; }
CardSuit CardViewModel::suit() const { return m_card ? m_card->suit() : CardSuit::Spade; }
int CardViewModel::number() const { return m_card ? m_card->number() : 0; }
QString CardViewModel::cardName() const { return m_card ? QString::fromStdString(m_card->cardName()) : QString(); }
QString CardViewModel::description() const { return m_card ? QString::fromStdString(m_card->description()) : QString(); }
CardColor CardViewModel::color() const { return m_card ? m_card->color() : CardColor::Black; }
bool CardViewModel::isBasic() const { return m_card ? m_card->isBasic() : false; }
bool CardViewModel::isStrategy() const { return m_card ? m_card->isStrategy() : false; }
QString CardViewModel::suitSymbol() const { return m_card ? QString::fromStdString(m_card->suitSymbol()) : QString(); }
QString CardViewModel::numberString() const { return m_card ? QString::fromStdString(m_card->numberString()) : QString(); }
Card* CardViewModel::card() const { return m_card; }

// ==================== UI 状态 ====================

bool CardViewModel::isSelected() const { return m_selected; }

void CardViewModel::setSelected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        emit selectedChanged(selected);
    }
}

bool CardViewModel::isPlayable() const { return m_playable; }

void CardViewModel::setPlayable(bool playable)
{
    if (m_playable != playable) {
        m_playable = playable;
        emit playableChanged(playable);
    }
}

bool CardViewModel::isHighlighted() const { return m_highlighted; }

void CardViewModel::setHighlighted(bool highlighted)
{
    if (m_highlighted != highlighted) {
        m_highlighted = highlighted;
        emit highlightedChanged(highlighted);
    }
}

void CardViewModel::resetUIState()
{
    setSelected(false);
    setPlayable(false);
    setHighlighted(false);
}

QString CardViewModel::cardTypeName(CardType type)
{
    return QString::fromStdString(Card::cardTypeName(type));
}
