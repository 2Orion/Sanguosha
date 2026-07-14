#include "CardManager.h"
#include "Card.h"
#include <QRandomGenerator>
#include <algorithm>

CardManager::CardManager(QObject* parent)
    : QObject(parent)
{
}

CardManager::~CardManager()
{
    for (Card* card : m_allCards) delete card;
    m_allCards.clear();
}

void CardManager::initialize()
{
    for (Card* card : m_allCards) delete card;
    m_allCards.clear();
    m_drawPile.clear();
    m_discardPile.clear();

    auto addCards = [&](CardType type, int count) {
        for (int i = 0; i < count; ++i) {
            Card* card = createCard(type, randomSuit(), (i % 13) + 1);
            m_allCards.push_back(card);
            m_drawPile.push_back(card);
        }
    };

    addCards(CardType::Kill, 15);
    addCards(CardType::Dodge, 10);
    addCards(CardType::Peach, 6);
    addCards(CardType::Wine, 3);
    addCards(CardType::Dismantle, 3);
    addCards(CardType::Steal, 3);
    addCards(CardType::Bountiful, 3);
    addCards(CardType::BarbarianInvasion, 2);
    addCards(CardType::Volley, 2);

    Card* peachGarden = createCard(CardType::PeachGarden, randomSuit(), 1);
    m_allCards.push_back(peachGarden);
    m_drawPile.push_back(peachGarden);

    shuffle();
}

void CardManager::shuffle()
{
    int n = static_cast<int>(m_drawPile.size());
    for (int i = n - 1; i > 0; --i) {
        int j = QRandomGenerator::global()->bounded(i + 1);
        std::swap(m_drawPile[i], m_drawPile[j]);
    }
}

Card* CardManager::drawCard()
{
    if (m_drawPile.empty()) {
        reshuffleDiscardPile();
        if (m_drawPile.empty()) {
            emit drawPileEmpty();
            return nullptr;
        }
    }
    Card* card = m_drawPile.back();
    m_drawPile.pop_back();
    return card;
}

std::vector<Card*> CardManager::drawCards(int count)
{
    std::vector<Card*> cards;
    for (int i = 0; i < count; ++i) {
        Card* card = drawCard();
        if (card) cards.push_back(card);
        else break;
    }
    return cards;
}

int CardManager::remainingCount() const { return static_cast<int>(m_drawPile.size()); }

int CardManager::totalCardCount() const { return static_cast<int>(m_allCards.size()); }

void CardManager::discard(Card* card)
{
    if (card && std::find(m_discardPile.begin(), m_discardPile.end(), card) == m_discardPile.end()) {
        m_discardPile.push_back(card);
        emit cardDiscarded(card->id());
    }
}

void CardManager::discardMultiple(const std::vector<Card*>& cards)
{
    for (Card* card : cards) discard(card);
}

int CardManager::discardPileCount() const { return static_cast<int>(m_discardPile.size()); }

void CardManager::reshuffleDiscardPile()
{
    if (m_discardPile.empty()) return;
    m_drawPile.insert(m_drawPile.end(), m_discardPile.begin(), m_discardPile.end());
    m_discardPile.clear();
    shuffle();
    emit reshuffled();
}

Card* CardManager::findCardById(int id) const
{
    for (Card* card : m_allCards) {
        if (card && card->id() == id) return card;
    }
    return nullptr;
}

Card* CardManager::createCard(CardType type, CardSuit suit, int number)
{
    switch (type) {
    case CardType::Kill:               return new KillCard(suit, number);
    case CardType::Dodge:              return new DodgeCard(suit, number);
    case CardType::Peach:              return new PeachCard(suit, number);
    case CardType::Wine:               return new WineCard(suit, number);
    case CardType::Dismantle:          return new DismantleCard(suit, number);
    case CardType::Steal:              return new StealCard(suit, number);
    case CardType::Bountiful:          return new BountifulCard(suit, number);
    case CardType::BarbarianInvasion:  return new BarbarianCard(suit, number);
    case CardType::Volley:             return new VolleyCard(suit, number);
    case CardType::PeachGarden:        return new PeachGardenCard(suit, number);
    default: break;
    }
    return nullptr;
}

CardSuit CardManager::randomSuit() const
{
    return static_cast<CardSuit>(QRandomGenerator::global()->bounded(4));
}
