#ifndef CARDVIEWMODEL_H
#define CARDVIEWMODEL_H

#include <QObject>
#include <QString>
#include "CommonTypes.h"

class Card;

/// 卡牌 ViewModel — 在 Card 基础上增加 UI 展示状态
class CardViewModel : public QObject {
    Q_OBJECT
public:
    explicit CardViewModel(Card* card, QObject* parent = nullptr);

    int id() const;
    CardType cardType() const;
    CardSuit suit() const;
    int number() const;
    QString cardName() const;
    QString description() const;
    CardColor color() const;
    bool isBasic() const;
    bool isStrategy() const;
    QString suitSymbol() const;
    QString numberString() const;

    bool isSelected() const;
    void setSelected(bool selected);

    bool isPlayable() const;
    void setPlayable(bool playable);

    bool isHighlighted() const;
    void setHighlighted(bool highlighted);

    void resetUIState();

    static QString cardTypeName(CardType type);

    Card* card() const;

signals:
    void selectedChanged(bool selected);
    void playableChanged(bool playable);
    void highlightedChanged(bool highlighted);

private:
    Card* m_card;
    bool m_selected   = false;
    bool m_playable   = false;
    bool m_highlighted = false;
};

#endif // CARDVIEWMODEL_H
