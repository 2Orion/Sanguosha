#include <QtTest>
#include <QSignalSpy>

#include <algorithm>
#include <map>
#include <memory>
#include <vector>

#include "Card.h"
#include "CardManager.h"
#include "Character.h"
#include "GameRule.h"
#include "GameState.h"
#include "Player.h"

namespace {

struct DuelFixture {
    GameState state;
    CardManager cardManager;
    Character character1;
    Character character2;
    Player player1;
    Player player2;

    DuelFixture()
        : character1("Player1", 4, "Skill1", "Description1")
        , character2("Player2", 4, "Skill2", "Description2")
    {
        cardManager.initialize();
        state.setCardManager(&cardManager);

        player1.setPlayerId(0);
        player1.setDisplayName(QStringLiteral("Player 1"));
        player1.setCharacter(&character1);
        player2.setPlayerId(1);
        player2.setDisplayName(QStringLiteral("Player 2"));
        player2.setCharacter(&character2);

        state.addPlayer(&player1);
        state.addPlayer(&player2);
        state.setCurrentPhase(PhaseType::Play);
    }
};

struct TripleFixture {
    GameState state;
    CardManager cardManager;
    Character character1;
    Character character2;
    Character character3;
    Player player1;
    Player player2;
    Player player3;

    TripleFixture()
        : character1("Player1", 4, "Skill1", "Description1")
        , character2("Player2", 4, "Skill2", "Description2")
        , character3("Player3", 4, "Skill3", "Description3")
    {
        cardManager.initialize();
        state.setCardManager(&cardManager);

        player1.setPlayerId(0);
        player1.setDisplayName(QStringLiteral("Player 1"));
        player1.setCharacter(&character1);
        player2.setPlayerId(1);
        player2.setDisplayName(QStringLiteral("Player 2"));
        player2.setCharacter(&character2);
        player3.setPlayerId(2);
        player3.setDisplayName(QStringLiteral("Player 3"));
        player3.setCharacter(&character3);

        state.addPlayer(&player1);
        state.addPlayer(&player2);
        state.addPlayer(&player3);
        state.setCurrentPhase(PhaseType::Play);
    }
};

} // namespace

class ModelTest : public QObject {
    Q_OBJECT

private slots:
    void cardMetadata();
    void cardManagerLifecycle();
    void playerStateAndSignals();
    void gameStateSignals();
    void characterSkills();
    void cardRulesAndEffects();
    void areaOfEffectAndDying();
};

void ModelTest::cardMetadata()
{
    std::vector<std::unique_ptr<Card>> cards;
    cards.emplace_back(new KillCard(CardSuit::Heart, 1));
    cards.emplace_back(new DodgeCard(CardSuit::Spade, 11));
    cards.emplace_back(new PeachCard(CardSuit::Diamond, 12));
    cards.emplace_back(new WineCard(CardSuit::Club, 13));
    cards.emplace_back(new DismantleCard(CardSuit::Spade, 2));
    cards.emplace_back(new StealCard(CardSuit::Heart, 3));
    cards.emplace_back(new BountifulCard(CardSuit::Club, 4));
    cards.emplace_back(new BarbarianCard(CardSuit::Diamond, 5));
    cards.emplace_back(new VolleyCard(CardSuit::Spade, 6));
    cards.emplace_back(new PeachGardenCard(CardSuit::Heart, 7));

    std::vector<int> ids;
    for (const auto& card : cards) {
        QVERIFY(card != nullptr);
        QVERIFY(card->id() > 0);
        QVERIFY(!card->cardName().empty());
        QVERIFY(!card->description().empty());
        QVERIFY(!card->suitSymbol().empty());
        QVERIFY(!card->numberString().empty());
        ids.push_back(card->id());
    }
    std::sort(ids.begin(), ids.end());
    QVERIFY(std::adjacent_find(ids.begin(), ids.end()) == ids.end());

    QCOMPARE(cards[0]->category(), CardCategory::Basic);
    QCOMPARE(cards[3]->category(), CardCategory::Basic);
    QCOMPARE(cards[4]->category(), CardCategory::Strategy);
    QCOMPARE(cards[9]->category(), CardCategory::Strategy);
    QVERIFY(cards[0]->isBasic());
    QVERIFY(cards[4]->isStrategy());
    QVERIFY(!cards[4]->isEquipment());
    QVERIFY(cards[0]->isRed());
    QVERIFY(cards[1]->isBlack());

    QCOMPARE(QString::fromStdString(cards[0]->numberString()), QStringLiteral("A"));
    QCOMPARE(QString::fromStdString(cards[1]->numberString()), QStringLiteral("J"));
    QCOMPARE(QString::fromStdString(cards[2]->numberString()), QStringLiteral("Q"));
    QCOMPARE(QString::fromStdString(cards[3]->numberString()), QStringLiteral("K"));
    QVERIFY(!Card::cardTypeName(CardType::PeachGarden).empty());
}

void ModelTest::cardManagerLifecycle()
{
    CardManager manager;
    QSignalSpy emptySpy(&manager, &CardManager::drawPileEmpty);
    QSignalSpy reshuffledSpy(&manager, &CardManager::reshuffled);
    QSignalSpy discardedSpy(&manager, &CardManager::cardDiscarded);

    manager.initialize();
    constexpr int expectedTotal = 101;
    QCOMPARE(manager.totalCardCount(), expectedTotal);
    QCOMPARE(manager.remainingCount(), expectedTotal);

    std::map<CardType, int> counts;
    for (Card* card : manager.getDrawPile()) {
        QVERIFY(card != nullptr);
        ++counts[card->cardType()];
    }
    QCOMPARE(counts[CardType::Kill], 30);
    QCOMPARE(counts[CardType::Dodge], 15);
    QCOMPARE(counts[CardType::Peach], 8);
    QCOMPARE(counts[CardType::Wine], 5);
    QCOMPARE(counts[CardType::Dismantle], 3);
    QCOMPARE(counts[CardType::Steal], 3);
    QCOMPARE(counts[CardType::Bountiful], 3);
    QCOMPARE(counts[CardType::BarbarianInvasion], 3);
    QCOMPARE(counts[CardType::Volley], 3);
    QCOMPARE(counts[CardType::PeachGarden], 1);
    QCOMPARE(counts[CardType::Duel], 3);
    QCOMPARE(counts[CardType::Borrow], 2);
    QCOMPARE(counts[CardType::Harvest], 2);
    QCOMPARE(counts[CardType::Nullify], 3);
    QCOMPARE(counts[CardType::Happy], 3);
    QCOMPARE(counts[CardType::Famine], 2);
    QCOMPARE(counts[CardType::Lightning], 1);
    QCOMPARE(counts[CardType::Crossbow], 2);
    QCOMPARE(counts[CardType::QinglongBlade], 1);
    QCOMPARE(counts[CardType::ZhangbaSnake], 1);
    QCOMPARE(counts[CardType::KylinBow], 1);
    QCOMPARE(counts[CardType::QinggangSword], 1);
    QCOMPARE(counts[CardType::IceSword], 1);
    QCOMPARE(counts[CardType::DualSword], 1);
    QCOMPARE(counts[CardType::EightDiagrams], 2);
    QCOMPARE(counts[CardType::BenevolentShield], 1);

    auto drawn = manager.drawCards(expectedTotal);
    QCOMPARE(static_cast<int>(drawn.size()), expectedTotal);
    QCOMPARE(manager.remainingCount(), 0);
    QVERIFY(manager.drawCards(1).empty());
    QCOMPARE(emptySpy.count(), 1);

    Card* recycled = drawn.front();
    manager.discard(recycled);
    manager.discard(recycled);
    QCOMPARE(manager.discardPileCount(), 1);
    QCOMPARE(discardedSpy.count(), 1);
    QVERIFY(manager.findCardById(recycled->id()) == recycled);

    QVERIFY(manager.drawCard() == recycled);
    QCOMPARE(manager.discardPileCount(), 0);
    QCOMPARE(reshuffledSpy.count(), 1);

    manager.initialize();
    QCOMPARE(manager.totalCardCount(), expectedTotal);
    QCOMPARE(manager.remainingCount(), expectedTotal);
}

void ModelTest::playerStateAndSignals()
{
    Player player;
    Character character("Test Hero", 4, "Test Skill", "Test description");
    player.setPlayerId(7);
    player.setDisplayName(QStringLiteral("Alice"));

    QSignalSpy hpSpy(&player, &Player::hpChanged);
    QSignalSpy maxHpSpy(&player, &Player::maxHpChanged);
    QSignalSpy handAddedSpy(&player, &Player::handCardAdded);
    QSignalSpy handRemovedSpy(&player, &Player::handCardRemoved);
    QSignalSpy handChangedSpy(&player, &Player::handCardsChanged);
    QSignalSpy dyingSpy(&player, &Player::dying);
    QSignalSpy revivedSpy(&player, &Player::revived);
    QSignalSpy diedSpy(&player, &Player::died);

    player.setCharacter(&character);
    QCOMPARE(player.hp(), 4);
    QCOMPARE(player.maxHp(), 4);
    QCOMPARE(maxHpSpy.count(), 1);
    QCOMPARE(hpSpy.count(), 1);
    QCOMPARE(player.characterName(), QStringLiteral("Test Hero"));
    QCOMPARE(player.handCardLimit(), 4);
    QCOMPARE(player.hpRatio(), 1.0);

    KillCard card(CardSuit::Spade, 8);
    player.addHandCard(&card);
    player.addHandCard(&card);
    QCOMPARE(player.handCardCount(), 1);
    QCOMPARE(handAddedSpy.count(), 1);
    QCOMPARE(handChangedSpy.count(), 1);
    QVERIFY(player.hasCard(&card));
    QVERIFY(player.getRandomHandCard() == &card);

    player.removeHandCard(&card);
    player.removeHandCard(&card);
    QCOMPARE(player.handCardCount(), 0);
    QCOMPARE(handRemovedSpy.count(), 1);
    QCOMPARE(handChangedSpy.count(), 2);
    QVERIFY(player.getRandomHandCard() == nullptr);

    player.damage(1);
    QCOMPARE(player.hp(), 3);
    player.heal(10);
    QCOMPARE(player.hp(), 4);
    player.setDying(true);
    QCOMPARE(dyingSpy.count(), 1);
    player.heal(1);
    QCOMPARE(player.isDying(), false);
    QCOMPARE(revivedSpy.count(), 1);

    player.setHp(0);
    QCOMPARE(player.isAlive(), false);
    QCOMPARE(diedSpy.count(), 0);
    player.markDead();
    QCOMPARE(diedSpy.count(), 1);

    player.setHp(2);
    CrossbowCard equipment(CardSuit::Spade, 1);
    player.addEquipCard(&equipment);
    player.addJudgmentCard(&card);
    QVERIFY(player.hasEquipCards());
    QVERIFY(player.hasJudgmentCards());
    QCOMPARE(static_cast<int>(player.allSelectableCards().size()), 1);
    player.removeEquipCard(&equipment);
    player.removeJudgmentCard(&card);
    QVERIFY(!player.hasEquipCards());
    QVERIFY(!player.hasJudgmentCards());
    player.setUsedKillThisTurn(true);
    player.setWineEnhanced(true);
    player.resetTurnState();
    QVERIFY(!player.hasUsedKillThisTurn());
    QVERIFY(!player.isWineEnhanced());
}

void ModelTest::gameStateSignals()
{
    GameState state;
    Player player;
    player.setPlayerId(3);
    player.setDisplayName(QStringLiteral("Player"));
    Character character("Hero", 4, "Skill", "Description");
    player.setCharacter(&character);

    int phaseEvents = 0;
    PhaseType lastPhase = PhaseType::Prepare;
    connect(&state, &GameState::phaseChanged, this, [&](PhaseType phase) {
        ++phaseEvents;
        lastPhase = phase;
    });
    QSignalSpy currentPlayerSpy(&state, &GameState::currentPlayerChanged);
    QSignalSpy pendingClearedSpy(&state, &GameState::pendingActionCleared);
    QSignalSpy gameOverSpy(&state, &GameState::gameOver);
    int pendingCreated = 0;
    connect(&state, &GameState::pendingActionCreated, this,
            [&](const PendingActionInfo&) { ++pendingCreated; });

    state.addPlayer(&player);
    QCOMPARE(state.playerCount(), 1);
    QCOMPARE(state.player(0), &player);
    QCOMPARE(state.currentPlayer(), &player);
    QCOMPARE(static_cast<int>(state.alivePlayers().size()), 1);
    state.addPlayer(&player);
    QCOMPARE(state.playerCount(), 1);

    state.setCurrentPhase(PhaseType::Play);
    state.setCurrentPhase(PhaseType::Play);
    QCOMPARE(phaseEvents, 1);
    QCOMPARE(lastPhase, PhaseType::Play);
    state.setCurrentPlayerIndex(0);
    QCOMPARE(currentPlayerSpy.count(), 0);
    state.setCurrentPlayerIndex(99);
    QCOMPARE(currentPlayerSpy.count(), 0);

    PendingActionInfo info;
    info.source = &player;
    info.target = &player;
    info.requiredCardType = CardType::Dodge;
    info.canSkip = true;
    state.setPendingAction(info);
    QVERIFY(state.hasPendingAction());
    QCOMPARE(pendingCreated, 1);
    QCOMPARE(state.pendingActionInfo().requiredCardType, CardType::Dodge);
    state.clearPendingAction();
    QVERIFY(!state.hasPendingAction());
    QCOMPARE(pendingClearedSpy.count(), 1);

    state.incrementTurn();
    QCOMPARE(state.turnCount(), 1);
    state.setGameOver(&player);
    QVERIFY(state.isGameOver());
    QCOMPARE(state.winner(), &player);
    QCOMPARE(gameOverSpy.count(), 1);
    QCOMPARE(gameOverSpy.at(0).at(0).toInt(), 3);
}

void ModelTest::characterSkills()
{
    GameState state;
    CardManager manager;
    manager.initialize();
    state.setCardManager(&manager);
    Player player;
    player.setPlayerId(0);

    CaoCao caoCao;
    GuanYu guanYu;
    ZhangFei zhangFei;
    ZhaoYun zhaoYun;
    KillCard blackKill(CardSuit::Spade, 5);
    KillCard redKill(CardSuit::Heart, 5);
    DodgeCard dodge(CardSuit::Club, 6);

    QCOMPARE(caoCao.maxHp(), 4);
    QCOMPARE(guanYu.maxHp(), 4);
    QVERIFY(caoCao.hasSkill());
    QVERIFY(guanYu.hasSkill());
    QVERIFY(zhangFei.hasSkill());
    QVERIFY(zhaoYun.hasSkill());
    QVERIFY(caoCao.triggerCondition(GameEvent::OnDamage, &state, &player));
    QVERIFY(!caoCao.triggerCondition(GameEvent::OnCardPlayed, &state, &player));
    QCOMPARE(guanYu.skillTransformCard(&redKill), CardType::Kill);
    QCOMPARE(guanYu.skillTransformCard(&blackKill), CardType::Kill);
    QCOMPARE(zhaoYun.skillTransformCard(&blackKill), CardType::Dodge);
    QCOMPARE(zhaoYun.skillTransformCard(&dodge), CardType::Kill);

    player.setCharacter(&caoCao);
    QSignalSpy skillSpy(&caoCao, &Character::skillTriggered);
    int before = player.handCardCount();
    caoCao.triggerSkill(&state, &player);
    QCOMPARE(player.handCardCount(), before + 1);
    QCOMPARE(skillSpy.count(), 1);

    player.setCharacter(&zhangFei);
    player.setUsedKillThisTurn(true);
    QVERIFY(GameRule::canPlayKill(&state, &player));
}

void ModelTest::cardRulesAndEffects()
{
    DuelFixture fixture;
    KillCard kill(CardSuit::Spade, 7);
    DodgeCard dodge(CardSuit::Heart, 6);
    PeachCard peach(CardSuit::Diamond, 3);

    QVERIFY(kill.canUse(&fixture.state, &fixture.player1));
    QVERIFY(!dodge.canUse(&fixture.state, &fixture.player1));
    QVERIFY(!kill.canTarget(&fixture.state, &fixture.player1, &fixture.player1));
    QVERIFY(kill.canTarget(&fixture.state, &fixture.player1, &fixture.player2));
    QCOMPARE(static_cast<int>(kill.getValidTargets(&fixture.state, &fixture.player1).size()), 1);
    QCOMPARE(kill.execute(&fixture.state, &fixture.player1, {&fixture.player2}), ActionResult::RequiresDodge);
    QVERIFY(fixture.state.hasPendingAction());
    fixture.player2.addHandCard(&dodge);
    QVERIFY(GameRule::hasDodgeToRespond(&fixture.player2));
    GameRule::handleKillResponse(&fixture.state, &fixture.player2, &dodge);
    QVERIFY(!fixture.state.hasPendingAction());
    QVERIFY(!fixture.player2.hasCard(&dodge));
    QCOMPARE(fixture.cardManager.discardPileCount(), 1);

    fixture.player2.damage(1);
    QVERIFY(peach.canUse(&fixture.state, &fixture.player2));
    QVERIFY(peach.canTarget(&fixture.state, &fixture.player2, &fixture.player2));
    QVERIFY(!peach.canTarget(&fixture.state, &fixture.player2, &fixture.player1));
    QCOMPARE(peach.execute(&fixture.state, &fixture.player2, {&fixture.player2}), ActionResult::Completed);
    QCOMPARE(fixture.player2.hp(), 4);

    DuelFixture wineFixture;
    WineCard wine(CardSuit::Club, 9);
    QVERIFY(wine.canUse(&wineFixture.state, &wineFixture.player1));
    QCOMPARE(wine.execute(&wineFixture.state, &wineFixture.player1, {}), ActionResult::Completed);
    QVERIFY(wineFixture.player1.isWineEnhanced());
    GameRule::executeKill(&wineFixture.state, &wineFixture.player1, &wineFixture.player2);
    GameRule::handleKillResponse(&wineFixture.state, &wineFixture.player2, nullptr);
    QCOMPARE(wineFixture.player2.hp(), 2);
    QVERIFY(!wineFixture.player1.isWineEnhanced());

    DuelFixture strategyFixture;
    KillCard victimCard(CardSuit::Spade, 2);
    strategyFixture.player2.addHandCard(&victimCard);
    DismantleCard dismantle(CardSuit::Diamond, 4);
    QVERIFY(dismantle.canTarget(&strategyFixture.state, &strategyFixture.player1,
                                &strategyFixture.player2));
    QCOMPARE(dismantle.execute(&strategyFixture.state, &strategyFixture.player1,
                               {&strategyFixture.player2}), ActionResult::Completed);
    QCOMPARE(strategyFixture.player2.handCardCount(), 0);
    QCOMPARE(strategyFixture.cardManager.discardPileCount(), 1);

    KillCard stolenCard(CardSuit::Club, 10);
    strategyFixture.player2.addHandCard(&stolenCard);
    StealCard steal(CardSuit::Heart, 5);
    QCOMPARE(steal.execute(&strategyFixture.state, &strategyFixture.player1,
                           {&strategyFixture.player2}), ActionResult::Completed);
    QVERIFY(strategyFixture.player1.hasCard(&stolenCard));
    QVERIFY(!strategyFixture.player2.hasCard(&stolenCard));

    DuelFixture noTargetFixture;
    DismantleCard noTargetDismantle(CardSuit::Spade, 6);
    StealCard noTargetSteal(CardSuit::Heart, 7);
    noTargetFixture.player1.addHandCard(&noTargetDismantle);
    noTargetFixture.player1.addHandCard(&noTargetSteal);
    QVERIFY(!noTargetDismantle.canUse(&noTargetFixture.state,
                                       &noTargetFixture.player1));
    QVERIFY(!noTargetSteal.canUse(&noTargetFixture.state,
                                  &noTargetFixture.player1));

    int beforeDraw = strategyFixture.player1.handCardCount();
    BountifulCard bountiful(CardSuit::Spade, 11);
    QCOMPARE(bountiful.execute(&strategyFixture.state, &strategyFixture.player1, {}), ActionResult::Completed);
    QCOMPARE(strategyFixture.player1.handCardCount(), beforeDraw + 2);

    strategyFixture.player1.damage(1);
    strategyFixture.player2.damage(2);
    PeachGardenCard garden(CardSuit::Heart, 12);
    QCOMPARE(garden.execute(&strategyFixture.state, &strategyFixture.player1, {}), ActionResult::Completed);
    QCOMPARE(strategyFixture.player1.hp(), 4);
    QCOMPARE(strategyFixture.player2.hp(), 3);
}

void ModelTest::areaOfEffectAndDying()
{
    TripleFixture fixture;
    BarbarianCard barbarian(CardSuit::Spade, 1);
    KillCard killResponse(CardSuit::Club, 2);
    fixture.player2.addHandCard(&killResponse);

    QCOMPARE(barbarian.execute(&fixture.state, &fixture.player1, {}), ActionResult::RequiresKill);
    QCOMPARE(fixture.state.pendingActionInfo().target, &fixture.player2);
    QCOMPARE(fixture.state.pendingActionInfo().remainingTargets.size(), static_cast<size_t>(1));
    GameRule::handleAoeKillResponse(&fixture.state, &fixture.player2, &killResponse);
    QVERIFY(fixture.state.hasPendingAction());
    QCOMPARE(fixture.state.pendingActionInfo().target, &fixture.player3);
    GameRule::handleAoeSkipResponse(&fixture.state, &fixture.player3);
    QVERIFY(!fixture.state.hasPendingAction());
    QCOMPARE(fixture.player3.hp(), 3);

    VolleyCard volley(CardSuit::Diamond, 3);
    DodgeCard dodgeResponse(CardSuit::Heart, 4);
    fixture.player2.addHandCard(&dodgeResponse);
    QCOMPARE(volley.execute(&fixture.state, &fixture.player1, {}), ActionResult::RequiresDodge);
    GameRule::handleAoeDodgeResponse(&fixture.state, &fixture.player2, &dodgeResponse);
    QVERIFY(fixture.state.hasPendingAction());
    QCOMPARE(fixture.state.pendingActionInfo().target, &fixture.player3);
    GameRule::handleAoeSkipResponse(&fixture.state, &fixture.player3);
    QVERIFY(!fixture.state.hasPendingAction());
    QCOMPARE(fixture.player3.hp(), 2);

    TripleFixture fatalAoe;
    fatalAoe.player2.setHp(1);
    BarbarianCard fatalBarbarian(CardSuit::Spade, 9);
    PeachCard rescueAoe(CardSuit::Diamond, 10);
    fatalAoe.player1.addHandCard(&rescueAoe);
    QCOMPARE(fatalBarbarian.execute(&fatalAoe.state, &fatalAoe.player1, {}),
             ActionResult::RequiresKill);
    GameRule::handleAoeSkipResponse(&fatalAoe.state, &fatalAoe.player2);
    QVERIFY(fatalAoe.player2.isDying());
    QVERIFY(fatalAoe.state.hasPendingAction());
    QCOMPARE(fatalAoe.state.pendingActionInfo().requiredCardType, CardType::Peach);
    QCOMPARE(fatalAoe.state.pendingActionInfo().continuationTargets.size(), 1);
    QVERIFY(GameRule::handleDyingPeach(&fatalAoe.state, &fatalAoe.player2,
                                       &fatalAoe.player1, &rescueAoe));
    QVERIFY(!fatalAoe.player2.isDying());
    QVERIFY(fatalAoe.state.hasPendingAction());
    QCOMPARE(fatalAoe.state.pendingActionInfo().target, &fatalAoe.player3);
    QCOMPARE(fatalAoe.state.pendingActionInfo().requiredCardType, CardType::Kill);

    DuelFixture dyingFixture;
    dyingFixture.player2.setHp(1);
    PeachCard saveCard(CardSuit::Diamond, 8);
    dyingFixture.player1.addHandCard(&saveCard);
    QSignalSpy dyingSpy(&dyingFixture.player2, &Player::dying);
    GameRule::dealDamage(&dyingFixture.state, &dyingFixture.player2, 1,
                         &dyingFixture.player1);
    QVERIFY(dyingFixture.player2.isDying());
    QCOMPARE(dyingSpy.count(), 1);
    QVERIFY(dyingFixture.state.hasPendingAction());
    QCOMPARE(dyingFixture.state.pendingActionInfo().source, &dyingFixture.player1);
    QCOMPARE(dyingFixture.state.pendingActionInfo().target, &dyingFixture.player2);
    QVERIFY(GameRule::hasPeachToSave(&dyingFixture.player1, &dyingFixture.player2));
    QVERIFY(GameRule::handleDyingPeach(&dyingFixture.state, &dyingFixture.player2,
                                       &dyingFixture.player1, &saveCard));
    QVERIFY(!dyingFixture.player2.isDying());
    QCOMPARE(dyingFixture.player2.hp(), 1);
    QVERIFY(!dyingFixture.state.hasPendingAction());

    DuelFixture deathFixture;
    deathFixture.player2.setHp(0);
    QSignalSpy diedSpy(&deathFixture.player2, &Player::died);
    GameRule::checkDeath(&deathFixture.state, &deathFixture.player2);
    QVERIFY(deathFixture.state.isGameOver());
    QCOMPARE(deathFixture.state.winner(), &deathFixture.player1);
    QCOMPARE(diedSpy.count(), 1);

    DuelFixture fatalFixture;
    fatalFixture.player2.setHp(1);
    GameRule::executeKill(&fatalFixture.state, &fatalFixture.player1,
                          &fatalFixture.player2);
    GameRule::handleKillResponse(&fatalFixture.state, &fatalFixture.player2, nullptr);
    QVERIFY(fatalFixture.state.hasPendingAction());
}

QTEST_APPLESS_MAIN(ModelTest)

#include "model_test.moc"
