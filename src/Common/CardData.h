#ifndef CARDDATA_H
#define CARDDATA_H

#include <QString>
#include <QVector>
#include "CommonTypes.h"

/// 卡牌展示数据（值类型，无指针，跨层传递用）
struct CardData {
    int cardId = -1;
    CardType cardType = CardType::Kill;
    CardSuit suit = CardSuit::Spade;
    int number = 0;
    QString cardName;
    QString description;
    CardColor color = CardColor::Black;
    bool isBasic = false;
    bool isStrategy = false;
    bool isEquipment = false;
    QString suitSymbol;
    QString numberString;
    bool isSelected = false;
    bool isPlayable = false;
    bool isHighlighted = false;
    int ownerId = -1;

    // ==================== 装备牌字段 ====================
    int equipSlot = -1;            // 装备槽位（-1 表示非装备，0=Weapon, 1=Armor, 2=Mount）
    int attackRange = 0;           // 武器攻击距离加成
};

using CardList = QVector<CardData>;

#endif // CARDDATA_H
