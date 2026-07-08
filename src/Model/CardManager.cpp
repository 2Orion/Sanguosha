#include "CardManager.h"
#include "Card.h"
#include "RandomUtils.h"
#include <algorithm>

CardManager::CardManager()
{
}

CardManager::~CardManager()
{
    for (Card* card : m_allCards) {
        delete card;
    }
    m_allCards.clear();
}

void CardManager::initialize()
{
    for (Card* card : m_allCards) {
        delete card;
    }
    m_allCards.clear();
    m_drawPile.clear();
    m_discardPile.clear();

    // 杀×15
    for (int i = 0; i < 15; ++i) {
        Card* card = createCard(CardType::Kill, randomSuit(), (i % 13) + 1);
        m_allCards.push_back(card);
        m_drawPile.push_back(card);
    }

    // 闪×10
    for (int i = 0; i < 10; ++i) {
        Card* card = createCard(CardType::Dodge, randomSuit(), (i % 13) + 1);
        m_allCards.push_back(card);
        m_drawPile.push_back(card);
    }

    // 桃×6
    for (int i = 0; i < 6; ++i) {
        Card* card = createCard(CardType::Peach, randomSuit(), (i % 13) + 1);
        m_allCards.push_back(card);
        m_drawPile.push_back(card);
    }

    // 酒×3
    for (int i = 0; i < 3; ++i) {
        Card* card = createCard(CardType::Wine, randomSuit(), (i % 13) + 1);
        m_allCards.push_back(card);
        m_drawPile.push_back(card);
    }

    // 过河拆桥×3
    for (int i = 0; i < 3; ++i) {
        Card* card = createCard(CardType::Dismantle, randomSuit(), (i % 13) + 1);
        m_allCards.push_back(card);
        m_drawPile.push_back(card);
    }

    // 顺手牵羊×3
    for (int i = 0; i < 3; ++i) {
        Card* card = createCard(CardType::Steal, randomSuit(), (i % 13) + 1);
        m_allCards.push_back(card);
        m_drawPile.push_back(card);
    }

    // 无中生有×3
    for (int i = 0; i < 3; ++i) {
        Card* card = createCard(CardType::Bountiful, randomSuit(), (i % 13) + 1);
        m_allCards.push_back(card);
        m_drawPile.push_back(card);
    }

    // 南蛮入侵×2
    for (int i = 0; i < 2; ++i) {
        Card* card = createCard(CardType::BarbarianInvasion, randomSuit(), (i % 13) + 1);
        m_allCards.push_back(card);
        m_drawPile.push_back(card);
    }

    // 万箭齐发×2
    for (int i = 0; i < 2; ++i) {
        Card* card = createCard(CardType::Volley, randomSuit(), (i % 13) + 1);
        m_allCards.push_back(card);
        m_drawPile.push_back(card);
    }

    // 桃园结义×1
    Card* card = createCard(CardType::PeachGarden, randomSuit(), 1);
    m_allCards.push_back(card);
    m_drawPile.push_back(card);

    shuffle();
}

void CardManager::shuffle()
{
    int n = static_cast<int>(m_drawPile.size());
    for (int i = n - 1; i > 0; --i) {
        int j = RandomUtils::bounded(i + 1);
        std::swap(m_drawPile[i], m_drawPile[j]);
    }
}

Card* CardManager::drawCard()
{
    if (m_drawPile.empty()) {
        reshuffleDiscardPile();
        if (m_drawPile.empty()) {
            drawPileEmpty.emit();
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
        if (card) {
            cards.push_back(card);
        } else {
            break;
        }
    }
    return cards;
}

int CardManager::remainingCount() const
{
    return static_cast<int>(m_drawPile.size());
}

int CardManager::totalCardCount() const
{
    return static_cast<int>(m_allCards.size());
}

void CardManager::discard(Card* card)
{
    if (card && std::find(m_discardPile.begin(), m_discardPile.end(), card) == m_discardPile.end()) {
        m_discardPile.push_back(card);
        cardDiscarded.emit(card);
    }
}

void CardManager::discardMultiple(const std::vector<Card*>& cards)
{
    for (Card* card : cards) {
        discard(card);
    }
}

int CardManager::discardPileCount() const
{
    return static_cast<int>(m_discardPile.size());
}

void CardManager::reshuffleDiscardPile()
{
    if (m_discardPile.empty()) {
        return;
    }

    m_drawPile.insert(m_drawPile.end(), m_discardPile.begin(), m_discardPile.end());
    m_discardPile.clear();
    shuffle();

    reshuffled.emit();
}

Card* CardManager::findCardById(int id) const
{
    for (Card* card : m_allCards) {
        if (card && card->id() == id) {
            return card;
        }
    }
    return nullptr;
}

Card* CardManager::createCard(CardType type, CardSuit suit, int number)
{
    Card* card = nullptr;

    switch (type) {
    case CardType::Kill:
        card = new KillCard(suit, number);
        break;
    case CardType::Dodge:
        card = new DodgeCard(suit, number);
        break;
    case CardType::Peach:
        card = new PeachCard(suit, number);
        break;
    case CardType::Wine:
        card = new WineCard(suit, number);
        break;
    case CardType::Dismantle:
        card = new DismantleCard(suit, number);
        break;
    case CardType::Steal:
        card = new StealCard(suit, number);
        break;
    case CardType::Bountiful:
        card = new BountifulCard(suit, number);
        break;
    case CardType::BarbarianInvasion:
        card = new BarbarianCard(suit, number);
        break;
    case CardType::Volley:
        card = new VolleyCard(suit, number);
        break;
    case CardType::PeachGarden:
        card = new PeachGardenCard(suit, number);
        break;
    default:
        break;
    }

    return card;
}

CardSuit CardManager::randomSuit() const
{
    int suit = RandomUtils::bounded(4);
    return static_cast<CardSuit>(suit);
}
