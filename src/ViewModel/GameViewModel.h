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

    PhaseType currentPhase() const;
    int turnCount() const;
    bool isGameOver() const;

    /// 获胜者信息（游戏结束后查询）
    std::string winnerDisplayName() const;
    std::string winnerCharacterName() const;

    /// 获取阶段的中文名称
    static std::string phaseName(PhaseType phase);

    // ==================== 待定动作查询（封装 GameState，View 无需知道 Model）====================

    bool hasPendingAction() const;
    std::string pendingActionDescription() const;
    bool canSkipPendingAction() const;
    std::string pendingActionTargetName() const;

    // ==================== 当前玩家查询（封装 Player，View 无需知道 Model）====================

    std::string currentPlayerDisplayName() const;
    std::string currentPlayerCharacterName() const;
    int currentPlayerHp() const;
    int currentPlayerMaxHp() const;
    int currentPlayerHandCardCount() const;
    int currentPlayerHandCardLimit() const;
    bool currentPlayerHasHandCards() const;

    // ==================== 玩家信息查询（按索引）====================

    std::string playerDisplayName(int index) const;
    std::string playerCharacterName(int index) const;

    // ==================== 子 ViewModel ====================

    PlayerViewModel* playerVM(int index) const;

    // ==================== 手牌展示辅助 ====================

    /// 获取当前玩家手牌的 CardViewModel 列表（重置 UI 状态后计算可打出的牌）
    std::vector<std::unique_ptr<CardViewModel>> getCurrentPlayerCardVMs() const;

    /// 获取某玩家手牌的 CardViewModel 列表
    std::vector<std::unique_ptr<CardViewModel>> getPlayerCardVMs(Player* player) const;

    // ==================== 出牌阶段操作 ====================

    /// 检查当前玩家的第 cardIndex 张手牌是否可打出
    bool canPlayCardByIndex(int cardIndex) const;

    /// 获取第 cardIndex 张手牌的合法目标玩家下标列表
    std::vector<int> getPlayTargetIndices(int cardIndex) const;

    /// 打出第 cardIndex 张手牌（targetIndex: -1 表示无目标）
    void executePlayCard(int cardIndex, int targetIndex);

    // ==================== 响应阶段操作 ====================

    /// 获取可响应的 CardViewModel 列表（已标记技能转化牌）
    std::vector<std::unique_ptr<CardViewModel>> getResponseCardVMs() const;

    /// 用第 cardIndex 张响应牌响应
    void executeRespondCard(int cardIndex);

    /// 跳过当前响应
    void executeSkipResponse();

    /// 尝试自动解决待定动作（无可响应牌且不能跳过时自动扣血/推进）
    /// @return true 表示已自动解决，View 无需弹出交互
    bool tryAutoResolvePendingAction();

    // ==================== 弃牌阶段操作 ====================

    /// 当前玩家需要弃置的张数（0 则不需要）
    int getDiscardCount() const;

    /// 弃置当前玩家的第 cardIndex 张手牌
    void executeDiscardCard(int cardIndex);

    // ==================== 事件 ====================

    EventListener<PhaseType> phaseChanged;
    EventListener<int> currentPlayerChanged;
    EventListener<> gameOver;
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
