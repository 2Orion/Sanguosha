#include "PlayerInfoWidget.h"
#include "PlayerViewModel.h"

#include <QMetaObject>
#include <QStyle>
#include <QFont>
#include <QMouseEvent>

// 体力符号
static const QString FULL_HEART  = QStringLiteral("❤");   // ❤ 红心
static const QString EMPTY_HEART = QStringLiteral("🖤"); // 🖤 灰心

// ==================== 构造 / 析构 ====================

PlayerInfoWidget::PlayerInfoWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

PlayerInfoWidget::~PlayerInfoWidget()
{
    unbindViewModel();
}

// ==================== 绑定 / 解绑 ====================

void PlayerInfoWidget::bindViewModel(PlayerViewModel* pvm)
{
    if (m_pvm == pvm) return;
    unbindViewModel();
    m_pvm = pvm;
    if (m_pvm) {
        refreshAll();
        connectViewModelEvents();
    }
}

void PlayerInfoWidget::unbindViewModel()
{
    disconnectViewModelEvents();
    m_pvm = nullptr;
}

// ==================== 当前回合标记 ====================

void PlayerInfoWidget::setCurrentPlayer(bool isCurrent)
{
    m_isCurrent = isCurrent;
    if (m_turnIndicator) {
        m_turnIndicator->setVisible(isCurrent);
    }
    // 高亮边框
    if (isCurrent) {
        setStyleSheet(
            "PlayerInfoWidget {"
            "  border: 2px solid #FFD700;"
            "  border-radius: 8px;"
            "  background-color: #FFFDE7;"
            "}"
        );
    } else {
        setStyleSheet(
            "PlayerInfoWidget {"
            "  border: 1px solid #CCCCCC;"
            "  border-radius: 8px;"
            "  background-color: #FAFAFA;"
            "}"
        );
    }
}

// ==================== 鼠标事件 ====================

void PlayerInfoWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_pvm) {
        emit clicked(m_pvm->playerId());
    }
    QWidget::mousePressEvent(event);
}

// ==================== 目标选择标记 ====================

void PlayerInfoWidget::setTargetable(bool targetable)
{
    m_targetable = targetable;
    if (targetable) {
        setStyleSheet(
            "PlayerInfoWidget {"
            "  border: 2px solid #4CAF50;"
            "  border-radius: 8px;"
            "  background-color: #E8F5E9;"
            "}"
            "PlayerInfoWidget:hover {"
            "  background-color: #C8E6C9;"
            "  cursor: pointer;"
            "}"
        );
    } else {
        // 恢复到当前回合标记样式
        if (m_isCurrent) {
            setCurrentPlayer(true);
        } else {
            setStyleSheet(
                "PlayerInfoWidget {"
                "  border: 1px solid #CCCCCC;"
                "  border-radius: 8px;"
                "  background-color: #FAFAFA;"
                "}"
            );
        }
    }
}

// ==================== UI 构建 ====================

void PlayerInfoWidget::setupUi()
{
    // 主水平布局：头像 | 信息（姓名+体力+技能+手牌）| 回合标记
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 6, 8, 6);
    mainLayout->setSpacing(8);

    // ---- 头像占位（带颜色的 QLabel，显示武将名首字） ----
    m_avatarLabel = new QLabel(this);
    m_avatarLabel->setFixedSize(48, 48);
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    m_avatarLabel->setStyleSheet(
        "QLabel {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "    stop:0 #5C6BC0, stop:1 #3949AB);"
        "  color: white;"
        "  font-size: 20px;"
        "  font-weight: bold;"
        "  border-radius: 24px;"
        "}"
    );
    mainLayout->addWidget(m_avatarLabel);

    // ---- 信息区域（垂直排列） ----
    QVBoxLayout* infoLayout = new QVBoxLayout;
    infoLayout->setSpacing(2);

    // 第一行：姓名 + 手牌张数
    QHBoxLayout* nameRow = new QHBoxLayout;
    nameRow->setSpacing(6);

    m_nameLabel = new QLabel(this);
    m_nameLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #333;");
    nameRow->addWidget(m_nameLabel);

    m_handCountLabel = new QLabel(this);
    m_handCountLabel->setStyleSheet("font-size: 12px; color: #888;");
    nameRow->addWidget(m_handCountLabel);

    nameRow->addStretch();
    infoLayout->addLayout(nameRow);

    // 第二行：体力
    m_hpLabel = new QLabel(this);
    m_hpLabel->setStyleSheet("font-size: 14px;");
    infoLayout->addWidget(m_hpLabel);

    // 第三行：技能名
    m_skillLabel = new QLabel(this);
    m_skillLabel->setWordWrap(true);
    m_skillLabel->setStyleSheet("font-size: 11px; color: #666;");
    infoLayout->addWidget(m_skillLabel);

    mainLayout->addLayout(infoLayout, 1); // stretch=1 占据剩余空间

    // ---- 当前回合标记 ----
    m_turnIndicator = new QLabel(QStringLiteral("▶ 当前回合"), this); // ▶ 当前回合
    m_turnIndicator->setStyleSheet(
        "QLabel {"
        "  color: #E65100;"
        "  font-size: 11px;"
        "  font-weight: bold;"
        "  padding: 2px 6px;"
        "  background: #FFF3E0;"
        "  border: 1px solid #FFB74D;"
        "  border-radius: 4px;"
        "}"
    );
    m_turnIndicator->setVisible(false);
    mainLayout->addWidget(m_turnIndicator);

    // 默认样式
    setStyleSheet(
        "PlayerInfoWidget {"
        "  border: 1px solid #CCCCCC;"
        "  border-radius: 8px;"
        "  background-color: #FAFAFA;"
        "}"
    );
}

// ==================== 事件连接 ====================

void PlayerInfoWidget::connectViewModelEvents()
{
    if (!m_pvm) return;
    disconnectViewModelEvents();

    // hpChanged → 更新体力显示
    auto id1 = m_pvm->hpChanged.connect([this](int /*hp*/) {
        QMetaObject::invokeMethod(this, [this]() { updateHp(); }, Qt::QueuedConnection);
    });
    m_connectionIds.push_back(id1);

    // maxHpChanged → 更新体力显示
    auto id2 = m_pvm->maxHpChanged.connect([this](int /*maxHp*/) {
        QMetaObject::invokeMethod(this, [this]() { updateHp(); }, Qt::QueuedConnection);
    });
    m_connectionIds.push_back(id2);

    // handCardsChanged → 更新手牌计数
    auto id3 = m_pvm->handCardsChanged.connect([this]() {
        QMetaObject::invokeMethod(this, [this]() { updateHandCount(); }, Qt::QueuedConnection);
    });
    m_connectionIds.push_back(id3);

    // dying → 显示濒死效果
    auto id4 = m_pvm->dying.connect([this]() {
        QMetaObject::invokeMethod(this, [this]() { onDying(); }, Qt::QueuedConnection);
    });
    m_connectionIds.push_back(id4);

    // stateChanged → 全面刷新
    auto id5 = m_pvm->stateChanged.connect([this]() {
        QMetaObject::invokeMethod(this, [this]() { refreshAll(); }, Qt::QueuedConnection);
    });
    m_connectionIds.push_back(id5);
}

void PlayerInfoWidget::disconnectViewModelEvents()
{
    if (!m_pvm) return;
    for (auto id : m_connectionIds) {
        m_pvm->hpChanged.disconnect(id);
        m_pvm->maxHpChanged.disconnect(id);
        m_pvm->handCardsChanged.disconnect(id);
        m_pvm->dying.disconnect(id);
        m_pvm->stateChanged.disconnect(id);
    }
    m_connectionIds.clear();
}

// ==================== 更新方法 ====================

void PlayerInfoWidget::updateHp()
{
    if (!m_pvm) return;

    int hp = m_pvm->hp();
    int maxHp = m_pvm->maxHp();

    QString hpText;
    for (int i = 0; i < hp; ++i) {
        hpText += FULL_HEART + QStringLiteral(" ");
    }
    for (int i = hp; i < maxHp; ++i) {
        hpText += EMPTY_HEART + QStringLiteral(" ");
    }

    if (m_pvm->isDying()) {
        hpText += QStringLiteral(" ☠️"); // ☠️ 濒死标记
        m_hpLabel->setStyleSheet("font-size: 14px; color: #D32F2F; font-weight: bold;");
    } else {
        m_hpLabel->setStyleSheet("font-size: 14px;");
    }

    m_hpLabel->setText(hpText.trimmed());
}

void PlayerInfoWidget::updateHandCount()
{
    if (!m_pvm) return;
    int count = m_pvm->handCardCount();
    int limit = m_pvm->handCardLimit();
    m_handCountLabel->setText(
        QString(QStringLiteral("手牌×%1")) // 手牌×N
            .arg(count) +
        (count > limit ? QStringLiteral(" (超限%1)").arg(count - limit) : QString()) // (超限N)
    );
}

void PlayerInfoWidget::onDying()
{
    // 闪红效果 + 更新显示
    updateHp();
    m_hpLabel->setStyleSheet("font-size: 14px; color: #D32F2F; font-weight: bold;");

    // 背景闪烁指示（设置属性后刷新）
    QString origStyle = styleSheet();
    setStyleSheet(
        "PlayerInfoWidget {"
        "  border: 2px solid #D32F2F;"
        "  border-radius: 8px;"
        "  background-color: #FFEBEE;"
        "}"
    );
    // 用 QTimer 在短时间内恢复？先简单处理：只改变颜色
    Q_UNUSED(origStyle);
}

void PlayerInfoWidget::refreshAll()
{
    if (!m_pvm) return;

    // 姓名
    QString name = QString::fromStdString(m_pvm->displayName());
    QString charName = QString::fromStdString(m_pvm->characterName());
    if (!charName.isEmpty()) {
        m_nameLabel->setText(name + QStringLiteral(" [") + charName + QStringLiteral("]"));
    } else {
        m_nameLabel->setText(name);
    }

    // 头像（取武将名第一个字符）
    if (!charName.isEmpty()) {
        m_avatarLabel->setText(charName.left(1));
    } else {
        m_avatarLabel->setText(name.left(1));
    }

    // 体力
    updateHp();

    // 技能
    QString skill = QString::fromStdString(m_pvm->skillName());
    QString skillDesc = QString::fromStdString(m_pvm->skillDescription());
    if (!skill.isEmpty()) {
        if (!skillDesc.isEmpty()) {
            m_skillLabel->setText(skill + QStringLiteral(": ") + skillDesc);
        } else {
            m_skillLabel->setText(skill);
        }
    } else {
        m_skillLabel->clear();
    }

    // 手牌
    updateHandCount();
}
