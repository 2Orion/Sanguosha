#ifndef PLAYERDISPLAYDATA_H
#define PLAYERDISPLAYDATA_H

#include <QString>

/// 玩家展示数据（值类型，无指针，跨层传递用）
struct PlayerDisplayData {
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
};

#endif // PLAYERDISPLAYDATA_H
