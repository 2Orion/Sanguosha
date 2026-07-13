#ifndef CARDDISPLAYDATA_H
#define CARDDISPLAYDATA_H

#include <QString>
#include <QVector>
#include "CommonTypes.h"

/// 卡牌展示数据（值类型，无指针，跨层传递用）
struct CardDisplayData {
    int cardId = -1;
    CardType cardType = CardType::Kill;
    CardSuit suit = CardSuit::Spade;
    int number = 0;
    QString cardName;
    QString description;
    CardColor color = CardColor::Black;
    bool isBasic = false;
    bool isStrategy = false;
    QString suitSymbol;
    QString numberString;
    bool isSelected = false;
    bool isPlayable = false;
    bool isHighlighted = false;
    int ownerId = -1;
};

using CardDisplayList = QVector<CardDisplayData>;

#endif // CARDDISPLAYDATA_H
