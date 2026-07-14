#ifndef PENDINGACTIONDATA_H
#define PENDINGACTIONDATA_H

#include <QString>
#include <QVector>
#include "CommonTypes.h"

/// 待定动作信息（值类型，跨层传递用）
struct PendingActionData {
    int sourceId = -1;
    int targetId = -1;
    int sourceCardId = -1;
    CardType requiredCardType = CardType::Kill;
    QString description;
    bool canSkip = false;
    QVector<int> remainingTargetIds;
};

#endif // PENDINGACTIONDATA_H
