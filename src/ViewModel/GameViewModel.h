#ifndef GAMEVIEWMODEL_H
#define GAMEVIEWMODEL_H

#include <string>
#include <vector>
#include <memory>
#include <cstddef>
#include "CommonTypes.h"
#include "Event.h"

class GameState;
class CardManager;
class Player;
class Card;
class Character;
class PlayerViewModel;
class ActionViewModel;
class CardViewModel;
struct PendingActionInfo;

/// 游戏 ViewModel — 中央协调器
/// 管理游戏生命周期、阶段状态机、回合流转
class GameViewModel {
public:
    GameViewModel();
    ~GameViewModel();

    // ==================== 游戏生命周期 ====================

    /// 开始游戏（characterId: 0=曹操, 1=关羽, 2=张飞, 3=赵云）
    void startGame(int characterId1, int characterId2);

    /// 推进到下个阶段
    /// Prepare→Judge→Draw→Play→Discard→End→(切换玩家)→Prepare...
    void advancePhase();

    /// 当前玩家结束出牌（Play→Discard 的快捷方式）
    void endPlayPhase();

    // ==================== 状态查询 ====================

    GameState* gameState() const;
    Player* currentPlayer() const;
    Player* opponentPlayer() const;
    PhaseType currentPhase() const;
    int turnCount() const;
    bool isGameOver() const;
    Player* winner() const;

    /// 获取阶段的中文名称
    static std::string phaseName(PhaseType phase);

    // ==================== 子 ViewModel ====================

    PlayerViewModel* playerVM(int index) const;
    ActionViewModel* actionVM() const;

    // ==================== 手牌展示辅助 ====================

    /// 获取当前玩家手牌的 CardViewModel 列表（重置 UI 状态后计算可打出的牌）
    std::vector<std::unique_ptr<CardViewModel>> getCurrentPlayerCardVMs() const;

    /// 获取某玩家手牌的 CardViewModel 列表
    std::vector<std::unique_ptr<CardViewModel>> getPlayerCardVMs(Player* player) const;

    // ==================== 事件 ====================

    EventListener<PhaseType> phaseChanged;
    EventListener<int> currentPlayerChanged;
    EventListener<Player*> gameOver;
    EventListener<const std::string&> logMessage;
    EventListener<> stateChanged;

private:
    // ---- 初始化 ----
    void initGame(Character* char1, Character* char2);
    Character* createCharacterById(int id);

    // ---- 阶段执行 ----
    void executePhasePrepare();
    void executePhaseJudge();
    void executePhaseDraw();
    void executePhaseEnd();

    // ---- 辅助 ----
    void setNextPhase(PhaseType phase);
    void switchToNextPlayer();
    void emitLog(const std::string& msg);

    // ---- Model 事件回调 ----
    void onPendingActionCreated(const PendingActionInfo& info);
    void onPendingActionCleared();
    void onGameOver(Player* winner);
    void onPlayerDied(Player* player);

    // ---- 成员 ----
    std::unique_ptr<GameState> m_state;
    std::unique_ptr<CardManager> m_cardManager;
    std::unique_ptr<ActionViewModel> m_actionVM;
    std::vector<std::unique_ptr<PlayerViewModel>> m_playerVMs;
    std::vector<std::unique_ptr<Character>> m_characters;  // 武将生命周期管理

    // Model 事件连接 ID
    struct ModelConnections {
        size_t phaseChangedId = 0;
        size_t currentPlayerChangedId = 0;
        size_t pendingActionCreatedId = 0;
        size_t pendingActionClearedId = 0;
        size_t gameOverId = 0;
        size_t stateRefreshedId = 0;
    };
    ModelConnections m_modelConn;
    std::vector<size_t> m_playerDiedConnections;  // 每个玩家的 died 连接 ID
};

#endif // GAMEVIEWMODEL_H
