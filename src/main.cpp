#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <iostream>
#include <string>
#include <vector>
#include <limits>

#include "ViewModel/GameViewModel.h"
#include "ViewModel/PlayerViewModel.h"
#include "ViewModel/ActionViewModel.h"
#include "ViewModel/CardViewModel.h"

// ==================== 工具函数 ====================

static void clearInput()
{
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

static int readInt(const std::string& prompt, int min, int max)
{
    int value;
    while (true) {
        std::cout << prompt;
        if (std::cin >> value && value >= min && value <= max) {
            clearInput();
            return value;
        }
        std::cout << "无效输入，请输入 " << min << "-" << max << " 之间的数字\n";
        clearInput();
    }
}

static void printSeparator()
{
    std::cout << "----------------------------------------\n";
}

// ==================== 武将选择 ====================

struct CharInfo {
    std::string name;
    std::string skillName;
    std::string skillDesc;
    int maxHp;
};

static const CharInfo CHAR_LIST[] = {
    { "曹操", "奸雄", "受到伤害后摸1张牌", 4 },
    { "关羽", "武圣", "红色牌可当【杀】使用或打出", 4 },
    { "张飞", "咆哮", "出牌阶段可使用任意张【杀】", 4 },
    { "赵云", "龙胆", "【杀】可当【闪】，【闪】可当【杀】", 4 },
};

static int selectCharacter(int playerNum)
{
    std::cout << "\n选择玩家" << playerNum << "的武将:\n";
    for (int i = 0; i < 4; ++i) {
        std::cout << "  " << i << ": " << CHAR_LIST[i].name
                  << "（" << CHAR_LIST[i].skillName << "）"
                  << " - " << CHAR_LIST[i].maxHp << "体力\n";
        std::cout << "     " << CHAR_LIST[i].skillDesc << "\n";
    }
    return readInt("> ", 0, 3);
}

// ==================== 状态显示 ====================

static void showPlayerStatus(PlayerViewModel* pvm)
{
    std::cout << "[" << pvm->displayName() << "] "
              << pvm->characterName()
              << " | 体力: " << pvm->hp() << "/" << pvm->maxHp()
              << " | 手牌: " << pvm->handCardCount() << " 张";
    if (pvm->isDying()) {
        std::cout << " [濒死!]";
    }
    std::cout << "\n";
}

static void showGameStatus(GameViewModel& gvm)
{
    printSeparator();
    std::cout << "回合 " << gvm.turnCount()
              << " | " << GameViewModel::phaseName(gvm.currentPhase()) << "\n";

    for (int i = 0; i < 2; ++i) {
        PlayerViewModel* pvm = gvm.playerVM(i);
        if (pvm) showPlayerStatus(pvm);
    }
    printSeparator();
}

// ==================== 手牌显示 ====================

static void showHandCards(const std::vector<std::unique_ptr<CardViewModel>>& cards)
{
    if (cards.empty()) {
        std::cout << "  （无手牌）\n";
        return;
    }
    for (size_t i = 0; i < cards.size(); ++i) {
        const auto& cvm = cards[i];
        std::cout << "  [" << i << "] "
                  << cvm->suitSymbol() << cvm->numberString()
                  << " " << cvm->cardName();
        if (cvm->isPlayable()) {
            std::cout << " [可打出]";
        }
        std::cout << "\n";
    }
}

// ==================== 响应处理 ====================

/// 处理待定动作（响应请求），返回 true 表示继续，false 表示游戏结束
static bool handlePendingAction(GameViewModel& gvm)
{
    if (!gvm.hasPendingAction()) return true;

    const PendingActionVM& info = gvm.pendingActionVM();
    ActionViewModel* avm = gvm.actionVM();
    int responderId = info.targetId;

    // 通过 PlayerViewModel 检查存活状态
    PlayerViewModel* responderVM = gvm.playerVM(responderId);
    if (!responderVM || !responderVM->isAlive()) return true;

    std::string cardTypeName = CardViewModel::cardTypeName(info.requiredCardType);
    std::string responderName = gvm.playerDisplayName(responderId);

    std::cout << "\n>>> " << responderName
              << " 需要响应: " << info.description << "\n";

    // 获取可响应的牌（int IDs）
    std::vector<int> responseCardIds =
        avm->getResponseCardIds(responderId, info.requiredCardType);

    // 显示响应选项
    int optionIndex = 0;
    std::vector<int> options;
    for (int cardId : responseCardIds) {
        std::string displayStr = gvm.cardDisplayString(cardId);
        std::cout << "  [" << optionIndex << "] 打出 " << displayStr;

        // 技能转化检测：如果卡牌类型不等于要求的类型，说明是技能转化
        if (gvm.cardTypeById(cardId) != info.requiredCardType) {
            std::cout << "（技能转化）";
        }
        std::cout << "\n";
        options.push_back(cardId);
        ++optionIndex;
    }

    if (info.canSkip) {
        std::cout << "  [" << optionIndex << "] 跳过\n";
    }

    int choice = readInt("选择 > ", 0,
                         info.canSkip ? optionIndex : optionIndex - 1);

    if (choice == optionIndex && info.canSkip) {
        avm->skipResponse(responderId);
    } else if (choice >= 0 && choice < static_cast<int>(options.size())) {
        avm->respondCard(options[choice], responderId);
    }

    return true;
}

// ==================== 出牌阶段 ====================

static void playPhase(GameViewModel& gvm)
{
    ActionViewModel* avm = gvm.actionVM();
    int curPlayerId = gvm.currentPlayerId();
    std::string curName = gvm.playerDisplayName(curPlayerId);

    std::cout << "\n--- " << curName << " 的出牌阶段 ---\n";

    while (!gvm.isGameOver() &&
           gvm.currentPhase() == PhaseType::Play &&
           !gvm.hasPendingAction()) {

        auto cardVMs = gvm.getCurrentPlayerCardVMs();
        std::cout << "\n你的手牌:\n";
        showHandCards(cardVMs);

        std::cout << "\n输入牌号使用, 或输入 -1 结束出牌\n";
        int cardIdx = readInt("选择 > ", -1, static_cast<int>(cardVMs.size()) - 1);

        if (cardIdx == -1) {
            gvm.advancePhase();  // 进入弃牌阶段
            return;
        }

        if (cardIdx < 0 || cardIdx >= static_cast<int>(cardVMs.size())) continue;

        int selectedCardId = cardVMs[cardIdx]->id();

        // 使用 ViewModel 方法进行综合可用性检查
        if (!avm->canPlayCard(selectedCardId, curPlayerId)) {
            std::cout << "无法使用这张牌！\n";
            continue;
        }

        // 获取合法目标（int IDs）
        std::vector<int> targetIds = avm->getValidTargetIds(selectedCardId, curPlayerId);

        if (targetIds.empty()) {
            // 无目标卡牌，直接使用
            avm->playCard(selectedCardId, curPlayerId, {});
        } else if (targetIds.size() == 1) {
            // 单一目标，自动选择（通过 ViewModel 获取显示名）
            std::string targetName = gvm.playerDisplayName(targetIds[0]);
            std::cout << "目标: " << targetName << "\n";
            avm->playCard(selectedCardId, curPlayerId, {targetIds[0]});
        } else {
            // 多目标，让玩家选择
            std::cout << "选择目标:\n";
            for (size_t i = 0; i < targetIds.size(); ++i) {
                PlayerViewModel* tvm = gvm.playerVM(targetIds[i]);
                if (tvm) {
                    std::cout << "  [" << i << "] " << tvm->displayName()
                              << "（" << tvm->characterName() << "）\n";
                }
            }
            int targetIdx = readInt("目标 > ", 0,
                                    static_cast<int>(targetIds.size()) - 1);
            avm->playCard(selectedCardId, curPlayerId, {targetIds[targetIdx]});
        }

        // 检查是否触发了待定动作
        if (gvm.hasPendingAction()) {
            handlePendingAction(gvm);
        }
    }
}

// ==================== 弃牌阶段 ====================

static void discardPhase(GameViewModel& gvm)
{
    ActionViewModel* avm = gvm.actionVM();
    int curPlayerId = gvm.currentPlayerId();

    int discardCount = avm->getDiscardCount(curPlayerId);
    if (discardCount <= 0) {
        gvm.advancePhase();  // 不需要弃牌，进入结束阶段
        return;
    }

    std::string curName = gvm.playerDisplayName(curPlayerId);
    std::cout << "\n--- " << curName << " 的弃牌阶段 ---\n";

    // 通过 ViewModel 获取手牌上限和当前手牌数
    PlayerViewModel* curPVM = gvm.playerVM(curPlayerId);

    std::cout << "手牌上限: " << (curPVM ? curPVM->handCardLimit() : 0)
              << " | 当前手牌: " << (curPVM ? curPVM->handCardCount() : 0)
              << " | 需要弃置: " << discardCount << " 张\n";

    while (discardCount > 0 && curPVM && curPVM->hasHandCards()) {
        auto cardVMs = gvm.getPlayerCardVMs(curPlayerId);
        std::cout << "\n手牌:\n";
        showHandCards(cardVMs);
        std::cout << "还需弃置 " << discardCount << " 张\n";

        int cardIdx = readInt("选择要弃置的牌号 > ", 0,
                              static_cast<int>(cardVMs.size()) - 1);
        if (cardIdx >= 0 && cardIdx < static_cast<int>(cardVMs.size())) {
            avm->discardCard(cardVMs[cardIdx]->id(), curPlayerId);
            --discardCount;
        }
    }

    gvm.advancePhase();  // 进入结束阶段
}

// ==================== 主循环 ====================

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::cout << "========================================\n";
    std::cout << "      三国杀 Console Version v1.0\n";
    std::cout << "========================================\n";

    // 1. 选择武将
    int charId1 = selectCharacter(1);
    int charId2 = selectCharacter(2);

    // 2. 创建 ViewModel 并开始游戏
    GameViewModel gvm;

    // 订阅日志
    gvm.logMessage.connect([](const std::string& msg) {
        std::cout << "[LOG] " << msg << "\n";
    });

    // 订阅游戏结束（int winnerId）
    bool gameEnded = false;
    gvm.gameOver.connect([&gvm, &gameEnded](int winnerId) {
        gameEnded = true;
        if (winnerId >= 0) {
            PlayerViewModel* pvm = gvm.playerVM(winnerId);
            if (pvm) {
                std::cout << "\n========================================\n";
                std::cout << "  " << pvm->displayName()
                          << "（" << pvm->characterName() << "）获胜！\n";
                std::cout << "========================================\n";
            }
        }
    });

    gvm.startGame(charId1, charId2);

    // 3. 游戏主循环
    while (!gvm.isGameOver() && !gameEnded) {
        showGameStatus(gvm);

        // 优先处理待定动作
        if (gvm.hasPendingAction()) {
            handlePendingAction(gvm);
            continue;
        }

        PhaseType phase = gvm.currentPhase();

        switch (phase) {
        case PhaseType::Prepare:
        case PhaseType::Judge:
        case PhaseType::Draw:
        case PhaseType::End:
            // 自动阶段，直接推进
            gvm.advancePhase();
            break;

        case PhaseType::Play:
            playPhase(gvm);
            break;

        case PhaseType::Discard:
            discardPhase(gvm);
            break;
        }
    }

    if (!gameEnded && gvm.isGameOver()) {
        int winnerId = gvm.winnerId();
        if (winnerId >= 0) {
            PlayerViewModel* pvm = gvm.playerVM(winnerId);
            if (pvm) {
                std::cout << "\n" << pvm->displayName()
                          << "（" << pvm->characterName() << "）获胜！\n";
            }
        }
    }

    std::cout << "\n按 Enter 退出...";
    std::cin.get();
    return 0;
}
