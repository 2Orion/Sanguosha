#ifndef GAMEVIEWMODEL_H
#define GAMEVIEWMODEL_H

#include <string>
#include <vector>
#include <memory>
#include <cstddef>
#include "Core/CommonTypes.h"
#include "Core/Event.h"
#include "GameState.h"  // 内部使用：PendingActionInfo, GameState

class CardManager;
class Player;
class Card;
class Character;
class PlayerViewModel;
class ActionViewModel;
class CardViewModel;

/// ViewModel 层的待定动作信息（无原始 Model 指针）
struct PendingActionVM {
    int sourceId = -1;            // 动作来源玩家 ID
    int targetId = -1;            // 需要响应的玩家 ID
    int sourceCardId = -1;        // 触发动作的卡牌 ID
    CardType requiredCardType = CardType::Kill;
    std::string description;      // 界面提示文字
    bool canSkip = false;         // 是否可以跳过响应
    std::vector<int> remainingTargetIds; // AOE/链式响应中尚未询问的后续目标 ID
};

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

    /// 跳过非交互阶段，直接推进到需要玩家操作的阶段（Play/Discard）
    void advanceToInteractivePhase();

    // ==================== 状态查询（仅值类型） ====================

    /// 当前玩家 ID（0 或 1），-1 表示无当前玩家
    int currentPlayerId() const;

    /// 对手玩家 ID（0 或 1），-1 表示无对手
    int opponentPlayerId() const;

    PhaseType currentPhase() const;
    int turnCount() const;
    bool isGameOver() const;

    /// 获胜者 ID，-1 表示无（平局或游戏未结束）
    int winnerId() const;

    /// 获取阶段的中文名称
    static std::string phaseName(PhaseType phase);

    // ==================== 待定动作（响应系统） ====================

    bool hasPendingAction() const;
    PendingActionVM pendingActionVM() const;

    // ==================== 子 ViewModel ====================

    PlayerViewModel* playerVM(int index) const;
    ActionViewModel* actionVM() const;

    // ==================== 手牌展示辅助 ====================

    /// 获取当前玩家手牌的 CardViewModel 列表（重置 UI 状态后计算可打出的牌）
    std::vector<std::unique_ptr<CardViewModel>> getCurrentPlayerCardVMs() const;

    /// 获取指定玩家手牌的 CardViewModel 列表（按玩家 ID）
    std::vector<std::unique_ptr<CardViewModel>> getPlayerCardVMs(int playerIndex) const;

    // ==================== 显示数据查找辅助（给 View 用，只返回值类型） ====================

    /// 通过卡牌 ID 查找完整显示字符串（如 "♠A 杀"）
    std::string cardDisplayString(int cardId) const;

    /// 通过卡牌 ID 查找卡牌名称
    std::string cardNameById(int cardId) const;

    /// 通过卡牌 ID 查找卡牌类型
    CardType cardTypeById(int cardId) const;

    /// 通过玩家 ID 获取玩家显示名
    std::string playerDisplayName(int playerId) const;

    // ==================== 事件 ====================

    EventListener<PhaseType> phaseChanged;
    EventListener<int> currentPlayerChanged;                // int = playerIndex
    EventListener<int> gameOver;                            // int = winnerId, -1 = 平局
    EventListener<const std::string&> logMessage;
    EventListener<const PendingActionVM&> pendingActionCreated;
    EventListener<> pendingActionCleared;
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

    // ---- 内部 Model 访问（仅 ViewModel 层内部使用） ----
    GameState* gameState() const;
    Player* currentPlayer() const;
    Player* opponentPlayer() const;
    Player* playerByIndex(int index) const;

    /// 通过 cardId 查找卡牌（遍历所有玩家手牌）
    Card* findCard(int cardId) const;

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
