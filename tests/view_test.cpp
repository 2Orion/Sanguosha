#include <QtTest>

#include <QButtonGroup>
#include <QLabel>
#include <QMouseEvent>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QStackedWidget>

#include <algorithm>

#include "ActionPanelWidget.h"
#include "CardWidget.h"
#include "GameBoardWidget.h"
#include "GameViewModel.h"
#include "HandCardAreaWidget.h"
#include "MainWindow.h"
#include "PlayerInfoWidget.h"
#include "SGSApp.h"

namespace {

CardData makeCardData(int id, const QString& name)
{
    CardData data;
    data.cardId = id;
    data.cardType = CardType::Kill;
    data.suit = CardSuit::Heart;
    data.number = 1;
    data.cardName = name;
    data.description = QStringLiteral("A test card");
    data.color = CardColor::Red;
    data.isBasic = true;
    data.suitSymbol = QStringLiteral("H");
    data.numberString = QStringLiteral("A");
    data.isPlayable = true;
    data.ownerId = 0;
    return data;
}

QPushButton* visibleButton(ActionPanelWidget* panel)
{
    for (QPushButton* button : panel->findChildren<QPushButton*>()) {
        if (button->isVisible()) return button;
    }
    return nullptr;
}

class ExposedPlayerInfoWidget : public PlayerInfoWidget {
public:
    using PlayerInfoWidget::mousePressEvent;
};

} // namespace

class ViewTest : public QObject {
    Q_OBJECT

private slots:
    void cardWidgetInteraction();
    void handCardAreaInteraction();
    void playerInfoDisplayAndClick();
    void actionPanelStates();
    void gameBoardRouting();
    void targetSelectionState();
    void mainWindowAndAppComposition();
};

void ViewTest::cardWidgetInteraction()
{
    CardWidget widget(true);
    widget.show();
    widget.setDisplayData(makeCardData(42, QStringLiteral("Test Card")));
    QCOMPARE(widget.cardId(), 42);
    QCOMPARE(widget.toolTip(), QStringLiteral("Test Card\nA test card"));
    QVERIFY(widget.width() == CardWidget::CARD_WIDTH);
    QVERIFY(widget.height() == CardWidget::CARD_HEIGHT);

    QSignalSpy clickedSpy(&widget, &CardWidget::clicked);
    QSignalSpy doubleClickedSpy(&widget, &CardWidget::doubleClicked);
    QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, QPoint(20, 20));
    QCOMPARE(clickedSpy.count(), 1);
    QCOMPARE(clickedSpy.at(0).at(0).toInt(), 42);

    widget.setSelected(true);
    QVERIFY(widget.isSelected());
    widget.setPlayable(true);
    widget.setFaceUp(false, true);
    widget.update();
    QTest::mouseDClick(&widget, Qt::LeftButton, Qt::NoModifier, QPoint(20, 20));
    QVERIFY(doubleClickedSpy.count() >= 1);
    widget.setSelected(false);
    QVERIFY(!widget.isSelected());

    CardData emptyData;
    emptyData.cardId = -1;
    widget.setDisplayData(emptyData);
    int before = clickedSpy.count();
    QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, QPoint(20, 20));
    QCOMPARE(clickedSpy.count(), before);
}

void ViewTest::handCardAreaInteraction()
{
    HandCardAreaWidget area;
    area.resize(300, 130);
    area.show();

    CardList cards;
    cards.append(makeCardData(10, QStringLiteral("First")));
    cards.append(makeCardData(11, QStringLiteral("Second")));
    area.setCards(cards, true);
    QCoreApplication::processEvents();
    QCOMPARE(area.cardCount(), 2);
    QCOMPARE(area.findChildren<CardWidget*>().size(), 2);

    QSignalSpy clickedSpy(&area, &HandCardAreaWidget::cardClicked);
    QSignalSpy doubleClickedSpy(&area, &HandCardAreaWidget::cardDoubleClicked);
    CardWidget* first = nullptr;
    CardWidget* second = nullptr;
    for (CardWidget* card : area.findChildren<CardWidget*>()) {
        if (card->cardId() == 10) first = card;
        if (card->cardId() == 11) second = card;
    }
    QVERIFY(first != nullptr);
    QVERIFY(second != nullptr);

    QTest::mouseClick(first, Qt::LeftButton, Qt::NoModifier, QPoint(15, 15));
    QCOMPARE(clickedSpy.count(), 1);
    QCOMPARE(clickedSpy.at(0).at(0).toInt(), 10);
    QCOMPARE(area.selectedCardId(), 10);
    QTest::mouseClick(second, Qt::LeftButton, Qt::NoModifier, QPoint(15, 15));
    QCOMPARE(area.selectedCardId(), 11);
    area.setSelection(10, true);
    QCOMPARE(area.selectedCardId(), 10);
    area.clearSelection();
    QCOMPARE(area.selectedCardId(), -1);

    QTest::mouseDClick(second, Qt::LeftButton, Qt::NoModifier, QPoint(15, 15));
    QVERIFY(doubleClickedSpy.count() >= 1);

    CardList replacement;
    replacement.append(makeCardData(12, QStringLiteral("Replacement")));
    area.setCards(replacement, false);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCOMPARE(area.cardCount(), 1);
    QCOMPARE(area.findChildren<CardWidget*>().size(), 1);
    QCOMPARE(area.findChildren<CardWidget*>().front()->cardId(), 12);
}

void ViewTest::playerInfoDisplayAndClick()
{
    ExposedPlayerInfoWidget widget;
    widget.resize(400, 90);
    widget.show();

    PlayerData data;
    data.playerId = 7;
    data.displayName = QStringLiteral("Alice");
    data.characterName = QStringLiteral("Hero");
    data.skillName = QStringLiteral("Skill");
    data.skillDescription = QStringLiteral("Description");
    data.hp = 2;
    data.maxHp = 4;
    data.isAlive = true;
    data.isDying = true;
    data.handCardCount = 3;
    data.handCardLimit = 2;
    data.isCurrentPlayer = true;
    widget.setDisplayData(data);

    QCOMPARE(widget.playerId(), 7);
    QVERIFY(widget.isCurrentPlayer());
    QVERIFY(widget.findChildren<QLabel*>().size() >= 5);
    bool foundName = false;
    bool foundSkill = false;
    for (QLabel* label : widget.findChildren<QLabel*>()) {
        foundName = foundName || label->text().contains(QStringLiteral("Alice"));
        foundSkill = foundSkill || label->text().contains(QStringLiteral("Skill"));
    }
    QVERIFY(foundName);
    QVERIFY(foundSkill);

    widget.setTargetable(true);
    QVERIFY(widget.isTargetable());
    widget.setTargetable(false);
    QVERIFY(!widget.isTargetable());

    QSignalSpy clickedSpy(&widget, &PlayerInfoWidget::clicked);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QMouseEvent event(QEvent::MouseButtonPress, QPointF(2, 2), Qt::LeftButton,
                      Qt::LeftButton, Qt::NoModifier);
#else
    QMouseEvent event(QEvent::MouseButtonPress, QPointF(2, 2), Qt::LeftButton,
                      Qt::LeftButton, Qt::NoModifier);
#endif
    widget.mousePressEvent(&event);
    QCOMPARE(clickedSpy.count(), 1);
    QCOMPARE(clickedSpy.at(0).at(0).toInt(), 7);
}

void ViewTest::actionPanelStates()
{
    ActionPanelWidget panel;
    panel.resize(600, 60);
    panel.show();
    const auto buttons = panel.findChildren<QPushButton*>();
    QCOMPARE(buttons.size(), 3);
    for (QPushButton* button : buttons)
        QVERIFY(!button->isVisible());

    QSignalSpy playEndedSpy(&panel, &ActionPanelWidget::playPhaseEnded);
    QSignalSpy skippedSpy(&panel, &ActionPanelWidget::respondSkipped);
    QSignalSpy discardSpy(&panel, &ActionPanelWidget::discardConfirmed);

    panel.updateForPhase(PhaseType::Prepare, false);
    QVERIFY(visibleButton(&panel) == nullptr);
    panel.updateForPhase(PhaseType::Play, false);
    QPushButton* playButton = visibleButton(&panel);
    QVERIFY(playButton != nullptr);
    QTest::mouseClick(playButton, Qt::LeftButton);
    QCOMPARE(playEndedSpy.count(), 1);

    panel.updateForPhase(PhaseType::Discard, false);
    QPushButton* discardButton = visibleButton(&panel);
    QVERIFY(discardButton != nullptr);
    QTest::mouseClick(discardButton, Qt::LeftButton);
    QCOMPARE(discardSpy.count(), 1);

    PendingActionData pending;
    pending.description = QStringLiteral("Waiting for response");
    pending.requiredCardType = CardType::Dodge;
    pending.targetId = 1;
    panel.updateForPendingAction(pending);
    QPushButton* skipButton = visibleButton(&panel);
    QVERIFY(skipButton != nullptr);
    QTest::mouseClick(skipButton, Qt::LeftButton);
    QCOMPARE(skippedSpy.count(), 1);
    panel.setHint(QStringLiteral("Custom hint"));
    bool foundHint = false;
    for (QLabel* label : panel.findChildren<QLabel*>())
        foundHint = foundHint || label->text() == QStringLiteral("Custom hint");
    QVERIFY(foundHint);
}

void ViewTest::gameBoardRouting()
{
    GameBoardWidget board;
    board.resize(900, 700);
    board.show();
    QCoreApplication::processEvents();

    const auto areas = board.findChildren<HandCardAreaWidget*>();
    QCOMPARE(areas.size(), 2);
    HandCardAreaWidget* topArea = areas.at(0);
    HandCardAreaWidget* bottomArea = areas.at(1);
    ActionPanelWidget* panel = board.findChild<ActionPanelWidget*>();
    QVERIFY(panel != nullptr);

    PlayerData current;
    current.playerId = 0;
    current.displayName = QStringLiteral("Bottom");
    current.characterName = QStringLiteral("Hero");
    current.hp = 4;
    current.maxHp = 4;
    current.isAlive = true;
    current.isCurrentPlayer = true;
    board.onPlayerDataUpdated(0, current);
    PlayerData opponent = current;
    opponent.playerId = 1;
    opponent.displayName = QStringLiteral("Top");
    opponent.isCurrentPlayer = false;
    board.onPlayerDataUpdated(1, opponent);

    CardList bottomCards;
    bottomCards.append(makeCardData(100, QStringLiteral("Bottom Card")));
    CardList topCards;
    topCards.append(makeCardData(200, QStringLiteral("Top Card")));
    board.onHandCardsUpdated(0, bottomCards);
    board.onHandCardsUpdated(1, topCards);
    QCOMPARE(bottomArea->cardCount(), 1);
    QCOMPARE(topArea->cardCount(), 1);

    QSignalSpy playSpy(&board, &GameBoardWidget::playCardRequested);
    CardWidget* bottomCard = bottomArea->findChildren<CardWidget*>().front();
    QTest::mouseClick(bottomCard, Qt::LeftButton, Qt::NoModifier, QPoint(15, 15));
    QCOMPARE(playSpy.count(), 1);
    QCOMPARE(playSpy.at(0).at(0).toInt(), 100);
    QCOMPARE(playSpy.at(0).at(1).toInt(), 0);

    PendingActionData pending;
    pending.sourceId = 0;
    pending.targetId = 1;
    pending.requiredCardType = CardType::Dodge;
    pending.description = QStringLiteral("Respond");
    board.onPendingActionCreated(pending);
    QSignalSpy responseSpy(&board, &GameBoardWidget::respondCardRequested);
    CardWidget* topCard = topArea->findChildren<CardWidget*>().front();
    QTest::mouseClick(topCard, Qt::LeftButton, Qt::NoModifier, QPoint(15, 15));
    QCOMPARE(responseSpy.count(), 1);
    QCOMPARE(responseSpy.at(0).at(0).toInt(), 200);
    QCOMPARE(responseSpy.at(0).at(1).toInt(), 1);
    QSignalSpy skipSpy(&board, &GameBoardWidget::skipRequested);
    QPushButton* skipButton = visibleButton(panel);
    QVERIFY(skipButton != nullptr);
    QTest::mouseClick(skipButton, Qt::LeftButton);
    QCOMPARE(skipSpy.count(), 1);
    board.onPendingActionCleared();

    PendingActionData dyingPending;
    dyingPending.sourceId = 0;
    dyingPending.targetId = 1;
    dyingPending.requiredCardType = CardType::Peach;
    dyingPending.description = QStringLiteral("Dying response");
    board.onPendingActionCreated(dyingPending);
    QSignalSpy dyingResponseSpy(&board, &GameBoardWidget::respondCardRequested);
    QTest::mouseClick(bottomArea->findChildren<CardWidget*>().front(),
                      Qt::LeftButton, Qt::NoModifier, QPoint(15, 15));
    QCOMPARE(dyingResponseSpy.count(), 1);
    QCOMPARE(dyingResponseSpy.at(0).at(0).toInt(), 100);
    QCOMPARE(dyingResponseSpy.at(0).at(1).toInt(), 0);
    board.onPendingActionCleared();

    board.onPhaseChanged(PhaseType::Discard);
    board.onHandCardsUpdated(0, bottomCards);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QTest::mouseClick(bottomArea->findChildren<CardWidget*>().front(), Qt::LeftButton,
                      Qt::NoModifier, QPoint(15, 15));
    QCOMPARE(bottomArea->selectedCardId(), 100);
    QSignalSpy discardSpy(&board, &GameBoardWidget::discardCardRequested);
    QPushButton* confirmButton = visibleButton(panel);
    QVERIFY(confirmButton != nullptr);
    QTest::mouseClick(confirmButton, Qt::LeftButton);
    QCOMPARE(discardSpy.count(), 1);
    QCOMPARE(discardSpy.at(0).at(0).toInt(), 100);
    QCOMPARE(discardSpy.at(0).at(1).toInt(), 0);
    QCOMPARE(bottomArea->selectedCardId(), -1);

    board.onPhaseChanged(PhaseType::Play);
    QSignalSpy endPlaySpy(&board, &GameBoardWidget::endPlayRequested);
    QPushButton* endButton = visibleButton(panel);
    QVERIFY(endButton != nullptr);
    QTest::mouseClick(endButton, Qt::LeftButton);
    QCOMPARE(endPlaySpy.count(), 1);

    QSignalSpy advanceSpy(&board, &GameBoardWidget::advanceRequested);
    board.onPhaseChanged(PhaseType::Prepare);
    QTRY_COMPARE_WITH_TIMEOUT(advanceSpy.count(), 1, 1000);
}

void ViewTest::targetSelectionState()
{
    GameBoardWidget board;
    board.resize(900, 700);
    board.show();

    PlayerData bottom;
    bottom.playerId = 0;
    bottom.displayName = QStringLiteral("Bottom");
    bottom.characterName = QStringLiteral("Hero");
    bottom.hp = 4;
    bottom.maxHp = 4;
    bottom.isAlive = true;
    bottom.isCurrentPlayer = true;
    board.onPlayerDataUpdated(0, bottom);
    PlayerData top = bottom;
    top.playerId = 1;
    top.displayName = QStringLiteral("Top");
    top.isCurrentPlayer = false;
    board.onPlayerDataUpdated(1, top);

    auto infos = board.findChildren<PlayerInfoWidget*>();
    PlayerInfoWidget* topInfo = nullptr;
    PlayerInfoWidget* bottomInfo = nullptr;
    for (PlayerInfoWidget* info : infos) {
        if (info->playerId() == 0) bottomInfo = info;
        if (info->playerId() == 1) topInfo = info;
    }
    QVERIFY(topInfo != nullptr);
    QVERIFY(bottomInfo != nullptr);

    QSignalSpy targetSpy(&board, &GameBoardWidget::targetPlayerSelected);
    board.onTargetSelectionStarted(QVector<int>{1});
    QVERIFY(topInfo->isTargetable());
    QVERIFY(!bottomInfo->isTargetable());
    QTest::mouseClick(topInfo, Qt::LeftButton);
    QCOMPARE(targetSpy.count(), 1);
    QCOMPARE(targetSpy.at(0).at(0).toInt(), 1);

    board.onTargetSelectionFinished();
    QVERIFY(!topInfo->isTargetable());
    QVERIFY(!bottomInfo->isTargetable());
}

void ViewTest::mainWindowAndAppComposition()
{
    MainWindow window;
    window.show();
    QStackedWidget* stack = window.findChild<QStackedWidget*>();
    QVERIFY(stack != nullptr);
    QCOMPARE(stack->count(), 1);

    const auto groups = window.findChildren<QButtonGroup*>();
    QCOMPARE(groups.size(), 2);
    const auto startButtons = window.findChildren<QPushButton*>();
    QCOMPARE(startButtons.size(), 1);
    QPushButton* startButton = startButtons.front();
    QVERIFY(!startButton->isEnabled());

    groups.at(0)->button(0)->click();
    groups.at(1)->button(0)->click();
    QVERIFY(!startButton->isEnabled());
    groups.at(1)->button(1)->click();
    QVERIFY(startButton->isEnabled());

    QSignalSpy startSpy(&window, &MainWindow::startGameRequested);
    QTest::mouseClick(startButton, Qt::LeftButton);
    QCOMPARE(startSpy.count(), 1);
    QCOMPARE(startSpy.at(0).at(0).toInt(), 0);
    QCOMPARE(startSpy.at(0).at(1).toInt(), 1);

    auto* board = new GameBoardWidget;
    window.showGamePage(board);
    QCOMPARE(stack->count(), 2);
    QCOMPARE(stack->currentWidget(), board);
    window.showSelectionPage();
    QCOMPARE(stack->currentWidget(), stack->widget(0));

    SGSApp app;
    auto* appMainWindow = qobject_cast<MainWindow*>(app.mainWindow());
    QVERIFY(appMainWindow != nullptr);
    app.startLocalGame(0, 1);
    QPointer<GameViewModel> firstGame = app.findChild<GameViewModel*>();
    QVERIFY(!firstGame.isNull());
    QStackedWidget* appStack = appMainWindow->findChild<QStackedWidget*>();
    QVERIFY(appStack != nullptr);
    QVERIFY(qobject_cast<GameBoardWidget*>(appStack->currentWidget()) != nullptr);
    QVERIFY(QMetaObject::invokeMethod(&app, "onGameFinished", Qt::DirectConnection));
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(firstGame.isNull());
    QCOMPARE(app.findChildren<GameViewModel*>().size(), 0);
    QCOMPARE(appStack->currentWidget(), appStack->widget(0));

    app.startLocalGame(0, 1);
    QCOMPARE(app.findChildren<GameViewModel*>().size(), 1);
    QVERIFY(QMetaObject::invokeMethod(&app, "onGameFinished", Qt::DirectConnection));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCOMPARE(app.findChildren<GameViewModel*>().size(), 0);
}

QTEST_MAIN(ViewTest)

#include "view_test.moc"
