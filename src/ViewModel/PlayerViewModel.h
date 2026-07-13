#ifndef PLAYERVIEWMODEL_H
#define PLAYERVIEWMODEL_H

#include <string>
#include <vector>
#include <cstddef>
#include "CommonTypes.h"
#include "Event.h"

class Player;
class Card;

/// 玩家 ViewModel — 包装 Player 并转发其事件，
/// 方便 View 层只监听 ViewModel 而不直接依赖 Model
class PlayerViewModel {
public:
    explicit PlayerViewModel(Player* player);
    ~PlayerViewModel();

    // ==================== 身份标识 ====================

    int playerId() const;
    std::string displayName() const;
    std::string characterName() const;
    std::string skillName() const;
    std::string skillDescription() const;

    // ==================== 体力 ====================

    int hp() const;
    int maxHp() const;
    double hpRatio() const;
    bool isAlive() const;
    bool isDying() const;
    bool isFullHp() const;

    // ==================== 手牌 ====================

    int handCardCount() const;
    bool hasHandCards() const;
    int handCardLimit() const;

    // ==================== 回合状态 ====================

    bool hasUsedKillThisTurn() const;
    bool isWineEnhanced() const;

    // ==================== 装备/判定区（预留） ====================

    int equipCount() const;
    int judgmentCount() const;

    // ==================== 事件（从 Model 转发，值类型） ====================

    EventListener<int> hpChanged;
    EventListener<int> maxHpChanged;
    EventListener<> dying;
    EventListener<> died;
    EventListener<> revived;
    EventListener<int> handCardAdded;     // cardId
    EventListener<int> handCardRemoved;   // cardId
    EventListener<> handCardsChanged;
    EventListener<> stateChanged;

private:
    void connectModelEvents();

    Player* m_player;

    // 每个 Model 事件的连接 ID
    struct EventConnections {
        size_t hpChangedId = 0;
        size_t maxHpChangedId = 0;
        size_t dyingId = 0;
        size_t diedId = 0;
        size_t revivedId = 0;
        size_t handCardAddedId = 0;
        size_t handCardRemovedId = 0;
        size_t handCardsChangedId = 0;
        size_t stateChangedId = 0;
    };
    EventConnections m_conn;
};

#endif // PLAYERVIEWMODEL_H
