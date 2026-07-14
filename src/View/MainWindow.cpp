#include "MainWindow.h"
#include "GameBoardWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QRadioButton>
#include <QFont>
#include <QGroupBox>

struct CharInfo { const char* name; const char* skillName; const char* skillDesc; int maxHp; };
static const CharInfo CHAR_LIST[] = {
    { "曹操", "奸雄", "受到伤害后摸1张牌", 4 },
    { "关羽", "武圣", "红色牌可当【杀】使用或打出", 4 },
    { "张飞", "咆哮", "出牌阶段可使用任意张【杀】", 4 },
    { "赵云", "龙胆", "【杀】可当【闪】，【闪】可当【杀】", 4 },
};
static constexpr int CHAR_COUNT = 4;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("三国杀 — 双人对战"));
    resize(900, 700);

    m_centralStack = new QStackedWidget(this);
    setCentralWidget(m_centralStack);
    setupSelectionPage();
    m_centralStack->setCurrentWidget(m_selectionPage);
}

MainWindow::~MainWindow() = default;

void MainWindow::showGamePage(GameBoardWidget* board)
{
    // 移除旧游戏页面（如果有）
    if (m_centralStack->count() > 1) {
        QWidget* old = m_centralStack->widget(1);
        m_centralStack->removeWidget(old);
        old->deleteLater();
    }
    m_centralStack->addWidget(board);
    m_centralStack->setCurrentWidget(board);
}

void MainWindow::showSelectionPage()
{
    // 游戏页面由 SGSApp 负责清理
    m_centralStack->setCurrentWidget(m_selectionPage);
    updateStartButton();
}

// ==================== 选将界面 ====================

void MainWindow::setupSelectionPage()
{
    m_selectionPage = new QWidget(this);
    auto* main = new QVBoxLayout(m_selectionPage);
    main->setContentsMargins(24, 16, 24, 16); main->setSpacing(12);

    auto* title = new QLabel(QStringLiteral("三国杀 — 双人对战"), m_selectionPage);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 28px; font-weight: bold; color: #B71C1C; padding: 12px;");
    main->addWidget(title);

    auto* subtitle = new QLabel(QStringLiteral("本地双人模式 — 请双方各自选择武将"), m_selectionPage);
    subtitle->setAlignment(Qt::AlignCenter); subtitle->setStyleSheet("font-size: 14px; color: #888; margin-bottom: 8px;");
    main->addWidget(subtitle);

    auto* selLayout = new QHBoxLayout; selLayout->setSpacing(24);

    // 玩家 1
    auto buildColumn = [&](const QString& titleText, const QString& color,
                             QButtonGroup*& group, QLabel*& selectedLabel) {
        auto* col = new QVBoxLayout;
        auto* t = new QLabel(titleText, m_selectionPage);
        t->setStyleSheet("font-size: 16px; font-weight: bold; color: " + color + ";");
        col->addWidget(t);

        group = new QButtonGroup(this); group->setExclusive(true);
        auto* grid = new QGridLayout; grid->setSpacing(8);

        for (int i = 0; i < CHAR_COUNT; ++i) {
            auto* card = new QGroupBox(m_selectionPage);
            card->setStyleSheet(
                "QGroupBox { border: 2px solid #E0E0E0; border-radius: 8px;"
                "  background: #FAFAFA; padding: 8px; }"
                "QGroupBox:hover { border-color: #90CAF9; background: #E3F2FD; }");

            auto* cl = new QVBoxLayout(card); cl->setSpacing(4); cl->setContentsMargins(8, 8, 8, 8);
            auto* radio = new QRadioButton(CHAR_LIST[i].name, card);
            radio->setStyleSheet("QRadioButton { font-size: 14px; font-weight: bold; color: #333; }"
                                 "QRadioButton::indicator { width: 16px; height: 16px; }");
            group->addButton(radio, i);

            cl->addWidget(radio);
            cl->addWidget(new QLabel(QStringLiteral("体力: %1").arg(CHAR_LIST[i].maxHp), card));
            cl->addWidget(new QLabel(QStringLiteral("【%1】%2").arg(CHAR_LIST[i].skillName, CHAR_LIST[i].skillDesc), card));
            grid->addWidget(card, i / 2, i % 2);
        }
        col->addLayout(grid);

        selectedLabel = new QLabel(QStringLiteral("未选择"), m_selectionPage);
        selectedLabel->setStyleSheet("font-size: 12px; color: #999; font-style: italic;");
        col->addWidget(selectedLabel);
        return col;
    };

    selLayout->addLayout(buildColumn(QStringLiteral("玩家 1 选择武将"), "#1565C0", m_p1Group, m_p1SelectedLabel));

    auto* vs = new QLabel(QStringLiteral("VS"), m_selectionPage);
    vs->setAlignment(Qt::AlignCenter); vs->setStyleSheet("font-size: 24px; font-weight: bold; color: #B71C1C; padding: 0 12px;");
    selLayout->addWidget(vs);

    selLayout->addLayout(buildColumn(QStringLiteral("玩家 2 选择武将"), "#E65100", m_p2Group, m_p2SelectedLabel));
    main->addLayout(selLayout);

    m_startBtn = new QPushButton(QStringLiteral("开始对战"), m_selectionPage);
    m_startBtn->setMinimumHeight(48);
    m_startBtn->setStyleSheet(
        "QPushButton { font-size: 18px; font-weight: bold; color: white;"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #D32F2F, stop:1 #C62828);"
        "  border: none; border-radius: 8px; padding: 10px 40px; }"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #E53935, stop:1 #D32F2F); }"
        "QPushButton:disabled { background: #BDBDBD; color: #9E9E9E; }");
    m_startBtn->setEnabled(false);
    connect(m_startBtn, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    main->addWidget(m_startBtn, 0, Qt::AlignCenter);

    connect(m_p1Group, QOverload<int>::of(&QButtonGroup::idClicked), this, &MainWindow::onP1CharChanged);
    connect(m_p2Group, QOverload<int>::of(&QButtonGroup::idClicked), this, &MainWindow::onP2CharChanged);

    m_centralStack->addWidget(m_selectionPage);
}

// ==================== 选将逻辑 ====================

void MainWindow::onP1CharChanged(int id)
{
    if (id >= 0 && id < CHAR_COUNT) {
        m_p1SelectedLabel->setText(QStringLiteral("已选择: %1").arg(CHAR_LIST[id].name));
        m_p1SelectedLabel->setStyleSheet("font-size: 12px; color: #1565C0; font-weight: bold;");
    }
    updateStartButton();
}

void MainWindow::onP2CharChanged(int id)
{
    if (id >= 0 && id < CHAR_COUNT) {
        m_p2SelectedLabel->setText(QStringLiteral("已选择: %1").arg(CHAR_LIST[id].name));
        m_p2SelectedLabel->setStyleSheet("font-size: 12px; color: #E65100; font-weight: bold;");
    }
    updateStartButton();
}

void MainWindow::updateStartButton()
{
    int p1 = m_p1Group->checkedId(), p2 = m_p2Group->checkedId();
    if (p1 < 0 || p2 < 0) {
        m_startBtn->setEnabled(false);
        m_startBtn->setText(QStringLiteral("开始对战（双方均需选择武将）"));
    } else if (p1 == p2) {
        m_startBtn->setEnabled(false);
        m_startBtn->setText(QStringLiteral("双方不可选择同一武将！"));
    } else {
        m_startBtn->setEnabled(true);
        m_startBtn->setText(QStringLiteral("开始对战！"));
    }
}

void MainWindow::onStartClicked()
{
    int p1 = m_p1Group->checkedId(), p2 = m_p2Group->checkedId();
    if (p1 < 0 || p2 < 0 || p1 == p2) return;
    emit startGameRequested(p1, p2);
}
