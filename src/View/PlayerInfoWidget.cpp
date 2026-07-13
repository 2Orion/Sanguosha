#include "PlayerInfoWidget.h"
#include "PlayerViewModel.h"

#include <QMetaObject>
#include <QFont>
#include <QMouseEvent>

static const QString FULL_HEART  = QStringLiteral("❤");
static const QString EMPTY_HEART = QStringLiteral("🖤");

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
    if (!m_pvm) return;

    refreshAll();

    // 使用 Qt 信号槽桥接 PlayerViewModel 事件
    m_connections.push_back(
        connect(m_pvm, &PlayerViewModel::hpChanged, this, &PlayerInfoWidget::updateHp));
    m_connections.push_back(
        connect(m_pvm, &PlayerViewModel::maxHpChanged, this, &PlayerInfoWidget::updateHp));
    m_connections.push_back(
        connect(m_pvm, &PlayerViewModel::handCardsChanged, this, &PlayerInfoWidget::updateHandCount));
    m_connections.push_back(
        connect(m_pvm, &PlayerViewModel::dying, this, &PlayerInfoWidget::onDying));
    m_connections.push_back(
        connect(m_pvm, &PlayerViewModel::stateChanged, this, &PlayerInfoWidget::refreshAll));
}

void PlayerInfoWidget::unbindViewModel()
{
    for (auto& conn : m_connections) {
        disconnect(conn);
    }
    m_connections.clear();
    m_pvm = nullptr;
}

// ==================== 当前回合标记 ====================

void PlayerInfoWidget::setCurrentPlayer(bool isCurrent)
{
    m_isCurrent = isCurrent;
    if (m_turnIndicator) m_turnIndicator->setVisible(isCurrent);

    if (isCurrent) {
        setStyleSheet(
            "PlayerInfoWidget {"
            "  border: 2px solid #FFD700;"
            "  border-radius: 8px;"
            "  background-color: #FFFDE7;"
            "}");
    } else {
        setStyleSheet(
            "PlayerInfoWidget {"
            "  border: 1px solid #CCCCCC;"
            "  border-radius: 8px;"
            "  background-color: #FAFAFA;"
            "}");
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
            "}");
    } else {
        if (m_isCurrent) setCurrentPlayer(true);
        else {
            setStyleSheet(
                "PlayerInfoWidget {"
                "  border: 1px solid #CCCCCC;"
                "  border-radius: 8px;"
                "  background-color: #FAFAFA;"
                "}");
        }
    }
}

// ==================== UI 构建 ====================

void PlayerInfoWidget::setupUi()
{
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 6, 8, 6);
    mainLayout->setSpacing(8);

    m_avatarLabel = new QLabel(this);
    m_avatarLabel->setFixedSize(48, 48);
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    m_avatarLabel->setStyleSheet(
        "QLabel {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "    stop:0 #5C6BC0, stop:1 #3949AB);"
        "  color: white; font-size: 20px; font-weight: bold;"
        "  border-radius: 24px;"
        "}");
    mainLayout->addWidget(m_avatarLabel);

    QVBoxLayout* infoLayout = new QVBoxLayout;
    infoLayout->setSpacing(2);

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

    m_hpLabel = new QLabel(this);
    m_hpLabel->setStyleSheet("font-size: 14px;");
    infoLayout->addWidget(m_hpLabel);

    m_skillLabel = new QLabel(this);
    m_skillLabel->setWordWrap(true);
    m_skillLabel->setStyleSheet("font-size: 11px; color: #666;");
    infoLayout->addWidget(m_skillLabel);

    mainLayout->addLayout(infoLayout, 1);

    m_turnIndicator = new QLabel(QStringLiteral("▶ 当前回合"), this);
    m_turnIndicator->setStyleSheet(
        "QLabel {"
        "  color: #E65100; font-size: 11px; font-weight: bold;"
        "  padding: 2px 6px; background: #FFF3E0;"
        "  border: 1px solid #FFB74D; border-radius: 4px;"
        "}");
    m_turnIndicator->setVisible(false);
    mainLayout->addWidget(m_turnIndicator);

    setStyleSheet(
        "PlayerInfoWidget {"
        "  border: 1px solid #CCCCCC; border-radius: 8px;"
        "  background-color: #FAFAFA;"
        "}");
}

// ==================== 更新方法 ====================

void PlayerInfoWidget::updateHp()
{
    if (!m_pvm) return;
    int hp = m_pvm->hp();
    int maxHp = m_pvm->maxHp();

    QString hpText;
    for (int i = 0; i < hp; ++i) hpText += FULL_HEART + QStringLiteral(" ");
    for (int i = hp; i < maxHp; ++i) hpText += EMPTY_HEART + QStringLiteral(" ");

    if (m_pvm->isDying()) {
        hpText += QStringLiteral(" ☠️");
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
        QStringLiteral("手牌×%1").arg(count) +
        (count > limit ? QStringLiteral(" (超限%1)").arg(count - limit) : QString()));
}

void PlayerInfoWidget::onDying()
{
    updateHp();
    setStyleSheet(
        "PlayerInfoWidget {"
        "  border: 2px solid #D32F2F; border-radius: 8px;"
        "  background-color: #FFEBEE;"
        "}");
}

void PlayerInfoWidget::refreshAll()
{
    if (!m_pvm) return;

    QString name = m_pvm->displayName();
    QString charName = m_pvm->characterName();
    m_nameLabel->setText(charName.isEmpty()
        ? name
        : name + QStringLiteral(" [") + charName + QStringLiteral("]"));

    m_avatarLabel->setText(charName.isEmpty() ? name.left(1) : charName.left(1));

    updateHp();

    QString skill = m_pvm->skillName();
    if (!skill.isEmpty()) {
        QString skillDesc = m_pvm->skillDescription();
        m_skillLabel->setText(skillDesc.isEmpty() ? skill : skill + QStringLiteral(": ") + skillDesc);
    } else {
        m_skillLabel->clear();
    }

    updateHandCount();
}
