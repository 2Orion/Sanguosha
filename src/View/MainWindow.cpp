#include "MainWindow.h"
#include "GameBoardWidget.h"
#include "GameViewModel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QRadioButton>
#include <QFont>
#include <QMessageBox>
#include <QFrame>

// ==================== 武将数据 ====================

struct CharInfo {
    const char* name;
    const char* skillName;
    const char* skillDesc;
    int maxHp;
};

static const CharInfo CHAR_LIST[] = {
    { "曹操", "奸雄", "受到伤害后摸1张牌", 4 },
    { "关羽", "武圣", "红色牌可当【杀】使用或打出", 4 },
    { "张飞", "咆哮", "出牌阶段可使用任意张【杀】", 4 },
    { "赵云", "龙胆", "【杀】可当【闪】，【闪】可当【杀】", 4 },
};
static constexpr int CHAR_COUNT = 4;

// ==================== 构造 ====================

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("三国杀 — 双人对战"));
    resize(900, 700);

    m_centralStack = new QStackedWidget(this);
    setCentralWidget(m_centralStack);

    setupSelectionPage();
    setupGamePage();

    m_centralStack->setCurrentWidget(m_selectionPage);
}

MainWindow::~MainWindow()
{
    delete m_gvm;
}

// ==================== 选将界面 ====================

void MainWindow::setupSelectionPage()
{
    m_selectionPage = new QWidget(this);

    QVBoxLayout* mainLayout = new QVBoxLayout(m_selectionPage);
    mainLayout->setContentsMargins(24, 16, 24, 16);
    mainLayout->setSpacing(12);

    // ---- 标题 ----
    QLabel* title = new QLabel(QStringLiteral("三国杀 — 双人对战"), m_selectionPage);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(
        "font-size: 28px; font-weight: bold; color: #B71C1C;"
        "padding: 12px;"
    );
    mainLayout->addWidget(title);

    QLabel* subtitle = new QLabel(
        QStringLiteral("本地双人模式 — 请双方各自选择武将"), m_selectionPage);
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet("font-size: 14px; color: #888; margin-bottom: 8px;");
    mainLayout->addWidget(subtitle);

    // ---- 选将区域 ----
    QHBoxLayout* selectionLayout = new QHBoxLayout;
    selectionLayout->setSpacing(24);

    // 玩家 1
    QVBoxLayout* p1Column = new QVBoxLayout;
    QLabel* p1Title = new QLabel(QStringLiteral("玩家 1 选择武将"), m_selectionPage);
    p1Title->setStyleSheet("font-size: 16px; font-weight: bold; color: #1565C0;");
    p1Column->addWidget(p1Title);

    m_p1Group = new QButtonGroup(this);
    m_p1Group->setExclusive(true);
    QGridLayout* p1Grid = new QGridLayout;
    p1Grid->setSpacing(8);

    for (int i = 0; i < CHAR_COUNT; ++i) {
        QGroupBox* card = new QGroupBox(m_selectionPage);
        card->setStyleSheet(
            "QGroupBox {"
            "  border: 2px solid #E0E0E0;"
            "  border-radius: 8px;"
            "  background: #FAFAFA;"
            "  padding: 8px;"
            "}"
            "QGroupBox:hover {"
            "  border-color: #90CAF9;"
            "  background: #E3F2FD;"
            "}"
        );

        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setSpacing(4);
        cardLayout->setContentsMargins(8, 8, 8, 8);

        QRadioButton* radio = new QRadioButton(CHAR_LIST[i].name, card);
        radio->setStyleSheet(
            "QRadioButton { font-size: 14px; font-weight: bold; color: #333; }"
            "QRadioButton::indicator { width: 16px; height: 16px; }"
        );
        m_p1Group->addButton(radio, i);

        QLabel* hpLabel = new QLabel(
            QString(QStringLiteral("体力: %1")).arg(CHAR_LIST[i].maxHp), card);
        hpLabel->setStyleSheet("font-size: 12px; color: #D32F2F;");

        QLabel* skillLabel = new QLabel(
            QString(QStringLiteral("【%1】%2"))
                .arg(CHAR_LIST[i].skillName)
                .arg(CHAR_LIST[i].skillDesc),
            card);
        skillLabel->setWordWrap(true);
        skillLabel->setStyleSheet("font-size: 11px; color: #666;");

        cardLayout->addWidget(radio);
        cardLayout->addWidget(hpLabel);
        cardLayout->addWidget(skillLabel);

        p1Grid->addWidget(card, i / 2, i % 2);
    }
    p1Column->addLayout(p1Grid);

    m_p1SelectedLabel = new QLabel(QStringLiteral("未选择"), m_selectionPage);
    m_p1SelectedLabel->setStyleSheet("font-size: 12px; color: #999; font-style: italic;");
    p1Column->addWidget(m_p1SelectedLabel);

    selectionLayout->addLayout(p1Column);

    // vs 分隔
    QLabel* vsLabel = new QLabel(QStringLiteral("VS"), m_selectionPage);
    vsLabel->setAlignment(Qt::AlignCenter);
    vsLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #B71C1C; padding: 0 12px;");
    selectionLayout->addWidget(vsLabel);

    // 玩家 2
    QVBoxLayout* p2Column = new QVBoxLayout;
    QLabel* p2Title = new QLabel(QStringLiteral("玩家 2 选择武将"), m_selectionPage);
    p2Title->setStyleSheet("font-size: 16px; font-weight: bold; color: #E65100;");
    p2Column->addWidget(p2Title);

    m_p2Group = new QButtonGroup(this);
    m_p2Group->setExclusive(true);
    QGridLayout* p2Grid = new QGridLayout;
    p2Grid->setSpacing(8);

    for (int i = 0; i < CHAR_COUNT; ++i) {
        QGroupBox* card = new QGroupBox(m_selectionPage);
        card->setStyleSheet(
            "QGroupBox {"
            "  border: 2px solid #E0E0E0;"
            "  border-radius: 8px;"
            "  background: #FAFAFA;"
            "  padding: 8px;"
            "}"
            "QGroupBox:hover {"
            "  border-color: #FFCC80;"
            "  background: #FFF3E0;"
            "}"
        );

        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setSpacing(4);
        cardLayout->setContentsMargins(8, 8, 8, 8);

        QRadioButton* radio = new QRadioButton(CHAR_LIST[i].name, card);
        radio->setStyleSheet(
            "QRadioButton { font-size: 14px; font-weight: bold; color: #333; }"
            "QRadioButton::indicator { width: 16px; height: 16px; }"
        );
        m_p2Group->addButton(radio, i);

        QLabel* hpLabel = new QLabel(
            QString(QStringLiteral("体力: %1")).arg(CHAR_LIST[i].maxHp), card);
        hpLabel->setStyleSheet("font-size: 12px; color: #D32F2F;");

        QLabel* skillLabel = new QLabel(
            QString(QStringLiteral("【%1】%2"))
                .arg(CHAR_LIST[i].skillName)
                .arg(CHAR_LIST[i].skillDesc),
            card);
        skillLabel->setWordWrap(true);
        skillLabel->setStyleSheet("font-size: 11px; color: #666;");

        cardLayout->addWidget(radio);
        cardLayout->addWidget(hpLabel);
        cardLayout->addWidget(skillLabel);

        p2Grid->addWidget(card, i / 2, i % 2);
    }
    p2Column->addLayout(p2Grid);

    m_p2SelectedLabel = new QLabel(QStringLiteral("未选择"), m_selectionPage);
    m_p2SelectedLabel->setStyleSheet("font-size: 12px; color: #999; font-style: italic;");
    p2Column->addWidget(m_p2SelectedLabel);

    selectionLayout->addLayout(p2Column);
    mainLayout->addLayout(selectionLayout);

    // ---- 开始按钮 ----
    m_startBtn = new QPushButton(QStringLiteral("开始对战"), m_selectionPage);
    m_startBtn->setMinimumHeight(48);
    m_startBtn->setStyleSheet(
        "QPushButton {"
        "  font-size: 18px; font-weight: bold;"
        "  color: white;"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #D32F2F, stop:1 #C62828);"
        "  border: none; border-radius: 8px;"
        "  padding: 10px 40px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #E53935, stop:1 #D32F2F);"
        "}"
        "QPushButton:disabled {"
        "  background: #BDBDBD;"
        "  color: #9E9E9E;"
        "}"
    );
    m_startBtn->setEnabled(false);
    connect(m_startBtn, &QPushButton::clicked, this, &MainWindow::onStartGame);
    mainLayout->addWidget(m_startBtn, 0, Qt::AlignCenter);

    // ---- 事件：选将变化 ----
    connect(m_p1Group, QOverload<int>::of(&QButtonGroup::idClicked),
            this, &MainWindow::onP1CharChanged);
    connect(m_p2Group, QOverload<int>::of(&QButtonGroup::idClicked),
            this, &MainWindow::onP2CharChanged);

    m_centralStack->addWidget(m_selectionPage);
}

// ==================== 游戏页面 ====================

void MainWindow::setupGamePage()
{
    // 游戏页面会在 startGame 时动态创建
    // 这里先占位，之后会被替换
    m_gameBoard = nullptr;
}

// ==================== 选将逻辑 ====================

void MainWindow::onP1CharChanged(int id)
{
    if (id >= 0 && id < CHAR_COUNT) {
        m_p1SelectedLabel->setText(
            QStringLiteral("已选择: %1").arg(CHAR_LIST[id].name));
        m_p1SelectedLabel->setStyleSheet("font-size: 12px; color: #1565C0; font-weight: bold;");
    }
    updateStartButton();
}

void MainWindow::onP2CharChanged(int id)
{
    if (id >= 0 && id < CHAR_COUNT) {
        m_p2SelectedLabel->setText(
            QStringLiteral("已选择: %1").arg(CHAR_LIST[id].name));
        m_p2SelectedLabel->setStyleSheet("font-size: 12px; color: #E65100; font-weight: bold;");
    }
    updateStartButton();
}

void MainWindow::updateStartButton()
{
    int p1Id = m_p1Group->checkedId();
    int p2Id = m_p2Group->checkedId();

    if (p1Id < 0 || p2Id < 0) {
        m_startBtn->setEnabled(false);
        m_startBtn->setText(QStringLiteral("开始对战（双方均需选择武将）"));
        return;
    }

    if (p1Id == p2Id) {
        m_startBtn->setEnabled(false);
        m_startBtn->setText(QStringLiteral("双方不可选择同一武将！"));
        return;
    }

    m_startBtn->setEnabled(true);
    m_startBtn->setText(QStringLiteral("开始对战！"));
}

// ==================== 游戏生命周期 ====================

void MainWindow::onStartGame()
{
    int p1Id = m_p1Group->checkedId();
    int p2Id = m_p2Group->checkedId();

    if (p1Id < 0 || p2Id < 0 || p1Id == p2Id) return;

    // 创建 GameViewModel
    m_gvm = new GameViewModel();
    m_gvm->startGame(p1Id, p2Id);

    // 创建游戏面板
    m_gameBoard = new GameBoardWidget(m_gvm, this);
    connect(m_gameBoard, &GameBoardWidget::gameFinished,
            this, &MainWindow::onGameFinished);

    // 替换游戏页面
    if (m_centralStack->count() > 1) {
        QWidget* old = m_centralStack->widget(1);
        m_centralStack->removeWidget(old);
        old->deleteLater();
    }
    m_centralStack->addWidget(m_gameBoard);
    m_centralStack->setCurrentWidget(m_gameBoard);
}

void MainWindow::onGameFinished()
{
    // 清理游戏状态
    if (m_gameBoard) {
        m_gameBoard->deleteLater();
        m_gameBoard = nullptr;
    }
    if (m_gvm) {
        delete m_gvm;
        m_gvm = nullptr;
    }

    // 重置选择
    auto resetGroup = [](QButtonGroup* group, QLabel* label) {
        QAbstractButton* checked = group->checkedButton();
        if (checked) {
            group->setExclusive(false);
            checked->setChecked(false);
            group->setExclusive(true);
        }
        label->setText(QStringLiteral("未选择"));
        label->setStyleSheet("font-size: 12px; color: #999; font-style: italic;");
    };
    resetGroup(m_p1Group, m_p1SelectedLabel);
    resetGroup(m_p2Group, m_p2SelectedLabel);
    updateStartButton();

    m_centralStack->setCurrentWidget(m_selectionPage);
}
