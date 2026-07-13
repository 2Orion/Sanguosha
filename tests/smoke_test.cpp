// 冒烟测试：直接驱动 Model 层核心流程，验证 Qt 移除后行为是否正确。
// 不依赖任何测试框架，方便随时用 g++ 单独编译运行。
#include "CommonTypes.h"
#include "Event.h"
#include "GameState.h"
#include "Player.h"
#include "Character.h"
#include "Card.h"
#include "CardManager.h"
#include "GameRule.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description)
{
    ++g_checks;
    if (condition) {
        std::cout << "[OK]   " << description << "\n";
    } else {
        ++g_failures;
        std::cout << "[FAIL] " << description << "\n";
    }
}

Card* takeIf(std::vector<Card*>& pool, const std::function<bool(Card*)>& pred)
{
    for (auto it = pool.begin(); it != pool.end(); ++it) {
        if (*it && pred(*it)) {
            Card* c = *it;
            pool.erase(it);
            return c;
        }
    }
    return nullptr;
}

Card* takeOfType(std::vector<Card*>& pool, CardType type)
{
    return takeIf(pool, [type](Card* c) { return c->cardType() == type; });
}

} // namespace

int main()
{
    // ==================== 1. CardManager：初始化、摸牌、回收重洗 ====================
    CardManager cardManager;
    cardManager.initialize();
    check(cardManager.totalCardCount() == 48, "CardManager: 总牌数为48");
    check(cardManager.remainingCount() == 48, "CardManager: 初始牌堆为48");

    std::vector<Card*> tempBatch = cardManager.drawCards(5);
    check(tempBatch.size() == 5, "CardManager: drawCards(5) 取到5张");
    check(cardManager.remainingCount() == 43, "CardManager: 摸5张后剩43张");
    cardManager.discardMultiple(tempBatch);
    check(cardManager.discardPileCount() == 5, "CardManager: 弃牌堆有5张");

    std::vector<Card*> restOfDeck = cardManager.drawCards(43);
    check(restOfDeck.size() == 43, "CardManager: 摸空剩余43张");
    check(cardManager.remainingCount() == 0, "CardManager: 牌堆摸空后剩0张");

    Card* afterReshuffle = cardManager.drawCard();
    check(afterReshuffle != nullptr, "CardManager: 牌堆空时摸牌自动回收弃牌堆重洗");
    check(cardManager.discardPileCount() == 0, "CardManager: 重洗后弃牌堆清空");

    std::vector<Card*> pool;
    pool.insert(pool.end(), restOfDeck.begin(), restOfDeck.end());
    pool.insert(pool.end(), tempBatch.begin(), tempBatch.end());
    check(pool.size() == 48, "测试准备: pool 汇总了全部48张牌（含重洗的一张已在tempBatch中）");

    // ==================== 2. GameState / Player / Character 基础状态 ====================
    GameState state;
    state.setCardManager(&cardManager);
    state.setCurrentPhase(PhaseType::Play);

    auto p1 = std::make_unique<Player>();
    auto p2 = std::make_unique<Player>();
    auto p3 = std::make_unique<Player>();
    p1->setDisplayName("甲");
    p2->setDisplayName("乙");
    p3->setDisplayName("丙");

    CaoCao caoCao;
    GuanYu guanYu;
    ZhangFei zhangFei;
    ZhaoYun zhaoYun;

    p1->setCharacter(&caoCao);
    p2->setCharacter(&guanYu);
    p3->setCharacter(&zhangFei);

    check(p1->hp() == 4 && p1->maxHp() == 4, "Player: setCharacter后hp/maxHp均为4");
    check(p1->isFullHp(), "Player: 满血状态");
    check(p1->handCardLimit() == 4, "Player: 手牌上限等于当前体力");

    // 转移所有权到 GameState 前保存裸指针
    Player* p1Raw = p1.get();
    Player* p2Raw = p2.get();
    Player* p3Raw = p3.get();

    state.addPlayer(std::move(p1));
    state.addPlayer(std::move(p2));
    state.addPlayer(std::move(p3));
    check(state.playerCount() == 3, "GameState: 玩家数为3");
    check(state.alivePlayers().size() == 3, "GameState: 初始3人均存活");

    // ==================== 3. 武将技能纯逻辑 ====================
    KillCard probeKill(CardSuit::Spade, 5);
    DodgeCard probeDodge(CardSuit::Heart, 6);
    check(zhaoYun.skillTransformCard(&probeKill) == CardType::Dodge, "赵云-龙胆: 杀可转化为闪");
    check(zhaoYun.skillTransformCard(&probeDodge) == CardType::Kill, "赵云-龙胆: 闪可转化为杀");

    check(caoCao.triggerCondition(GameEvent::OnDamage, &state, p1Raw), "曹操-奸雄: 受到伤害事件触发条件成立");
    check(!caoCao.triggerCondition(GameEvent::OnCardPlayed, &state, p1Raw), "曹操-奸雄: 无关事件不触发");

    p3Raw->setUsedKillThisTurn(true);
    check(GameRule::canPlayKill(&state, p3Raw), "张飞-咆哮: 即使本回合已用杀仍可再次使用");
    p3Raw->resetTurnState();

    // ==================== 4. Card 多态接口（Kill / Peach） ====================
    KillCard sampleKill(CardSuit::Spade, 7);
    check(sampleKill.canUse(&state, p1Raw), "KillCard: 出牌阶段、未用过杀时可使用");
    check(!sampleKill.canTarget(&state, p1Raw, p1Raw), "KillCard: 不能指向自己");
    check(sampleKill.canTarget(&state, p1Raw, p2Raw), "KillCard: 可指向其他存活玩家");
    check(sampleKill.getValidTargets(&state, p1Raw).size() == 2, "KillCard: 合法目标为除自己外的2人");

    ActionResult killResult = sampleKill.execute(&state, p1Raw, {p2Raw});
    check(killResult == ActionResult::RequiresDodge, "KillCard::execute 返回 RequiresDodge");
    check(state.hasPendingAction() && state.pendingActionInfo().requiredCardType == CardType::Dodge,
          "KillCard::execute 正确串联到 GameRule::executeKill");
    check(p1Raw->hasUsedKillThisTurn(), "KillCard::execute 标记本回合已用杀");
    GameRule::handleKillResponse(&state, p2Raw, nullptr);
    check(p2Raw->hp() == 3, "接口测试: p2 未闪避，扣1点血");
    p1Raw->resetTurnState();

    PeachCard samplePeach(CardSuit::Heart, 3);
    check(!samplePeach.canUse(&state, p1Raw), "PeachCard: 满血时不可使用");
    check(samplePeach.canUse(&state, p2Raw), "PeachCard: 受伤后可使用");
    check(samplePeach.canTarget(&state, p2Raw, p2Raw), "PeachCard: 只能指向自己");
    check(!samplePeach.canTarget(&state, p2Raw, p1Raw), "PeachCard: 不能指向他人");
    ActionResult peachResult = samplePeach.execute(&state, p2Raw, {p2Raw});
    check(peachResult == ActionResult::Completed, "PeachCard::execute 返回 Completed");
    check(p2Raw->hp() == 4, "接口测试: p2 回复至满血");

    // ==================== 5. GameRule: 杀/闪循环 + 酒 ====================
    Card* dodgeForP2 = takeOfType(pool, CardType::Dodge);
    check(dodgeForP2 != nullptr, "测试准备: 取到一张闪");
    p2Raw->addHandCard(dodgeForP2);

    check(GameRule::canPlayKill(&state, p1Raw), "规则: p1 本回合尚未用杀，可以出杀");
    GameRule::executeKill(&state, p1Raw, p2Raw);
    check(p1Raw->hasUsedKillThisTurn(), "杀: 攻击者标记本回合已用杀");
    check(state.pendingActionInfo().target == p2Raw, "杀: 待响应目标为p2");
    GameRule::handleKillResponse(&state, p2Raw, dodgeForP2);
    check(p2Raw->hp() == 4, "闪: 成功闪避不掉血");
    check(!p2Raw->hasCard(dodgeForP2), "闪: 闪牌从手牌移除");
    check(!state.hasPendingAction(), "闪: 响应后待定动作清空");
    p1Raw->resetTurnState();

    GameRule::executeKill(&state, p1Raw, p2Raw);
    GameRule::handleKillResponse(&state, p2Raw, nullptr);
    check(p2Raw->hp() == 3, "杀(未闪): 扣1点血");
    p1Raw->resetTurnState();

    GameRule::executeWine(&state, p1Raw);
    check(p1Raw->isWineEnhanced(), "酒: 攻击者标记酒强化");
    GameRule::executeKill(&state, p1Raw, p2Raw);
    GameRule::handleKillResponse(&state, p2Raw, nullptr);
    check(p2Raw->hp() == 1, "酒+杀(未闪): 扣2点血");
    check(!p1Raw->isWineEnhanced(), "酒: 使用后强化状态被消耗");
    p1Raw->resetTurnState();

    // ==================== 6. GameRule: 桃 ====================
    Card* peachForP2 = takeOfType(pool, CardType::Peach);
    check(peachForP2 != nullptr, "测试准备: 取到一张桃");
    p2Raw->addHandCard(peachForP2);
    GameRule::executePeach(&state, p2Raw, p2Raw);
    check(p2Raw->hp() == 2, "桃: 回复1点体力");
    p2Raw->removeHandCard(peachForP2);
    cardManager.discard(peachForP2);

    // ==================== 7. 关羽-武圣: 红牌转化为杀 ====================
    Card* redNonKill = takeIf(pool, [](Card* c) {
        return c->isRed() && c->cardType() != CardType::Kill && c->cardType() != CardType::Dodge;
    });
    check(redNonKill != nullptr, "测试准备: 取到一张非杀/闪的红牌");
    p2Raw->addHandCard(redNonKill);
    check(GameRule::hasKillToRespond(p2Raw), "关羽-武圣: 红牌可当杀响应南蛮入侵");
    p2Raw->removeHandCard(redNonKill);
    cardManager.discard(redNonKill);

    // ==================== 8. 曹操-奸雄: 受伤摸牌 ====================
    int p1HandBefore = p1Raw->handCardCount();
    GameRule::dealDamage(&state, p1Raw, 1, p2Raw);
    check(p1Raw->hp() == 3, "奸雄前置: p1 受到1点伤害");
    check(p1Raw->handCardCount() == p1HandBefore + 1, "曹操-奸雄: 受伤后摸1张牌");

    // ==================== 9. 过河拆桥 / 顺手牵羊 ====================
    Card* lootA = takeOfType(pool, CardType::Bountiful);
    check(lootA != nullptr, "测试准备: 给p2一张牌作为拆桥目标");
    p2Raw->addHandCard(lootA);
    GameRule::executeDismantle(&state, p1Raw, p2Raw);
    check(!p2Raw->hasCard(lootA), "过河拆桥: 目标手牌被弃置");
    check(cardManager.discardPileCount() > 0, "过河拆桥: 弃置的牌进入弃牌堆");

    Card* lootB = takeOfType(pool, CardType::Steal);
    check(lootB != nullptr, "测试准备: 给p3一张牌作为顺手牵羊目标");
    p3Raw->addHandCard(lootB);
    GameRule::executeSteal(&state, p1Raw, p3Raw);
    check(!p3Raw->hasCard(lootB), "顺手牵羊: 目标手牌被拿走");
    check(p1Raw->hasCard(lootB), "顺手牵羊: 发动者获得该牌");

    // ==================== 10. 无中生有 ====================
    int p1HandBeforeBountiful = p1Raw->handCardCount();
    GameRule::executeBountiful(&state, p1Raw);
    check(p1Raw->handCardCount() == p1HandBeforeBountiful + 2, "无中生有: 摸2张牌");

    // ==================== 11. 桃园结义 ====================
    int p1HpBefore = p1Raw->hp();
    int p2HpBefore = p2Raw->hp();
    int p3HpBefore = p3Raw->hp();
    GameRule::executePeachGarden(&state);
    check(p1Raw->hp() == std::min(p1HpBefore + 1, p1Raw->maxHp()), "桃园结义: p1回复1点(未满则+1)");
    check(p2Raw->hp() == std::min(p2HpBefore + 1, p2Raw->maxHp()), "桃园结义: p2回复1点(未满则+1)");
    check(p3Raw->hp() == p3HpBefore, "桃园结义: p3已满血不受影响");
    check(p1Raw->isFullHp(), "桃园结义后: p1恢复满血");

    // ==================== 12. 南蛮入侵：多目标链式响应（回归测试） ====================
    Card* killForP2 = takeOfType(pool, CardType::Kill);
    check(killForP2 != nullptr, "测试准备: 给p2一张杀用于响应南蛮入侵");
    p2Raw->addHandCard(killForP2);

    GameRule::executeBarbarianInvasion(&state, p1Raw);
    check(state.hasPendingAction() && state.pendingActionInfo().target == p2Raw,
          "南蛮入侵: 第一个响应目标为p2");
    check(state.pendingActionInfo().canSkip, "南蛮入侵: 可跳过标记为true");

    int p2HpBeforeAoe = p2Raw->hp();
    GameRule::handleAoeKillResponse(&state, p2Raw, killForP2);
    check(p2Raw->hp() == p2HpBeforeAoe, "南蛮入侵: p2出杀，不掉血");
    check(state.hasPendingAction() && state.pendingActionInfo().target == p3Raw,
          "南蛮入侵: 链式推进到p3");

    int p3HpBeforeAoe = p3Raw->hp();
    GameRule::handleAoeKillResponse(&state, p3Raw, nullptr);
    check(p3Raw->hp() == p3HpBeforeAoe - 1, "南蛮入侵: p3未出杀，扣1点血");
    check(!state.hasPendingAction(),
          "南蛮入侵(回归): 两名目标都已响应后链条正确终止，不会重复询问p2");

    // ==================== 13. 濒死流程：单人救援成功 ====================
    GameRule::dealDamage(&state, p2Raw, p2Raw->hp() - 1, p1Raw);
    check(p2Raw->hp() == 1, "濒死准备: p2降至1点血");

    GameRule::dealDamage(&state, p2Raw, 1, p1Raw);
    check(p2Raw->hp() == 0, "濒死: 致命伤害后hp为0");
    check(p2Raw->isDying(), "濒死: 玩家被标记为濒死");
    check(state.hasPendingAction(), "濒死(回归): 致命伤害后应自动进入待救援状态");
    check(state.pendingActionInfo().requiredCardType == CardType::Peach, "濒死: 待定动作要求桃");
    check(state.pendingActionInfo().target == p2Raw, "濒死: 待定动作目标为濒死玩家");
    check(state.pendingActionInfo().source != nullptr, "濒死(回归): 待定动作应记录当前询问的救援者");

    Card* peachForSave = takeOfType(pool, CardType::Peach);
    check(peachForSave != nullptr, "测试准备: 取到一张桃用于救援");
    p3Raw->addHandCard(peachForSave);
    bool saved = GameRule::handleDyingPeach(&state, p2Raw, p3Raw, peachForSave);
    check(saved, "濒死: handleDyingPeach 返回救援成功");
    check(p2Raw->hp() == 1, "濒死: 被救玩家回复到1点血");
    check(!p2Raw->isDying(), "濒死: 被救玩家不再处于濒死状态");
    check(!state.hasPendingAction(), "濒死: 救援后待定动作清空");

    // ==================== 14. 濒死流程：多人依次跳过，最终死亡 ====================
    GameRule::dealDamage(&state, p2Raw, 1, p1Raw);
    check(p2Raw->hp() == 0, "濒死链准备: p2再次降至0点血");
    check(state.hasPendingAction(), "濒死链: 再次进入待救援状态");

    Player* firstSavior = state.pendingActionInfo().source;
    check(firstSavior != nullptr, "濒死链: 记录了首位被询问的救援者");

    GameRule::skipDyingResponse(&state, p2Raw);
    check(state.hasPendingAction(), "濒死链(回归): 第一人跳过后应询问下一位，而非直接判定死亡");
    Player* secondSavior = state.pendingActionInfo().source;
    check(secondSavior != nullptr && secondSavior != firstSavior,
          "濒死链(回归): 跳过后询问的是不同的下一位玩家");

    GameRule::skipDyingResponse(&state, p2Raw);
    check(!p2Raw->isAlive(), "濒死链: 所有人跳过后玩家死亡");
    check(!p2Raw->isDying(), "濒死链: 死亡后不再是濒死状态");
    check(!state.hasPendingAction(), "濒死链: 死亡判定后待定动作清空");
    check(!state.isGameOver(), "濒死链: 仍有2人存活，游戏未结束");

    // ==================== 15. 胜负判定 ====================
    GameRule::dealDamage(&state, p3Raw, p3Raw->hp() - 1, p1Raw);
    GameRule::dealDamage(&state, p3Raw, 1, p1Raw);
    check(p3Raw->hp() == 0, "终局准备: p3降至0点血");
    check(state.hasPendingAction(), "终局准备: p3进入待救援状态");

    GameRule::skipDyingResponse(&state, p3Raw);
    check(!p3Raw->isAlive(), "终局: 唯一的救援者跳过后p3死亡");
    check(state.isGameOver(), "终局: 仅剩1人存活，游戏结束");
    check(state.winner() == p1Raw, "终局: 获胜者为p1");

    // ==================== 汇总 ====================
    std::cout << "\n============================\n";
    std::cout << "总检查项: " << g_checks << "，失败: " << g_failures << "\n";
    std::cout << "============================\n";

    return g_failures == 0 ? 0 : 1;
}
