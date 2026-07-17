#include "PlayerInfoWidget.h"
#include "Theme.h"
#include <QFont>
#include <QMouseEvent>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QApplication>

/// 加载武将头像（返回圆形裁剪），同 MainWindow 中的版本一致
static QPixmap loadCharacterPortrait(const QString& charName, int size)
{
    QString base = QApplication::applicationDirPath() + QStringLiteral("/images/%1").arg(charName);
    QPixmap px(base + QStringLiteral(".png"));
    if (px.isNull()) px = QPixmap(base + QStringLiteral(".jpg"));
    if (px.isNull()) return {};
    px = px.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
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

static const QString FULL_HEART  = QStringLiteral("❤");
static const QString EMPTY_HEART = QStringLiteral("❌");   // 带×的心表示已损失体力
static const QString WEAPON_ICON = QStringLiteral("⚔");
static const QString ARMOR_ICON  = QStringLiteral("🛡");

PlayerInfoWidget::PlayerInfoWidget(QWidget* parent) : QWidget(parent) { setupUi(); }

void PlayerInfoWidget::setDisplayData(const PlayerData& data)
{
    m_playerId = data.playerId;

    // 设置武将头像（优先加载图片，无图片时显示首字符）
    if (!data.characterName.isEmpty()) {
        QPixmap portrait = loadCharacterPortrait(data.characterName, 48);
        if (!portrait.isNull()) {
            m_avatarLabel->setPixmap(portrait);
        } else {
            m_avatarLabel->setText(data.characterName.left(1));
            m_avatarLabel->setPixmap(QPixmap()); // 清除 pixmap，回到文本模式
        }
    } else {
        m_avatarLabel->setText(data.displayName.left(1));
    }

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
    m_hpLabel->setStyleSheet(data.isDying
        ? QStringLiteral("font-size: 14px; color: %1; font-weight: bold;").arg(QLatin1String(Theme::DyingRed))
        : QStringLiteral("font-size: 14px;"));

    // 技能
    if (!data.skillName.isEmpty()) {
        m_skillLabel->setText(data.skillDescription.isEmpty()
            ? data.skillName
            : data.skillName + QStringLiteral(": ") + data.skillDescription);
    } else {
        m_skillLabel->clear();
    }

    if (data.isDying) {
        setStyleSheet(Theme::panelDying());
    }

    // ===== 装备区 =====
    QString equipStr;
    for (const CardData& eq : data.equipCards) {
        if (eq.cardId < 0) continue;
        if (eq.equipSlot == 0) {  // EquipSlot::Weapon
            if (!equipStr.isEmpty()) equipStr += QStringLiteral(" | ");
            equipStr += WEAPON_ICON + QStringLiteral(" ") + eq.cardName;
            if (data.attackRange > 1)
                equipStr += QStringLiteral("(范围%1)").arg(data.attackRange);
        } else if (eq.equipSlot == 1) {  // EquipSlot::Armor
            if (!equipStr.isEmpty()) equipStr += QStringLiteral(" | ");
            equipStr += ARMOR_ICON + QStringLiteral(" ") + eq.cardName;
        }
    }
    if (!equipStr.isEmpty()) {
        m_equipLabel->setText(QStringLiteral("装备: ") + equipStr);
        m_equipLabel->setVisible(true);
    } else {
        m_equipLabel->clear();
        m_equipLabel->setVisible(false);
    }
    if (data.attackRange > 1) {
        m_attackRangeLabel->setText(QStringLiteral("攻击范围%1").arg(data.attackRange));
        m_attackRangeLabel->setVisible(data.isCurrentPlayer);
    } else {
        m_attackRangeLabel->setVisible(false);
    }

    // ===== 判定区 =====
    if (!data.judgmentCards.isEmpty()) {
        QStringList jd;
        for (const CardData& jc : data.judgmentCards) {
            if (jc.cardId < 0) continue;
            jd << QStringLiteral("🔮 ") + jc.cardName;
        }
        m_judgmentLabel->setText(QStringLiteral("判定: ") + jd.join(QStringLiteral(" | ")));
        m_judgmentLabel->setVisible(true);
    } else {
        m_judgmentLabel->setVisible(false);
    }
}

void PlayerInfoWidget::setCurrentPlayer(bool isCurrent)
{
    m_isCurrent = isCurrent;
    if (m_turnIndicator) m_turnIndicator->setVisible(isCurrent);
    setStyleSheet(isCurrent ? Theme::panelCurrent() : Theme::panelNormal());
}

void PlayerInfoWidget::setTargetable(bool targetable)
{
    m_targetable = targetable;
    setStyleSheet(targetable
        ? Theme::panelTargetable()
        : m_isCurrent ? Theme::panelCurrent() : Theme::panelNormal());
}

void PlayerInfoWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_playerId >= 0)
        emit clicked(m_playerId);
    QWidget::mousePressEvent(event);
}

void PlayerInfoWidget::setupUi()
{
    // QSS 的 background/border 要在 QWidget 直接子类上生效必须开启 WA_StyledBackground
    setAttribute(Qt::WA_StyledBackground, true);

    auto* main = new QHBoxLayout(this);
    main->setContentsMargins(8, 6, 8, 6); main->setSpacing(8);

    m_avatarLabel = new QLabel(this);
    m_avatarLabel->setFixedSize(48, 48); m_avatarLabel->setAlignment(Qt::AlignCenter);
    m_avatarLabel->setStyleSheet("QLabel { background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "stop:0 #8C2B22, stop:1 #5A1611); color: #F0E0B8; font-size: 20px; font-weight: bold;"
        " border: 1px solid #D4AF37; border-radius: 24px; }");
    main->addWidget(m_avatarLabel);

    auto* info = new QVBoxLayout; info->setSpacing(2);
    auto* nameRow = new QHBoxLayout; nameRow->setSpacing(6);

    m_nameLabel = new QLabel(this);
    m_nameLabel->setStyleSheet(QStringLiteral("font-size: 14px; font-weight: bold; color: %1;")
                                   .arg(QLatin1String(Theme::TextPrimary)));
    nameRow->addWidget(m_nameLabel);
    m_handCountLabel = new QLabel(this);
    m_handCountLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: %1;")
                                        .arg(QLatin1String(Theme::TextMuted)));
    nameRow->addWidget(m_handCountLabel); nameRow->addStretch();
    info->addLayout(nameRow);

    m_hpLabel = new QLabel(this); m_hpLabel->setStyleSheet("font-size: 14px;"); info->addWidget(m_hpLabel);

    // 装备区
    auto* equipRow = new QHBoxLayout; equipRow->setSpacing(4);
    m_equipLabel = new QLabel(this);
    m_equipLabel->setStyleSheet(Theme::badge("#D7B98C", "#2A2018", "#5A4632"));
    equipRow->addWidget(m_equipLabel);
    m_attackRangeLabel = new QLabel(this);
    m_attackRangeLabel->setStyleSheet(Theme::badge("#FFB74D", "#33220E", "#8C6428", 10));
    equipRow->addWidget(m_attackRangeLabel);
    equipRow->addStretch();
    info->addLayout(equipRow);

    // 判定区
    m_judgmentLabel = new QLabel(this);
    m_judgmentLabel->setStyleSheet(Theme::badge("#CE93D8", "#2A1830", "#6A4A78"));
    m_judgmentLabel->setVisible(false);
    info->addWidget(m_judgmentLabel);

    m_skillLabel = new QLabel(this); m_skillLabel->setWordWrap(true);
    m_skillLabel->setStyleSheet(QStringLiteral("font-size: 11px; color: %1;")
                                    .arg(QLatin1String(Theme::TextMuted)));
    info->addWidget(m_skillLabel);
    main->addLayout(info, 1);

    m_turnIndicator = new QLabel(QStringLiteral("▶ 当前回合"), this);
    m_turnIndicator->setStyleSheet(Theme::badge("#D4AF37", "#33270F", "#9C7E24"));
    m_turnIndicator->setVisible(false);
    main->addWidget(m_turnIndicator);
    setStyleSheet(Theme::panelNormal());
}
