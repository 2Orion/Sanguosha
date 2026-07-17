#include "MainWindow.h"
#include "GameBoardWidget.h"
#include "NetworkConfigDialog.h"
#include "Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QRadioButton>
#include <QFont>
#include <QGroupBox>
#include <QFrame>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QApplication>

/// 加载武将头像 QPixmap，返回裁剪为圆形的版本（若无图片则返回空）
static QPixmap loadCharacterPortrait(const QString& charName, int size)
{
    // 从 images/ 目录加载（优先 .png 再 .jpg）
    QString base = QApplication::applicationDirPath() + QStringLiteral("/images/%1").arg(charName);
    QPixmap px(base + QStringLiteral(".png"));
    if (px.isNull()) px = QPixmap(base + QStringLiteral(".jpg"));
    if (px.isNull()) return {};

    // 等比缩放至至少填满 size×size 区域
    px = px.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    // 居中裁剪至 size×size
    int x = (px.width() - size) / 2;
    int y = (px.height() - size) / 2;
    px = px.copy(x, y, size, size);

    QPixmap rounded(size, size);
    rounded.fill(Qt::transparent);
    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath clipPath;
    clipPath.addEllipse(0, 0, size, size);
    painter.setClipPath(clipPath);
    painter.drawPixmap(0, 0, px);
    painter.end();
    return rounded;
}

struct CharInfo { const char* name; const char* skillName; const char* skillDesc; int maxHp; };
static const CharInfo CHAR_LIST[] = {
    { "曹操", "奸雄", "受到伤害后摸1张牌", 4 },
    { "关羽", "武圣", "红色牌可当【杀】使用或打出", 4 },
    { "张飞", "咆哮", "出牌阶段可使用任意张【杀】", 4 },
    { "赵云", "龙胆", "【杀】可当【闪】，【闪】可当【杀】", 4 },
    { "孙权", "制衡", "出牌阶段可弃任意张牌并摸等量的牌", 4 },
    { "周瑜", "英姿", "摸牌阶段多摸一张", 3 },
    { "吕布", "无双", "【杀】需两张【闪】才能抵消", 4 },
    { "大乔", "流离", "【杀】可转移给其他玩家", 3 },
    { "司马懿", "反馈", "受到伤害后可获得伤害来源一张牌", 3 },
};
static constexpr int CHAR_COUNT = 9;

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
    m_selectionPage->setObjectName(QStringLiteral("selectionPage"));
    m_selectionPage->setAttribute(Qt::WA_StyledBackground, true);
    m_selectionPage->setStyleSheet(Theme::pageBackground("selectionPage"));
    auto* main = new QVBoxLayout(m_selectionPage);
    main->setContentsMargins(24, 16, 24, 16); main->setSpacing(12);

    auto* title = new QLabel(QStringLiteral("三国杀 — 双人对战"), m_selectionPage);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(Theme::pageTitle());
    main->addWidget(title);

    auto* subtitle = new QLabel(QStringLiteral("本地双人模式 — 请双方各自选择武将"), m_selectionPage);
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet(Theme::pageSubtitle() + QStringLiteral(" margin-bottom: 8px;"));
    main->addWidget(subtitle);

    auto* selLayout = new QHBoxLayout; selLayout->setSpacing(24);

    // 玩家 1
    auto buildColumn = [&](const QString& titleText, const char* accent,
                             QButtonGroup*& group, QLabel*& selectedLabel) {
        auto* col = new QVBoxLayout;
        auto* t = new QLabel(titleText, m_selectionPage);
        t->setStyleSheet(Theme::accentText(accent, 16));
        col->addWidget(t);

        group = new QButtonGroup(this); group->setExclusive(true);
        auto* grid = new QGridLayout; grid->setSpacing(8);

        for (int i = 0; i < CHAR_COUNT; ++i) {
            auto* card = new QGroupBox(m_selectionPage);
            card->setStyleSheet(Theme::charCard());

            auto* cl = new QVBoxLayout(card); cl->setSpacing(4); cl->setContentsMargins(8, 8, 8, 8);
            auto* radio = new QRadioButton(CHAR_LIST[i].name, card);
            group->addButton(radio, i);

            cl->addWidget(radio);
            // 武将头像
            QPixmap portrait = loadCharacterPortrait(CHAR_LIST[i].name, 60);
            if (!portrait.isNull()) {
                auto* avatar = new QLabel(card);
                avatar->setPixmap(portrait);
                avatar->setFixedSize(60, 60);
                avatar->setAlignment(Qt::AlignCenter);
                avatar->setStyleSheet("QLabel { background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
                "stop:0 #8C2B22, stop:1 #5A1611); border-radius: 30px; }");
                cl->addWidget(avatar);
            }
            cl->addWidget(new QLabel(QStringLiteral("体力: %1").arg(CHAR_LIST[i].maxHp), card));
            cl->addWidget(new QLabel(QStringLiteral("【%1】%2").arg(CHAR_LIST[i].skillName, CHAR_LIST[i].skillDesc), card));
            grid->addWidget(card, i / 2, i % 2);
        }
        col->addLayout(grid);

        selectedLabel = new QLabel(QStringLiteral("未选择"), m_selectionPage);
        selectedLabel->setStyleSheet(Theme::mutedText(12, true));
        col->addWidget(selectedLabel);
        return col;
    };

    selLayout->addLayout(buildColumn(QStringLiteral("玩家 1 选择武将"), Theme::P1Accent, m_p1Group, m_p1SelectedLabel));

    auto* vs = new QLabel(QStringLiteral("VS"), m_selectionPage);
    vs->setAlignment(Qt::AlignCenter);
    vs->setStyleSheet(Theme::accentText(Theme::Gold, 24) + QStringLiteral(" padding: 0 12px;"));
    selLayout->addWidget(vs);

    selLayout->addLayout(buildColumn(QStringLiteral("玩家 2 选择武将"), Theme::P2Accent, m_p2Group, m_p2SelectedLabel));
    main->addLayout(selLayout);

    m_startBtn = new QPushButton(QStringLiteral("开始对战"), m_selectionPage);
    m_startBtn->setMinimumHeight(48);
    m_startBtn->setStyleSheet(Theme::gradientButton(
        "#A03A28", "#71271A", "#B84A34", "#8C3020", "#571D12", 18));
    m_startBtn->setEnabled(false);
    connect(m_startBtn, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    main->addWidget(m_startBtn, 0, Qt::AlignCenter);

    connect(m_p1Group, QOverload<int>::of(&QButtonGroup::idClicked), this, &MainWindow::onP1CharChanged);
    connect(m_p2Group, QOverload<int>::of(&QButtonGroup::idClicked), this, &MainWindow::onP2CharChanged);

    // 联网模式入口
    setupNetworkSection(main);

    m_centralStack->addWidget(m_selectionPage);
}

// ==================== 选将逻辑 ====================

void MainWindow::onP1CharChanged(int id)
{
    if (id >= 0 && id < CHAR_COUNT) {
        m_p1SelectedLabel->setText(QStringLiteral("已选择: %1").arg(CHAR_LIST[id].name));
        m_p1SelectedLabel->setStyleSheet(Theme::accentText(Theme::P1Accent));
    }
    updateStartButton();
}

void MainWindow::onP2CharChanged(int id)
{
    if (id >= 0 && id < CHAR_COUNT) {
        m_p2SelectedLabel->setText(QStringLiteral("已选择: %1").arg(CHAR_LIST[id].name));
        m_p2SelectedLabel->setStyleSheet(Theme::accentText(Theme::P2Accent));
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

// ==================== 联网模式：选将页面 ====================

void MainWindow::showCharacterSelection(int playerId, const QString& playerName,
                                         const QString& statusHint)
{
    // 移除旧的网络页面（如果存在）
    for (int i = m_centralStack->count() - 1; i > 0; --i) {
        QWidget* w = m_centralStack->widget(i);
        if (w != m_selectionPage) {
            m_centralStack->removeWidget(w);
            w->deleteLater();
        }
    }

    auto* page = new QWidget(this);
    page->setObjectName(QStringLiteral("netSelectionPage"));
    page->setAttribute(Qt::WA_StyledBackground, true);
    page->setStyleSheet(Theme::pageBackground("netSelectionPage"));
    auto* main = new QVBoxLayout(page);
    main->setContentsMargins(24, 16, 24, 16);
    main->setSpacing(12);

    // 标题
    auto* title = new QLabel(QStringLiteral("三国杀 — 联网对战"), page);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(Theme::pageTitle(24) + QStringLiteral(" padding: 8px;"));
    main->addWidget(title);

    // 状态提示（如本机 IP 地址）
    if (!statusHint.isEmpty()) {
        auto* hintLabel = new QLabel(statusHint, page);
        hintLabel->setAlignment(Qt::AlignCenter);
        hintLabel->setStyleSheet(Theme::infoBar(Theme::P1Accent, "#16222E", "#3A5A78"));
        main->addWidget(hintLabel);
    }

    auto* subtitle = new QLabel(
        QStringLiteral("%1 — 请选择你的武将").arg(playerName), page);
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet(Theme::pageSubtitle() + QStringLiteral(" margin-bottom: 8px;"));
    main->addWidget(subtitle);

    // 武将卡片网格
    auto* grid = new QGridLayout;
    grid->setSpacing(8);

    auto* group = new QButtonGroup(page);
    group->setExclusive(true);

    for (int i = 0; i < CHAR_COUNT; ++i) {
        auto* card = new QGroupBox(page);
        card->setStyleSheet(Theme::charCard());

        auto* cl = new QVBoxLayout(card);
        cl->setSpacing(4);
        cl->setContentsMargins(8, 8, 8, 8);

        auto* radio = new QRadioButton(CHAR_LIST[i].name, card);
        group->addButton(radio, i);

        cl->addWidget(radio);
        // 武将头像
        QPixmap portrait = loadCharacterPortrait(CHAR_LIST[i].name, 60);
        if (!portrait.isNull()) {
            auto* avatar = new QLabel(card);
            avatar->setPixmap(portrait);
            avatar->setFixedSize(60, 60);
            avatar->setAlignment(Qt::AlignCenter);
            avatar->setStyleSheet("QLabel { background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
                "stop:0 #8C2B22, stop:1 #5A1611); border-radius: 30px; }");
            cl->addWidget(avatar);
        }
        cl->addWidget(new QLabel(QStringLiteral("体力: %1").arg(CHAR_LIST[i].maxHp), card));
        cl->addWidget(new QLabel(
            QStringLiteral("【%1】%2").arg(CHAR_LIST[i].skillName, CHAR_LIST[i].skillDesc), card));
        grid->addWidget(card, i / 3, i % 3);
    }
    main->addLayout(grid);
    // 3列网格宽度可能不够，加stretch保持居中

    // 确认选择按钮（暗松绿）
    auto* confirmBtn = new QPushButton(QStringLiteral("确认选择"), page);
    confirmBtn->setMinimumHeight(44);
    confirmBtn->setEnabled(false);
    confirmBtn->setStyleSheet(Theme::gradientButton(
        "#3E7A46", "#285230", "#4E9458", "#33663C", "#1D3E24", 16));

    connect(group, QOverload<int>::of(&QButtonGroup::idClicked), confirmBtn, [confirmBtn, group](int) {
        confirmBtn->setEnabled(group->checkedId() >= 0);
    });
    connect(confirmBtn, &QPushButton::clicked, this, [this, group]() {
        int id = group->checkedId();
        if (id >= 0) emit characterConfirmed(id);
    });
    main->addWidget(confirmBtn, 0, Qt::AlignCenter);

    m_centralStack->addWidget(page);
    m_centralStack->setCurrentWidget(page);
    page->setFocus();
}

void MainWindow::showWaitingForOpponent()
{
    // 在当前页面基础上覆盖等待提示（确认按钮已隐藏）
    QWidget* current = m_centralStack->currentWidget();
    if (!current || current == m_selectionPage) return;

    // 找到当前页面的布局，追加等待标签
    auto* layout = qobject_cast<QVBoxLayout*>(current->layout());
    if (!layout) return;

    // 禁用所有按钮
    for (auto* btn : current->findChildren<QPushButton*>())
        btn->setEnabled(false);

    auto* waitLabel = new QLabel(QStringLiteral("已选择武将，等待对手选择..."), current);
    waitLabel->setAlignment(Qt::AlignCenter);
    waitLabel->setStyleSheet(
        Theme::infoBar(Theme::P2Accent, "#33220E", "#8C6428", 15)
        + QStringLiteral(" padding: 12px 24px;"));
    layout->addWidget(waitLabel);
}

// ==================== 联网模式入口 ====================

void MainWindow::setupNetworkSection(QVBoxLayout* parent)
{
    // 分割线
    auto* separator = new QFrame(m_selectionPage);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet(Theme::separator());
    parent->addWidget(separator);

    auto* netRow = new QHBoxLayout;
    netRow->setSpacing(16);
    netRow->setAlignment(Qt::AlignCenter);

    // "联网模式"指示标签
    auto* netLabel = new QLabel(QStringLiteral("联网对战"), m_selectionPage);
    netLabel->setStyleSheet(Theme::accentText(Theme::Gold, 14));
    netRow->addWidget(netLabel);

    // 创建房间（暗青蓝）
    m_createRoomBtn = new QPushButton(QStringLiteral("创建房间（房主）"), m_selectionPage);
    m_createRoomBtn->setStyleSheet(Theme::gradientButton(
        "#33526E", "#22374C", "#40668A", "#2C4660", "#1A2A3A", 14));
    connect(m_createRoomBtn, &QPushButton::clicked, this, &MainWindow::onCreateRoomClicked);
    netRow->addWidget(m_createRoomBtn);

    // 加入房间（暗黛青）
    m_joinRoomBtn = new QPushButton(QStringLiteral("加入房间"), m_selectionPage);
    m_joinRoomBtn->setStyleSheet(Theme::gradientButton(
        "#2C5E56", "#1D423C", "#38766C", "#26514A", "#15302B", 14));
    connect(m_joinRoomBtn, &QPushButton::clicked, this, &MainWindow::onJoinRoomClicked);
    netRow->addWidget(m_joinRoomBtn);

    parent->addLayout(netRow);

    // 联网状态标签
    m_networkStatusLabel = new QLabel(m_selectionPage);
    m_networkStatusLabel->setAlignment(Qt::AlignCenter);
    m_networkStatusLabel->setVisible(false);
    parent->addWidget(m_networkStatusLabel);
}

void MainWindow::onCreateRoomClicked()
{
    // 即时 UI 反馈
    m_createRoomBtn->setEnabled(false);
    m_createRoomBtn->setText(QStringLiteral("创建房间中..."));
    m_joinRoomBtn->setEnabled(false);
    m_networkStatusLabel->setText(QStringLiteral("正在启动服务器，等待对手加入..."));
    m_networkStatusLabel->setStyleSheet(
        Theme::infoBar(Theme::P1Accent, "#16222E", "#3A5A78", 12)
        + QStringLiteral(" padding: 4px 8px;"));
    m_networkStatusLabel->setVisible(true);

    emit createRoomRequested();
}

void MainWindow::onJoinRoomClicked()
{
    auto* dlg = new NetworkConfigDialog(this);
    connect(dlg, &NetworkConfigDialog::connectRequested, this, [this](const QString& host, quint16 port) {
        // 即时 UI 反馈
        m_createRoomBtn->setEnabled(false);
        m_joinRoomBtn->setEnabled(false);
        m_networkStatusLabel->setText(QStringLiteral("正在连接到 %1:%2...").arg(host).arg(port));
        m_networkStatusLabel->setStyleSheet(
            Theme::infoBar("#63B8A8", "#12251F", "#2C5E56", 12)
            + QStringLiteral(" padding: 4px 8px;"));
        m_networkStatusLabel->setVisible(true);
        emit joinRoomRequested(host, port);
    });
    dlg->exec();
    dlg->deleteLater();
}

void MainWindow::setNetworkStatusText(const QString& text)
{
    m_networkStatusLabel->setText(text);
    m_networkStatusLabel->setVisible(true);
}

void MainWindow::resetNetworkState()
{
    m_createRoomBtn->setEnabled(true);
    m_createRoomBtn->setText(QStringLiteral("创建房间（房主）"));
    m_joinRoomBtn->setEnabled(true);
    if (m_networkStatusLabel) {
        m_networkStatusLabel->setVisible(false);
    }
}
