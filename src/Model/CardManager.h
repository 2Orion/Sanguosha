#ifndef CARDMANAGER_H
#define CARDMANAGER_H

#include <vector>
#include <QObject>
#include "CommonTypes.h"

class Card;

class CardManager : public QObject {
    Q_OBJECT
public:
    explicit CardManager(QObject* parent = nullptr);
    ~CardManager() override;

    void initialize();
    void shuffle();
    Card* drawCard();
    std::vector<Card*> drawCards(int count);
    int remainingCount() const;
    int totalCardCount() const;
    void discard(Card* card);
    void discardMultiple(const std::vector<Card*>& cards);
    int discardPileCount() const;
    void reshuffleDiscardPile();
    Card* findCardById(int id) const;
    const std::vector<Card*>& getDrawPile() const { return m_drawPile; }

signals:
    void drawPileEmpty();
    void reshuffled();
    void cardDiscarded(int cardId);

private:
    Card* createCard(CardType type, CardSuit suit, int number);
    CardSuit randomSuit() const;

    std::vector<Card*> m_drawPile;
    std::vector<Card*> m_discardPile;
    std::vector<Card*> m_allCards;
};

#endif // CARDMANAGER_H
