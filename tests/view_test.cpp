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

QPushButton* buttonWithText(ActionPanelWidget* panel, const QString& text)
{
    for (QPushButton* button : panel->findChildren<QPushButton*>()) {
        if (button->text() == text) return button;
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
    void handCardFanArrangement();
    void playerInfoDisplayAndClick();
    void actionPanelStates();
    void gameBoardRouting();
    void logQueueSequentialPlayback();
    void logQueueClearedOnPhaseChange();
    void gameBoardSkillSelection();
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

    area.setMultiSelectMode(true);
    QTest::mouseClick(first, Qt::LeftButton, Qt::NoModifier, QPoint(15, 15));
    QTest::mouseClick(second, Qt::LeftButton, Qt::NoModifier, QPoint(15, 15));
    const QVector<int> selectedIds = area.selectedCardIds();
    QCOMPARE(selectedIds.size(), 2);
    QVERIFY(selectedIds.contains(10));
    QVERIFY(selectedIds.contains(11));
    area.clearSelection();
    area.setMultiSelectMode(false);

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

void ViewTest::handCardFanArrangement()
{
    HandCardAreaWidget area;
    area.resize(600, 140);
    area.show();

    // ≤6 张：扇形模式，控件外扩 2*FAN_MARGIN，牌面居中绘制、控件正立
    CardList few;
    for (int i = 0; i < 5; ++i)
        few.append(makeCardData(300 + i, QStringLiteral("Fan %1").arg(i)));
    area.setCards(few, true);
    QCoreApplication::processEvents();
    QCOMPARE(area.cardCount(), 5);

    const auto fanCards = area.findChildren<CardWidget*>();
    QCOMPARE(fanCards.size(), 5);
    for (CardWidget* c : fanCards) {
        QVERIFY(c->fanMode());
        QCOMPARE(c->width(),  CardWidget::CARD_WIDTH + 2 * CardWidget::FAN_MARGIN);
        QCOMPARE(c->height(), CardWidget::CARD_HEIGHT + 2 * CardWidget::FAN_MARGIN);
    }

    // 扇形外扩后，固定坐标点击仍命中正确卡片（核心回归保护）
    QSignalSpy clickedSpy(&area, &HandCardAreaWidget::cardClicked);
    CardWidget* target = nullptr;
    for (CardWidget* c : fanCards)
        if (c->cardId() == 302) target = c;
    QVERIFY(target != nullptr);
    QTest::mouseClick(target, Qt::LeftButton, Qt::NoModifier, QPoint(15, 15));
    QCOMPARE(clickedSpy.count(), 1);
    QCOMPARE(clickedSpy.at(0).at(0).toInt(), 302);
    QCOMPARE(area.selectedCardId(), 302);

    // >6 张：退回水平重叠，控件恢复原始 80×112、非扇形
    CardList many;
    for (int i = 0; i < 8; ++i)
        many.append(makeCardData(400 + i, QStringLiteral("Flat %1").arg(i)));
    area.setCards(many, true);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    QCOMPARE(area.cardCount(), 8);

    const auto flatCards = area.findChildren<CardWidget*>();
    QCOMPARE(flatCards.size(), 8);
    for (CardWidget* c : flatCards) {
        QVERIFY(!c->fanMode());
        QCOMPARE(c->width(),  CardWidget::CARD_WIDTH);
        QCOMPARE(c->height(), CardWidget::CARD_HEIGHT);
    }

    // 水平重叠模式点击仍命中
    QSignalSpy flatSpy(&area, &HandCardAreaWidget::cardClicked);
    CardWidget* flatTarget = nullptr;
    for (CardWidget* c : flatCards)
        if (c->cardId() == 405) flatTarget = c;
    QVERIFY(flatTarget != nullptr);
    QTest::mouseClick(flatTarget, Qt::LeftButton, Qt::NoModifier, QPoint(15, 15));
    QCOMPARE(flatSpy.count(), 1);
    QCOMPARE(flatSpy.at(0).at(0).toInt(), 405);
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
    CardData judgment = makeCardData(99, QStringLiteral("乐不思蜀"));
    judgment.cardType = CardType::Happy;
    data.judgmentCards.append(judgment);
    widget.setDisplayData(data);

    QCOMPARE(widget.playerId(), 7);
    QVERIFY(widget.isCurrentPlayer());
    QVERIFY(widget.findChildren<QLabel*>().size() >= 5);
    bool foundName = false;
    bool foundSkill = false;
    bool foundJudgment = false;
    for (QLabel* label : widget.findChildren<QLabel*>()) {
        foundName = foundName || label->text().contains(QStringLiteral("Alice"));
        foundSkill = foundSkill || label->text().contains(QStringLiteral("Skill"));
        foundJudgment = foundJudgment || label->text().contains(QStringLiteral("乐不思蜀"));
    }
    QVERIFY(foundName);
    QVERIFY(foundSkill);
    QVERIFY(foundJudgment);

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
    QCOMPARE(buttons.size(), 4);
    for (QPushButton* button : buttons)
        QVERIFY(!button->isVisible());

    QSignalSpy playEndedSpy(&panel, &ActionPanelWidget::playPhaseEnded);
    QSignalSpy skippedSpy(&panel, &ActionPanelWidget::respondSkipped);
    QSignalSpy discardSpy(&panel, &ActionPanelWidget::discardConfirmed);
    QSignalSpy skillSpy(&panel, &ActionPanelWidget::skillRequested);

    panel.updateForPhase(PhaseType::Prepare, false);
    QVERIFY(visibleButton(&panel) == nullptr);
    panel.updateForPhase(PhaseType::Play, false);
    QPushButton* playButton = buttonWithText(&panel, QStringLiteral("结束出牌"));
    QVERIFY(playButton != nullptr);
    QVERIFY(playButton->isVisible());
    QTest::mouseClick(playButton, Qt::LeftButton);
    QCOMPARE(playEndedSpy.count(), 1);

    panel.setSkillAvailable(true);
    QPushButton* skillButton = buttonWithText(&panel, QStringLiteral("发动技能"));
    QVERIFY(skillButton != nullptr);
    QVERIFY(skillButton->isVisible());
    QTest::mouseClick(skillButton, Qt::LeftButton);
    QCOMPARE(skillSpy.count(), 1);
    panel.setSkillSelectionMode(true);
    QCOMPARE(skillButton->text(), QStringLiteral("确认技能"));
    panel.setSkillSelectionMode(false);

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

void ViewTest::logQueueSequentialPlayback()
{
    GameBoardWidget board;
    board.resize(900, 700);
    board.show();
    QCoreApplication::processEvents();

    QLabel* logLabel = board.findChild<QLabel*>(QStringLiteral("logLabel"));
    QVERIFY(logLabel != nullptr);

    // 同一调用栈连发 3 条结算 log：应逐条显示，而非只剩最后一条
    board.onLogMessage(QStringLiteral("A 使用了【杀】"));
    board.onLogMessage(QStringLiteral("B 打出了【闪】"));
    board.onLogMessage(QStringLiteral("A 的【杀】被抵消"));

    // 队首立即显示
    QCOMPARE(logLabel->text(), QStringLiteral("A 使用了【杀】"));

    // 约 1.2s 后播放第二条
    QTRY_COMPARE_WITH_TIMEOUT(logLabel->text(), QStringLiteral("B 打出了【闪】"), 2000);
    // 再约 1.2s 后播放第三条
    QTRY_COMPARE_WITH_TIMEOUT(logLabel->text(), QStringLiteral("A 的【杀】被抵消"), 2000);

    // 播完后回退到当前阶段提示（默认 Prepare）
    QTRY_COMPARE_WITH_TIMEOUT(logLabel->text(), QStringLiteral("准备阶段"), 4000);
}

void ViewTest::logQueueClearedOnPhaseChange()
{
    GameBoardWidget board;
    board.resize(900, 700);
    board.show();
    QCoreApplication::processEvents();

    QLabel* logLabel = board.findChild<QLabel*>(QStringLiteral("logLabel"));
    QVERIFY(logLabel != nullptr);

    board.onLogMessage(QStringLiteral("结算 log 1"));
    board.onLogMessage(QStringLiteral("结算 log 2"));
    board.onLogMessage(QStringLiteral("结算 log 3"));
    QCOMPARE(logLabel->text(), QStringLiteral("结算 log 1"));

    // 阶段切换应立即清空队列并显示新阶段提示
    board.onPhaseChanged(PhaseType::Play);
    QCOMPARE(logLabel->text(), QStringLiteral("出牌阶段"));

    // 等待超过一条 log 的播放间隔，确认队列已被清空、不再弹出残留 log
    QTest::qWait(1500);
    QCOMPARE(logLabel->text(), QStringLiteral("出牌阶段"));
}

void ViewTest::gameBoardSkillSelection()
{
    GameBoardWidget board;
    board.resize(900, 700);
    board.show();

    PlayerData current;
    current.playerId = 0;
    current.displayName = QStringLiteral("Sun Quan");
    current.characterName = QStringLiteral("孙权");
    current.hp = 4;
    current.maxHp = 4;
    current.isAlive = true;
    current.isCurrentPlayer = true;
    current.canUseActiveSkill = true;
    board.onPlayerDataUpdated(0, current);

    PlayerData opponent = current;
    opponent.playerId = 1;
    opponent.displayName = QStringLiteral("Opponent");
    opponent.isCurrentPlayer = false;
    opponent.canUseActiveSkill = false;
    board.onPlayerDataUpdated(1, opponent);

    CardList cards;
    cards.append(makeCardData(301, QStringLiteral("First")));
    cards.append(makeCardData(302, QStringLiteral("Second")));
    board.onHandCardsUpdated(0, cards);
    board.onPhaseChanged(PhaseType::Play);
    QCoreApplication::processEvents();

    auto* panel = board.findChild<ActionPanelWidget*>();
    QVERIFY(panel != nullptr);
    auto* skillButton = panel->findChild<QPushButton*>(QStringLiteral("skillButton"));
    QVERIFY(skillButton != nullptr);
    QVERIFY(skillButton->isVisible());

    QSignalSpy skillSpy(&board, &GameBoardWidget::skillRequested);
    QTest::mouseClick(skillButton, Qt::LeftButton);
    QCOMPARE(skillButton->text(), QStringLiteral("确认技能"));

    HandCardAreaWidget* ownArea = nullptr;
    for (HandCardAreaWidget* area : board.findChildren<HandCardAreaWidget*>()) {
        for (CardWidget* card : area->findChildren<CardWidget*>()) {
            if (card->cardId() == 301) ownArea = area;
        }
    }
    QVERIFY(ownArea != nullptr);

    for (CardWidget* card : ownArea->findChildren<CardWidget*>()) {
        QTest::mouseClick(card, Qt::LeftButton, Qt::NoModifier, QPoint(15, 15));
    }
    QCOMPARE(ownArea->selectedCardIds().size(), 2);

    QTest::mouseClick(skillButton, Qt::LeftButton);
    QCOMPARE(skillSpy.count(), 1);
    const QVector<int> selected = skillSpy.at(0).at(0).value<QVector<int>>();
    QCOMPARE(selected.size(), 2);
    QVERIFY(selected.contains(301));
    QVERIFY(selected.contains(302));
    QCOMPARE(skillSpy.at(0).at(1).toInt(), 0);
    QVERIFY(ownArea->selectedCardIds().isEmpty());

    CardData judgeCard = makeCardData(400, QStringLiteral("判定牌"));
    judgeCard.suitSymbol = QStringLiteral("♠");
    judgeCard.numberString = QStringLiteral("7");
    board.onJudgmentPerformed(judgeCard, QStringLiteral("判定生效"), true);
    bool foundJudgment = false;
    for (QLabel* label : board.findChildren<QLabel*>()) {
        foundJudgment = foundJudgment || label->text().contains(QStringLiteral("判定生效"));
    }
    QVERIFY(foundJudgment);
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
    QCOMPARE(startButtons.size(), 3);
    QPushButton* startButton = nullptr;
    for (QPushButton* button : startButtons) {
        if (button->text() == QStringLiteral("开始对战")) {
            startButton = button;
            break;
        }
    }
    QVERIFY(startButton != nullptr);
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
