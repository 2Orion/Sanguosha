#ifndef PLAYERDATA_H
#define PLAYERDATA_H

#include <QString>
#include <QVector>
#include "CardData.h"

/// 玩家展示数据（值类型，无指针，跨层传递用）
struct PlayerData {
    int playerId = -1;
    QString displayName;
    QString characterName;
    QString skillName;
    QString skillDescription;
    int hp = 0;
    int maxHp = 0;
    bool isAlive = false;
    bool isDying = false;
    int handCardCount = 0;
    int handCardLimit = 0;
    bool isCurrentPlayer = false;
    /// 装备区卡牌（按 EquipSlot 索引，空位为无效 CardData）
    QVector<CardData> equipCards;
    int attackRange = 1;
};

#endif // PLAYERDATA_H
