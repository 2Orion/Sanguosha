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

    auto addOne = [&](CardType type, CardSuit suit, int number) {
        Card* card = createCard(type, suit, number);
        m_allCards.push_back(card);
        m_drawPile.push_back(card);
    };

    auto addMultiple = [&](CardType type, int count) {
        for (int i = 0; i < count; ++i) {
            Card* card = createCard(type, randomSuit(), (i % 13) + 1);
            m_allCards.push_back(card);
            m_drawPile.push_back(card);
        }
    };

    // ==================== 基本牌（58张） ====================
    addMultiple(CardType::Kill, 30);
    addMultiple(CardType::Dodge, 15);
    addMultiple(CardType::Peach, 8);
    addMultiple(CardType::Wine, 5);

    // ==================== 普通锦囊（20张） ====================
    addMultiple(CardType::Dismantle, 3);
    addMultiple(CardType::Steal, 3);
    addMultiple(CardType::Bountiful, 3);
    addMultiple(CardType::BarbarianInvasion, 3);
    addMultiple(CardType::Volley, 3);
    addOne(CardType::PeachGarden, CardSuit::Heart, 1);

    // 决斗（♠A, ♦A, ♣A）
    addOne(CardType::Duel, CardSuit::Spade, 1);
    addOne(CardType::Duel, CardSuit::Diamond, 1);
    addOne(CardType::Duel, CardSuit::Club, 1);

    // 借刀杀人（♠Q, ♣K）
    addOne(CardType::Borrow, CardSuit::Spade, 12);
    addOne(CardType::Borrow, CardSuit::Club, 13);

    // 无懈可击（♠K, ♣Q, ♣K）
    addOne(CardType::Nullify, CardSuit::Spade, 13);
    addOne(CardType::Nullify, CardSuit::Club, 12);
    addOne(CardType::Nullify, CardSuit::Club, 13);

    // ==================== 延时锦囊（6张） ====================
    // 乐不思蜀（♥6 ×2, ♠6）
    addOne(CardType::Happy, CardSuit::Heart, 6);
    addOne(CardType::Happy, CardSuit::Heart, 6);
    addOne(CardType::Happy, CardSuit::Spade, 6);

    // 兵粮寸断（♠10, ♣10）
    addOne(CardType::Famine, CardSuit::Spade, 10);
    addOne(CardType::Famine, CardSuit::Club, 10);

    // 闪电（♠A）
    addOne(CardType::Lightning, CardSuit::Spade, 1);

    // ==================== 装备牌（11张） ====================
    // 诸葛连弩（♠A ×2）
    addOne(CardType::Crossbow, CardSuit::Spade, 1);
    addOne(CardType::Crossbow, CardSuit::Spade, 1);

    // 青龙偃月刀（♠5）
    addOne(CardType::QinglongBlade, CardSuit::Spade, 5);

    // 青釭剑（♠6）
    addOne(CardType::QinggangSword, CardSuit::Spade, 6);

    // 寒冰剑（♠2）
    addOne(CardType::IceSword, CardSuit::Spade, 2);

    // 雌雄双股剑（♠2）
    addOne(CardType::DualSword, CardSuit::Spade, 2);

    // 仁王盾（♠2）
    addOne(CardType::BenevolentShield, CardSuit::Spade, 2);

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

bool CardManager::isInDiscardPile(const Card* card) const
{
    return card && std::find(m_discardPile.begin(), m_discardPile.end(), card) != m_discardPile.end();
}

bool CardManager::takeFromDiscard(Card* card)
{
    auto it = std::find(m_discardPile.begin(), m_discardPile.end(), card);
    if (it == m_discardPile.end()) return false;
    m_discardPile.erase(it);
    return true;
}

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
    // 基本牌
    case CardType::Kill:               return new KillCard(suit, number);
    case CardType::Dodge:              return new DodgeCard(suit, number);
    case CardType::Peach:              return new PeachCard(suit, number);
    case CardType::Wine:               return new WineCard(suit, number);
    // 锦囊牌
    case CardType::Dismantle:          return new DismantleCard(suit, number);
    case CardType::Steal:              return new StealCard(suit, number);
    case CardType::Bountiful:          return new BountifulCard(suit, number);
    case CardType::BarbarianInvasion:  return new BarbarianCard(suit, number);
    case CardType::Volley:             return new VolleyCard(suit, number);
    case CardType::PeachGarden:        return new PeachGardenCard(suit, number);
    // 新锦囊牌
    case CardType::Duel:               return new DuelCard(suit, number);
    case CardType::Lightning:          return new LightningCard(suit, number);
    case CardType::Nullify:            return new NullifyCard(suit, number);
    case CardType::Borrow:             return new BorrowCard(suit, number);
    case CardType::Harvest:            return new HarvestCard(suit, number);
    case CardType::Happy:              return new HappyCard(suit, number);
    case CardType::Famine:             return new FamineCard(suit, number);
    // 装备牌
    case CardType::Crossbow:           return new CrossbowCard(suit, number);
    case CardType::QinglongBlade:      return new QinglongBladeCard(suit, number);
    case CardType::ZhangbaSnake:       return new ZhangbaSnakeCard(suit, number);
    case CardType::KylinBow:           return new KylinBowCard(suit, number);
    case CardType::QinggangSword:      return new QinggangSwordCard(suit, number);
    case CardType::IceSword:           return new IceSwordCard(suit, number);
    case CardType::DualSword:          return new DualSwordCard(suit, number);
    case CardType::EightDiagrams:      return new EightDiagramsCard(suit, number);
    case CardType::BenevolentShield:   return new BenevolentShieldCard(suit, number);
    }
    return nullptr;
}

CardSuit CardManager::randomSuit() const
{
    return static_cast<CardSuit>(QRandomGenerator::global()->bounded(4));
}
