#ifndef PLAYERDATA_H
#define PLAYERDATA_H

#include <QString>
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
    int attackRange = 1;       // 攻击距离（含武器加成）

    // ==================== 装备区展示数据 ====================
    QVector<CardData> equipCards;  // 已装备的卡牌列表（按槽位顺序，仅非空槽）
};

#endif // PLAYERDATA_H
