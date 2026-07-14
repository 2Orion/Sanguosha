#include "PlayerInfoWidget.h"
#include <QFont>
#include <QMouseEvent>

static const QString FULL_HEART  = QStringLiteral("❤");
static const QString EMPTY_HEART = QStringLiteral("🖤");

PlayerInfoWidget::PlayerInfoWidget(QWidget* parent) : QWidget(parent) { setupUi(); }

void PlayerInfoWidget::setDisplayData(const PlayerData& data)
{
    m_playerId = data.playerId;

    m_avatarLabel->setText(data.characterName.isEmpty() ? data.displayName.left(1) : data.characterName.left(1));

    QString name = data.displayName;
    if (!data.characterName.isEmpty())
        name += QStringLiteral(" [") + data.characterName + QStringLiteral("]");
    m_nameLabel->setText(name);

    m_handCountLabel->setText(QStringLiteral("手牌×%1").arg(data.handCardCount)
        + (data.handCardCount > data.handCardLimit ? QStringLiteral(" (超限%1)").arg(data.handCardCount - data.handCardLimit) : QString()));

    setCurrentPlayer(data.isCurrentPlayer);

    // 体力
    QString hpText;
    for (int i = 0; i < data.hp; ++i) hpText += FULL_HEART + " ";
    for (int i = data.hp; i < data.maxHp; ++i) hpText += EMPTY_HEART + " ";
    if (data.isDying) hpText += QStringLiteral(" ☠️");
    m_hpLabel->setText(hpText.trimmed());
    m_hpLabel->setStyleSheet(data.isDying ? "font-size: 14px; color: #D32F2F; font-weight: bold;" : "font-size: 14px;");

    // 技能
    if (!data.skillName.isEmpty()) {
        m_skillLabel->setText(data.skillDescription.isEmpty()
            ? data.skillName
            : data.skillName + QStringLiteral(": ") + data.skillDescription);
    } else {
        m_skillLabel->clear();
    }

    if (data.isDying) {
        setStyleSheet(
            "PlayerInfoWidget {"
            "  border: 2px solid #D32F2F; border-radius: 8px;"
            "  background-color: #FFEBEE;"
            "}");
    }
}

void PlayerInfoWidget::setCurrentPlayer(bool isCurrent)
{
    m_isCurrent = isCurrent;
    if (m_turnIndicator) m_turnIndicator->setVisible(isCurrent);
    setStyleSheet(isCurrent
        ? "PlayerInfoWidget { border: 2px solid #FFD700; border-radius: 8px; background-color: #FFFDE7; }"
        : "PlayerInfoWidget { border: 1px solid #CCCCCC; border-radius: 8px; background-color: #FAFAFA; }");
}

void PlayerInfoWidget::setTargetable(bool targetable)
{
    m_targetable = targetable;
    setStyleSheet(targetable
        ? "PlayerInfoWidget { border: 2px solid #4CAF50; border-radius: 8px; background-color: #E8F5E9; }"
            "PlayerInfoWidget:hover { background-color: #C8E6C9; cursor: pointer; }"
        : m_isCurrent
            ? "PlayerInfoWidget { border: 2px solid #FFD700; border-radius: 8px; background-color: #FFFDE7; }"
            : "PlayerInfoWidget { border: 1px solid #CCCCCC; border-radius: 8px; background-color: #FAFAFA; }");
}

void PlayerInfoWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_playerId >= 0)
        emit clicked(m_playerId);
    QWidget::mousePressEvent(event);
}

void PlayerInfoWidget::setupUi()
{
    auto* main = new QHBoxLayout(this);
    main->setContentsMargins(8, 6, 8, 6); main->setSpacing(8);

    m_avatarLabel = new QLabel(this);
    m_avatarLabel->setFixedSize(48, 48); m_avatarLabel->setAlignment(Qt::AlignCenter);
    m_avatarLabel->setStyleSheet("QLabel { background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "stop:0 #5C6BC0, stop:1 #3949AB); color: white; font-size: 20px; font-weight: bold; border-radius: 24px; }");
    main->addWidget(m_avatarLabel);

    auto* info = new QVBoxLayout; info->setSpacing(2);
    auto* nameRow = new QHBoxLayout; nameRow->setSpacing(6);

    m_nameLabel = new QLabel(this); m_nameLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #333;");
    nameRow->addWidget(m_nameLabel);
    m_handCountLabel = new QLabel(this); m_handCountLabel->setStyleSheet("font-size: 12px; color: #888;");
    nameRow->addWidget(m_handCountLabel); nameRow->addStretch();
    info->addLayout(nameRow);

    m_hpLabel = new QLabel(this); m_hpLabel->setStyleSheet("font-size: 14px;"); info->addWidget(m_hpLabel);
    m_skillLabel = new QLabel(this); m_skillLabel->setWordWrap(true); m_skillLabel->setStyleSheet("font-size: 11px; color: #666;");
    info->addWidget(m_skillLabel);
    main->addLayout(info, 1);

    m_turnIndicator = new QLabel(QStringLiteral("▶ 当前回合"), this);
    m_turnIndicator->setStyleSheet("QLabel { color: #E65100; font-size: 11px; font-weight: bold;"
        " padding: 2px 6px; background: #FFF3E0; border: 1px solid #FFB74D; border-radius: 4px; }");
    m_turnIndicator->setVisible(false);
    main->addWidget(m_turnIndicator);
    setStyleSheet("PlayerInfoWidget { border: 1px solid #CCCCCC; border-radius: 8px; background-color: #FAFAFA; }");
}
