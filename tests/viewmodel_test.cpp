#include <QtTest>

#include <algorithm>
#include <vector>

#include "ActionViewModel.h"
#include "Card.h"
#include "CardManager.h"
#include "Character.h"
#include "GameRule.h"
#include "GameState.h"
#include "GameViewModel.h"
#include "Player.h"

namespace {

struct ActionFixture {
    GameState state;
    CardManager cardManager;
    Character character1;
    Character character2;
    Character character3;
    Player player1;
    Player player2;
    Player player3;
    ActionViewModel actionVM;

    ActionFixture()
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
        actionVM.setGameState(&state);
    }
};

bool containsId(const std::vector<int>& ids, int id)
{
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

} // namespace

class ViewModelTest : public QObject {
    Q_OBJECT

private slots:
    void actionQueriesAndPlay();
    void responseValidation();
    void duelAlternatingResponses();
    void delayedStrategyAndNullifyZones();
    void activeSkillValidation();
    void discardAndTargetValidation();
    void gameViewModelInitialization();
    void gameViewModelActiveSkillData();
    void gameViewModelJudgmentResolution();
    void gameViewModelPhaseProgression();
    void gameViewModelPendingDto();
    void dyingResponseRouting();
    void phaseNames();
};

void ViewModelTest::actionQueriesAndPlay()
{
    ActionFixture fixture;
    KillCard kill(CardSuit::Spade, 7);
    DodgeCard dodge(CardSuit::Heart, 6);
    WineCard wine(CardSuit::Club, 9);
    fixture.player1.addHandCard(&kill);
    fixture.player1.addHandCard(&dodge);
    fixture.player1.addHandCard(&wine);

    const auto playable = fixture.actionVM.getPlayableCardIds(0);
    QVERIFY(containsId(playable, kill.id()));
    QVERIFY(containsId(playable, wine.id()));
    QVERIFY(!containsId(playable, dodge.id()));
    QVERIFY(fixture.actionVM.isOwnCard(kill.id(), 0));
    QVERIFY(!fixture.actionVM.isOwnCard(kill.id(), 1));
    QVERIFY(fixture.actionVM.canPlayCard(kill.id(), 0));
    QVERIFY(!fixture.actionVM.canPlayCard(kill.id(), 1));

    const auto targets = fixture.actionVM.getValidTargetIds(kill.id(), 0);
    QCOMPARE(static_cast<int>(targets.size()), 2);
    QVERIFY(containsId(targets, 1));
    QVERIFY(containsId(targets, 2));

    QCOMPARE(fixture.actionVM.playCard(kill.id(), 0, {1}), ActionResult::RequiresDodge);
    QVERIFY(fixture.state.hasPendingAction());
    QVERIFY(!fixture.player1.hasCard(&kill));
    QCOMPARE(fixture.cardManager.discardPileCount(), 1);
    QCOMPARE(fixture.state.pendingActionInfo().target, &fixture.player2);
}

void ViewModelTest::responseValidation()
{
    ActionFixture fixture;
    KillCard attack(CardSuit::Spade, 7);
    DodgeCard dodge(CardSuit::Heart, 6);
    PeachCard peach(CardSuit::Diamond, 3);
    WineCard wine(CardSuit::Club, 9);
    fixture.player1.addHandCard(&attack);
    fixture.player2.addHandCard(&dodge);
    fixture.player2.addHandCard(&peach);
    fixture.player2.addHandCard(&wine);

    QVERIFY(fixture.actionVM.playCard(attack.id(), 0, {1}) == ActionResult::RequiresDodge);
    const auto dodgeIds = fixture.actionVM.getResponseCardIds(1, CardType::Dodge);
    QCOMPARE(static_cast<int>(dodgeIds.size()), 1);
    QVERIFY(fixture.actionVM.isValidResponseCard(dodge.id(), 1, CardType::Dodge));
    QVERIFY(!fixture.actionVM.isValidResponseCard(peach.id(), 1, CardType::Dodge));
    fixture.actionVM.respondCard(dodge.id(), 1);
    QVERIFY(!fixture.state.hasPendingAction());
    QVERIFY(!fixture.player2.hasCard(&dodge));

    ActionFixture wrongType;
    KillCard attack2(CardSuit::Spade, 8);
    PeachCard wrongCard(CardSuit::Diamond, 4);
    wrongType.player1.addHandCard(&attack2);
    wrongType.player2.addHandCard(&wrongCard);
    wrongType.actionVM.playCard(attack2.id(), 0, {1});
    wrongType.actionVM.respondCard(wrongCard.id(), 1);
    QVERIFY(wrongType.state.hasPendingAction());
    QVERIFY(wrongType.player2.hasCard(&wrongCard));

    ActionFixture wrongOwner;
    KillCard attack3(CardSuit::Spade, 9);
    DodgeCard cardOwnedByAttacker(CardSuit::Club, 10);
    wrongOwner.player1.addHandCard(&attack3);
    wrongOwner.player1.addHandCard(&cardOwnedByAttacker);
    wrongOwner.actionVM.playCard(attack3.id(), 0, {1});
    wrongOwner.actionVM.respondCard(cardOwnedByAttacker.id(), 1);
    QVERIFY(wrongOwner.state.hasPendingAction());
    QVERIFY(wrongOwner.player1.hasCard(&cardOwnedByAttacker));

    ActionFixture dyingResponse;
    WineCard rescueWine(CardSuit::Heart, 11);
    dyingResponse.player2.addHandCard(&rescueWine);
    const auto peachResponses = dyingResponse.actionVM.getResponseCardIds(1, CardType::Peach);
    QVERIFY(containsId(peachResponses, rescueWine.id()));

    ActionFixture dyingAction;
    dyingAction.player2.setHp(1);
    PeachCard rescuePeach(CardSuit::Diamond, 12);
    dyingAction.player1.addHandCard(&rescuePeach);
    GameRule::dealDamage(&dyingAction.state, &dyingAction.player2, 1,
                         &dyingAction.player1);
    QVERIFY(dyingAction.state.hasPendingAction());
    QCOMPARE(dyingAction.state.pendingActionInfo().source, &dyingAction.player1);
    dyingAction.actionVM.respondCard(rescuePeach.id(), dyingAction.player1.playerId());
    QCOMPARE(dyingAction.player2.hp(), 1);
    QVERIFY(!dyingAction.player2.isDying());
    QVERIFY(!dyingAction.state.hasPendingAction());
    QVERIFY(!dyingAction.player1.hasCard(&rescuePeach));

    ActionFixture aoe;
    BarbarianCard barbarian(CardSuit::Spade, 1);
    aoe.player1.addHandCard(&barbarian);
    QCOMPARE(aoe.actionVM.playCard(barbarian.id(), 0, {}), ActionResult::RequiresKill);
    QVERIFY(aoe.actionVM.canSkipPendingAction());
    aoe.actionVM.skipResponse(1);
    QVERIFY(aoe.state.hasPendingAction());
    QCOMPARE(aoe.state.pendingActionInfo().target, &aoe.player3);
}

void ViewModelTest::duelAlternatingResponses()
{
    ActionFixture fixture;
    DuelCard duel(CardSuit::Spade, 1);
    KillCard player1Kill(CardSuit::Heart, 7);
    KillCard player2Kill(CardSuit::Club, 8);
    fixture.player1.addHandCard(&duel);
    fixture.player1.addHandCard(&player1Kill);
    fixture.player2.addHandCard(&player2Kill);

    QCOMPARE(fixture.actionVM.playCard(duel.id(), 0, {1}), ActionResult::RequiresKill);
    QVERIFY(fixture.state.hasPendingAction());
    QCOMPARE(fixture.state.pendingActionInfo().target, &fixture.player2);
    QCOMPARE(fixture.state.pendingActionInfo().sourceCard, &duel);

    fixture.actionVM.respondCard(player2Kill.id(), 1);
    QVERIFY(fixture.state.hasPendingAction());
    QCOMPARE(fixture.state.pendingActionInfo().source, &fixture.player2);
    QCOMPARE(fixture.state.pendingActionInfo().target, &fixture.player1);

    fixture.actionVM.respondCard(player1Kill.id(), 0);
    QVERIFY(fixture.state.hasPendingAction());
    QCOMPARE(fixture.state.pendingActionInfo().source, &fixture.player1);
    QCOMPARE(fixture.state.pendingActionInfo().target, &fixture.player2);

    const int hpBefore = fixture.player2.hp();
    fixture.actionVM.skipResponse(1, true);
    QVERIFY(!fixture.state.hasPendingAction());
    QCOMPARE(fixture.player2.hp(), hpBefore - 1);
}

void ViewModelTest::delayedStrategyAndNullifyZones()
{
    ActionFixture direct;
    HappyCard happy(CardSuit::Heart, 6);
    direct.player1.addHandCard(&happy);
    QCOMPARE(direct.actionVM.playCard(happy.id(), 0, {1}), ActionResult::Completed);
    QVERIFY(!direct.player1.hasCard(&happy));
    QVERIFY(direct.player2.hasJudgmentCards());
    QCOMPARE(direct.player2.judgmentCards().front(), &happy);
    QCOMPARE(direct.cardManager.discardPileCount(), 0);

    ActionFixture cancelled;
    HappyCard nullifiedHappy(CardSuit::Spade, 6);
    NullifyCard nullify(CardSuit::Club, 12);
    cancelled.player1.addHandCard(&nullifiedHappy);
    cancelled.player2.addHandCard(&nullify);

    cancelled.actionVM.playCard(nullifiedHappy.id(), 0, {1});
    QVERIFY(cancelled.state.hasPendingAction());
    QCOMPARE(cancelled.state.pendingActionInfo().requiredCardType, CardType::Nullify);
    QVERIFY(!cancelled.player2.hasJudgmentCards());
    QCOMPARE(cancelled.cardManager.discardPileCount(), 0);

    cancelled.actionVM.respondCard(nullify.id(), 1);
    QVERIFY(!cancelled.state.hasPendingAction());
    QVERIFY(!cancelled.player2.hasJudgmentCards());
    QVERIFY(!cancelled.player2.hasCard(&nullify));
    QCOMPARE(cancelled.cardManager.discardPileCount(), 2);
}

void ViewModelTest::activeSkillValidation()
{
    ActionFixture fixture;
    SunQuan sunQuan;
    fixture.player1.setCharacter(&sunQuan);
    KillCard first(CardSuit::Spade, 7);
    DodgeCard second(CardSuit::Heart, 6);
    fixture.player1.addHandCard(&first);
    fixture.player1.addHandCard(&second);

    QVERIFY(fixture.actionVM.canUseActiveSkill(0));
    QVERIFY(!fixture.actionVM.canUseActiveSkill(1));

    QVERIFY(!fixture.actionVM.useActiveSkill(0, {first.id(), first.id()}));
    QVERIFY(fixture.player1.hasCard(&first));
    QVERIFY(!fixture.player1.hasUsedActiveSkillThisTurn());

    QVERIFY(!fixture.actionVM.useActiveSkill(0, {first.id(), 999999}));
    QVERIFY(fixture.player1.hasCard(&first));
    QVERIFY(fixture.player1.hasCard(&second));

    const int handBefore = fixture.player1.handCardCount();
    QVERIFY(fixture.actionVM.useActiveSkill(0, {first.id(), second.id()}));
    QCOMPARE(fixture.player1.handCardCount(), handBefore);
    QVERIFY(fixture.player1.hasUsedActiveSkillThisTurn());
    QVERIFY(!fixture.actionVM.canUseActiveSkill(0));
    QVERIFY(!fixture.actionVM.useActiveSkill(0, {
        fixture.player1.handCards().front()->id()
    }));

    fixture.player1.resetTurnState();
    QVERIFY(fixture.actionVM.canUseActiveSkill(0));

    PendingActionInfo pending;
    pending.source = &fixture.player1;
    pending.target = &fixture.player2;
    pending.requiredCardType = CardType::Dodge;
    fixture.state.setPendingAction(pending);
    QVERIFY(!fixture.actionVM.canUseActiveSkill(0));
    fixture.state.clearPendingAction();

    fixture.state.setCurrentPhase(PhaseType::Discard);
    QVERIFY(!fixture.actionVM.canUseActiveSkill(0));
}

void ViewModelTest::discardAndTargetValidation()
{
    ActionFixture fixture;
    KillCard card(CardSuit::Spade, 7);
    fixture.player1.addHandCard(&card);

    fixture.actionVM.discardCard(card.id(), 1);
    QVERIFY(fixture.player1.hasCard(&card));
    QCOMPARE(fixture.cardManager.discardPileCount(), 0);

    ActionFixture noTarget;
    DismantleCard noTargetCard(CardSuit::Spade, 5);
    noTarget.player1.addHandCard(&noTargetCard);
    QVERIFY(!noTarget.actionVM.canPlayCard(noTargetCard.id(), 0));
    noTarget.actionVM.onPlayCardRequested(noTargetCard.id(), 0);
    QVERIFY(noTarget.player1.hasCard(&noTargetCard));
    QCOMPARE(noTarget.cardManager.discardPileCount(), 0);

    ActionFixture wrongPlayOwner;
    KillCard opponentCard(CardSuit::Heart, 8);
    wrongPlayOwner.player2.addHandCard(&opponentCard);
    wrongPlayOwner.actionVM.playCard(opponentCard.id(), 0, {1});
    QVERIFY(!wrongPlayOwner.state.hasPendingAction());
    QVERIFY(wrongPlayOwner.player2.hasCard(&opponentCard));

    ActionFixture targetSelection;
    DismantleCard dismantle(CardSuit::Diamond, 4);
    KillCard targetCard1(CardSuit::Spade, 2);
    KillCard targetCard2(CardSuit::Club, 3);
    targetSelection.player1.addHandCard(&dismantle);
    targetSelection.player2.addHandCard(&targetCard1);
    targetSelection.player3.addHandCard(&targetCard2);
    const auto targetIds = targetSelection.actionVM.getValidTargetIds(dismantle.id(), 0);
    QCOMPARE(static_cast<int>(targetIds.size()), 2);
    QSignalSpy selectionStartedSpy(&targetSelection.actionVM,
                                   &ActionViewModel::targetSelectionStarted);
    QSignalSpy selectionFinishedSpy(&targetSelection.actionVM,
                                    &ActionViewModel::targetSelectionFinished);
    targetSelection.actionVM.onPlayCardRequested(dismantle.id(), 0);
    QVERIFY(targetSelection.player1.hasCard(&dismantle));
    QCOMPARE(selectionStartedSpy.count(), 1);
    QCOMPARE(selectionStartedSpy.at(0).at(0).value<QVector<int>>().size(), 2);
    targetSelection.actionVM.onTargetSelected(2);
    QCOMPARE(selectionFinishedSpy.count(), 1);
    QVERIFY(!targetSelection.player1.hasCard(&dismantle));
    QVERIFY(targetSelection.player2.hasCard(&targetCard1));
    QVERIFY(!targetSelection.player3.hasCard(&targetCard2));
}

void ViewModelTest::gameViewModelInitialization()
{
    GameViewModel viewModel;
    std::vector<PlayerData> playerUpdates;
    std::vector<CardList> handUpdates;
    int phaseUpdates = 0;
    connect(&viewModel, &GameViewModel::playerDataUpdated, this,
            [&](int, const PlayerData& data) { playerUpdates.push_back(data); });
    connect(&viewModel, &GameViewModel::handCardsUpdated, this,
            [&](int, const CardList& cards) { handUpdates.push_back(cards); });
    connect(&viewModel, &GameViewModel::phaseChanged, this,
            [&](PhaseType) { ++phaseUpdates; });

    viewModel.startGame(0, 1);
    QVERIFY(viewModel.actionVM() != nullptr);
    QVERIFY(viewModel.gameState() != nullptr);
    QCOMPARE(viewModel.gameState()->playerCount(), 2);
    QCOMPARE(viewModel.gameState()->currentPhase(), PhaseType::Prepare);
    QCOMPARE(viewModel.currentPlayerId(), 0);
    QCOMPARE(viewModel.gameState()->player(0)->handCardCount(), 4);
    QCOMPARE(viewModel.gameState()->player(1)->handCardCount(), 4);
    QCOMPARE(viewModel.gameState()->cardManager()->remainingCount(), 87);
    QCOMPARE(phaseUpdates, 1);
    QCOMPARE(static_cast<int>(playerUpdates.size()), 2);
    QCOMPARE(static_cast<int>(handUpdates.size()), 2);

    for (const PlayerData& data : playerUpdates) {
        QVERIFY(!data.displayName.isEmpty());
        QVERIFY(!data.characterName.isEmpty());
        QCOMPARE(data.hp, 4);
        QCOMPARE(data.maxHp, 4);
        QCOMPARE(data.handCardCount, 4);
        QVERIFY(data.isAlive);
    }
    for (const CardList& cards : handUpdates) {
        QCOMPARE(cards.size(), 4);
        for (const CardData& card : cards) {
            QVERIFY(card.cardId > 0);
            QVERIFY(card.ownerId == 0 || card.ownerId == 1);
            QVERIFY(!card.cardName.isEmpty());
            QVERIFY(!card.numberString.isEmpty());
        }
    }
}

void ViewModelTest::gameViewModelActiveSkillData()
{
    GameViewModel viewModel;
    QVector<PlayerData> player0Updates;
    connect(&viewModel, &GameViewModel::playerDataUpdated, this,
            [&](int playerId, const PlayerData& data) {
        if (playerId == 0) player0Updates.append(data);
    });

    QVERIFY(viewModel.startGame(4, 0)); // 玩家 0：孙权
    QVERIFY(!player0Updates.isEmpty());
    QCOMPARE(player0Updates.last().canUseActiveSkill, false);

    viewModel.advancePhase(); // Prepare -> Judge
    viewModel.advancePhase(); // Judge -> Draw
    viewModel.advancePhase(); // Draw -> Play
    QVERIFY(!player0Updates.isEmpty());
    QCOMPARE(player0Updates.last().canUseActiveSkill, true);

    HappyCard delayed(CardSuit::Heart, 6);
    viewModel.gameState()->player(0)->addJudgmentCard(&delayed);
    QVERIFY(!player0Updates.last().judgmentCards.isEmpty());
    QCOMPARE(player0Updates.last().judgmentCards.front().cardId, delayed.id());
    QCOMPARE(player0Updates.last().judgmentCards.front().cardType, CardType::Happy);

    const int selectedId = viewModel.gameState()->player(0)->handCards().front()->id();
    QVERIFY(viewModel.actionVM()->useActiveSkill(0, {selectedId}));
    QCOMPARE(player0Updates.last().canUseActiveSkill, false);
}

void ViewModelTest::gameViewModelJudgmentResolution()
{
    GameViewModel viewModel;
    QVERIFY(viewModel.startGame(0, 1));
    Player* current = viewModel.gameState()->player(0);
    HappyCard delayed(CardSuit::Spade, 6);
    current->addJudgmentCard(&delayed);

    const auto& drawPile = viewModel.gameState()->cardManager()->getDrawPile();
    QVERIFY(!drawPile.empty());
    Card* expectedResult = drawPile.back();
    const bool expectedEffective = expectedResult->suit() != CardSuit::Heart;

    QVector<CardData> judgeCards;
    QVector<bool> effectiveResults;
    connect(&viewModel, &GameViewModel::judgmentPerformed, this,
            [&](const CardData& card, const QString&, bool effective) {
        judgeCards.append(card);
        effectiveResults.append(effective);
    });

    viewModel.advancePhase(); // Prepare -> Judge
    viewModel.advancePhase(); // 执行判定，等待展示定时器
    QCOMPARE(viewModel.gameState()->currentPhase(), PhaseType::Judge);
    QCOMPARE(judgeCards.size(), 1);
    QCOMPARE(judgeCards.front().cardId, expectedResult->id());
    QCOMPARE(effectiveResults.front(), expectedEffective);
    QVERIFY(!current->hasJudgmentCards());
    QCOMPARE(viewModel.gameState()->cardManager()->discardPileCount(), 2);
}

void ViewModelTest::gameViewModelPhaseProgression()
{
    GameViewModel viewModel;
    viewModel.startGame(2, 3);

    viewModel.advancePhase();
    QCOMPARE(viewModel.gameState()->currentPhase(), PhaseType::Judge);
    viewModel.advancePhase();
    QCOMPARE(viewModel.gameState()->currentPhase(), PhaseType::Draw);
    QCOMPARE(viewModel.gameState()->player(0)->handCardCount(), 4);
    viewModel.advancePhase();
    QCOMPARE(viewModel.gameState()->currentPhase(), PhaseType::Play);
    QCOMPARE(viewModel.gameState()->player(0)->handCardCount(), 6);

    viewModel.endPlayPhase();
    QCOMPARE(viewModel.gameState()->currentPhase(), PhaseType::Discard);
    QCOMPARE(viewModel.actionVM()->getDiscardCount(0), 2);

    int first = viewModel.gameState()->player(0)->handCards().front()->id();
    viewModel.onDiscardCardRequested(first, 0);
    QCOMPARE(viewModel.gameState()->currentPhase(), PhaseType::Discard);
    int second = viewModel.gameState()->player(0)->handCards().front()->id();
    viewModel.onDiscardCardRequested(second, 0);
    QCOMPARE(viewModel.actionVM()->getDiscardCount(0), 0);
    QCOMPARE(viewModel.gameState()->currentPhase(), PhaseType::End);

    viewModel.advancePhase();
    QCOMPARE(viewModel.gameState()->currentPlayerIndex(), 1);
    QCOMPARE(viewModel.currentPlayerId(), 1);
    QCOMPARE(viewModel.gameState()->turnCount(), 1);
    QCOMPARE(viewModel.gameState()->currentPhase(), PhaseType::Prepare);
}

void ViewModelTest::gameViewModelPendingDto()
{
    GameViewModel viewModel;
    std::vector<PendingActionData> pending;
    int cleared = 0;
    connect(&viewModel, &GameViewModel::pendingActionCreated, this,
            [&](const PendingActionData& info) { pending.push_back(info); });
    connect(&viewModel, &GameViewModel::pendingActionCleared, this,
            [&]() { ++cleared; });

    viewModel.startGame(0, 1);
    viewModel.gameState()->setCurrentPhase(PhaseType::Play);
    KillCard attack(CardSuit::Spade, 7);
    DodgeCard dodge(CardSuit::Heart, 6);
    viewModel.gameState()->player(0)->addHandCard(&attack);
    viewModel.gameState()->player(1)->addHandCard(&dodge);

    QCOMPARE(viewModel.actionVM()->playCard(attack.id(), 0, {1}), ActionResult::RequiresDodge);
    QCOMPARE(static_cast<int>(pending.size()), 1);
    QCOMPARE(pending.front().sourceId, 0);
    QCOMPARE(pending.front().targetId, 1);
    QCOMPARE(pending.front().requiredCardType, CardType::Dodge);
    QVERIFY(!pending.front().description.isEmpty());
    QVERIFY(viewModel.actionVM()->isValidResponseCard(dodge.id(), 1, CardType::Dodge));

    viewModel.actionVM()->respondCard(dodge.id(), 1);
    QCOMPARE(cleared, 1);
    QVERIFY(!viewModel.gameState()->hasPendingAction());
}

void ViewModelTest::dyingResponseRouting()
{
    GameViewModel viewModel;
    std::vector<PendingActionData> pending;
    connect(&viewModel, &GameViewModel::pendingActionCreated, this,
            [&](const PendingActionData& info) { pending.push_back(info); });

    viewModel.startGame(0, 1);
    Player* dying = viewModel.gameState()->player(1);
    Player* savior = viewModel.gameState()->player(0);
    dying->setHp(1);
    const auto dyingHand = dying->handCards();
    for (Card* card : dyingHand) {
        if (card && (card->cardType() == CardType::Peach || card->cardType() == CardType::Wine))
            dying->removeHandCard(card);
    }
    PeachCard peach(CardSuit::Diamond, 8);
    savior->addHandCard(&peach);

    GameRule::dealDamage(viewModel.gameState(), dying, 1, savior);
    QCOMPARE(static_cast<int>(pending.size()), 1);
    QCOMPARE(pending.front().sourceId, savior->playerId());
    QCOMPARE(pending.front().targetId, dying->playerId());
}

void ViewModelTest::phaseNames()
{
    const PhaseType phases[] = {
        PhaseType::Prepare, PhaseType::Judge, PhaseType::Draw,
        PhaseType::Play, PhaseType::Discard, PhaseType::End
    };
    for (PhaseType phase : phases)
        QVERIFY(!GameViewModel::phaseName(phase).isEmpty());
}

QTEST_APPLESS_MAIN(ViewModelTest)

#include "viewmodel_test.moc"
