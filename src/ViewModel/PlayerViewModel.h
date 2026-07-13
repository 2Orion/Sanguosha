#ifndef PLAYERVIEWMODEL_H
#define PLAYERVIEWMODEL_H

#include <QObject>
#include <QString>
#include "CommonTypes.h"

class Player;
class Card;

/// 玩家 ViewModel — 包装 Player，转发 Qt 信号，提供只读值类型查询
class PlayerViewModel : public QObject {
    Q_OBJECT
public:
    explicit PlayerViewModel(Player* player, QObject* parent = nullptr);
    ~PlayerViewModel() override;

    int playerId() const;
    QString displayName() const;
    QString characterName() const;
    QString skillName() const;
    QString skillDescription() const;

    int hp() const;
    int maxHp() const;
    double hpRatio() const;
    bool isAlive() const;
    bool isDying() const;
    bool isFullHp() const;

    int handCardCount() const;
    bool hasHandCards() const;
    int handCardLimit() const;

    bool hasUsedKillThisTurn() const;
    bool isWineEnhanced() const;

    int equipCount() const;
    int judgmentCount() const;

    Player* player() const;

signals:
    void hpChanged(int hp);
    void maxHpChanged(int maxHp);
    void dying(int playerId);
    void died(int playerId);
    void revived(int playerId);
    void handCardAdded(int cardId);
    void handCardRemoved(int cardId);
    void handCardsChanged();
    void stateChanged();

private:
    Player* m_player;
};

#endif // PLAYERVIEWMODEL_H
