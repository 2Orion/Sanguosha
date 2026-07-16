#include "CardWidget.h"
#include "Theme.h"
#include <QPainter>
#include <QMouseEvent>
#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QPainterPath>
#include <QVariantAnimation>
#include <QTimer>

CardWidget::CardWidget(bool faceUp, QWidget* parent)
    : QFrame(parent), m_faceUp(faceUp)
{
    setFixedSize(CARD_WIDTH, CARD_HEIGHT);
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);

    m_liftAnimation = new QVariantAnimation(this);
    m_liftAnimation->setDuration(120);
    m_liftAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_liftAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& v) { m_liftAnim = v.toDouble(); update(); });
}

QRect CardWidget::cardFaceRect() const
{
    // 扇形模式：牌面居中在外扩控件的中央 80×112 区域，四周留 FAN_MARGIN
    if (m_fanMode)
        return QRect(FAN_MARGIN + 1, FAN_MARGIN + 1, CARD_WIDTH - 2, CARD_HEIGHT - 2);
    return QRect(1, 1, width() - 2, height() - 2);
}

int CardWidget::targetLift() const
{
    // 选中最高、悬停次之，叠加扇形基线抬升
    int dynamic = m_selected ? -12 : (m_hovered ? -6 : 0);
    return m_baseLift + dynamic;
}

void CardWidget::animateLift()
{
    double target = targetLift();
    if (qFuzzyCompare(m_liftAnim, target)) return;
    m_liftAnimation->stop();
    m_liftAnimation->setStartValue(m_liftAnim);
    m_liftAnimation->setEndValue(target);
    m_liftAnimation->start();
}

void CardWidget::setFanMode(bool enabled, double rotationDeg, int baseLift)
{
    m_fanMode = enabled;
    m_rotationDeg = rotationDeg;
    m_baseLift = baseLift;
    if (enabled)
        setFixedSize(CARD_WIDTH + 2 * FAN_MARGIN, CARD_HEIGHT + 2 * FAN_MARGIN);
    else
        setFixedSize(CARD_WIDTH, CARD_HEIGHT);
    m_liftAnim = targetLift();   // 基线立即生效，不为静态摆放播动画
    update();
}

void CardWidget::playEntranceFade(int delayMs)
{
    if (!m_fadeAnimation) {
        m_fadeAnimation = new QVariantAnimation(this);
        m_fadeAnimation->setDuration(200);
        m_fadeAnimation->setEasingCurve(QEasingCurve::OutQuad);
        connect(m_fadeAnimation, &QVariantAnimation::valueChanged, this,
                [this](const QVariant& v) { m_opacity = v.toDouble(); update(); });
    }
    m_opacity = 0.0;
    update();
    auto start = [this]() {
        m_fadeAnimation->stop();
        m_fadeAnimation->setStartValue(0.0);
        m_fadeAnimation->setEndValue(1.0);
        m_fadeAnimation->start();
    };
    if (delayMs > 0) QTimer::singleShot(delayMs, this, start);
    else start();
}

void CardWidget::setDisplayData(const CardData& data)
{
    m_data = data;
    QString tip = data.cardName;
    if (!data.description.isEmpty())
        tip += QStringLiteral("\n") + data.description;
    setToolTip(tip);
    update();
}

void CardWidget::setFaceUp(bool faceUp, bool showPlayable)
{
    m_faceUp = faceUp;
    m_showPlayable = showPlayable;
    update();
}

void CardWidget::setSelected(bool sel)
{
    if (m_selected != sel) { m_selected = sel; animateLift(); update(); }
}

void CardWidget::setPlayable(bool playable)
{
    if (m_playable != playable) { m_playable = playable; update(); }
}

void CardWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_data.cardId >= 0)
        emit clicked(m_data.cardId);
    QFrame::mousePressEvent(event);
}

void CardWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_data.cardId >= 0)
        emit doubleClicked(m_data.cardId);
    QFrame::mouseDoubleClickEvent(event);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void CardWidget::enterEvent(QEnterEvent*) { m_hovered = true; animateLift(); update(); }
#else
void CardWidget::enterEvent(QEvent*) { m_hovered = true; animateLift(); update(); }
#endif

void CardWidget::leaveEvent(QEvent*) { m_hovered = false; animateLift(); update(); }

void CardWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (m_opacity < 1.0) painter.setOpacity(qMax(0.0, m_opacity));

    int radius = 6;
    QRect cardRect = cardFaceRect();
    int lift = static_cast<int>(m_liftAnim);

    // 扇形模式绕牌面中心旋转；上浮为向上平移。二者均为绘制层，控件几何不变。
    painter.save();
    if (m_fanMode && !qFuzzyIsNull(m_rotationDeg)) {
        QPointF c = cardRect.center();
        painter.translate(c);
        painter.rotate(m_rotationDeg);
        painter.translate(-c);
    }
    painter.translate(0, lift);

    // 投影
    QPainterPath shadowPath;
    shadowPath.addRoundedRect(cardRect.adjusted(2, 4, 2, 4), radius, radius);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, m_fanMode ? 60 : 50));
    painter.drawPath(shadowPath);

    if (!m_faceUp) drawCardBack(painter, cardRect, radius);
    else drawCardFront(painter, cardRect, radius);
    drawStateOverlay(painter, cardRect, radius);
    painter.restore();
}

void CardWidget::drawCardBack(QPainter& painter, const QRect& rect, int radius)
{
    // 暗红纸底渐变
    QLinearGradient bg(rect.topLeft(), rect.bottomRight());
    bg.setColorAt(0.0, Theme::CardBackTop);
    bg.setColorAt(0.5, QColor(108, 26, 24));
    bg.setColorAt(1.0, Theme::CardBackBottom);
    QPainterPath p; p.addRoundedRect(rect, radius, radius);
    painter.setPen(QPen(QColor(64, 12, 12), 1));
    painter.setBrush(bg);
    painter.drawPath(p);

    // 金色菱形网格纹理（裁剪在牌面内）
    painter.save();
    painter.setClipPath(p);
    painter.setPen(QPen(QColor(Theme::CardBackGold.red(), Theme::CardBackGold.green(),
                                Theme::CardBackGold.blue(), 46), 1));
    const int step = 11;
    for (int d = -rect.height(); d < rect.width(); d += step) {
        painter.drawLine(rect.left() + d, rect.top(), rect.left() + d + rect.height(), rect.bottom());
        painter.drawLine(rect.left() + d, rect.bottom(), rect.left() + d + rect.height(), rect.top());
    }
    painter.restore();

    // 内嵌金框（双线）
    QRect inner = rect.adjusted(5, 5, -5, -5);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(Theme::CardBackGold.red(), Theme::CardBackGold.green(),
                                Theme::CardBackGold.blue(), 150), 1));
    painter.drawRoundedRect(inner, radius - 2, radius - 2);
    painter.setPen(QPen(QColor(Theme::CardBackGold.red(), Theme::CardBackGold.green(),
                                Theme::CardBackGold.blue(), 70), 1));
    painter.drawRoundedRect(inner.adjusted(2, 2, -2, -2), radius - 3, radius - 3);

    // 中央双层金菱徽记
    int cx = rect.center().x(), cy = rect.center().y();
    auto diamond = [&](int r) {
        QPainterPath d;
        d.moveTo(cx, cy - r); d.lineTo(cx + r * 3 / 4, cy);
        d.lineTo(cx, cy + r); d.lineTo(cx - r * 3 / 4, cy);
        d.closeSubpath();
        return d;
    };
    painter.setPen(QPen(Theme::CardBackGold, 1.5));
    painter.setBrush(QColor(Theme::CardBackGold.red(), Theme::CardBackGold.green(),
                             Theme::CardBackGold.blue(), 55));
    painter.drawPath(diamond(16));
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(Theme::CardBackGold.red(), Theme::CardBackGold.green(),
                                Theme::CardBackGold.blue(), 120), 1));
    painter.drawPath(diamond(9));
}

void CardWidget::drawCardFront(QPainter& painter, const QRect& rect, int radius)
{
    // 空占位牌（cardId<0）：仅纸底 + 内描边，供牌区占位
    if (m_data.cardId < 0) {
        QPainterPath p; p.addRoundedRect(rect, radius, radius);
        painter.setPen(QPen(Theme::CardBorderInner, 1));
        painter.setBrush(Theme::CardPaperEmpty);
        painter.drawPath(p);
        return;
    }

    bool isRed = m_data.color == CardColor::Red;
    QColor suitColor = isRed ? Theme::CardSuitRed : Theme::CardSuitBlack;
    QColor nameColor = isRed ? Theme::CardNameRed : Theme::CardNameBlack;

    // 纸质暖白渐变底
    QLinearGradient paper(rect.topLeft(), rect.bottomLeft());
    paper.setColorAt(0.0, Theme::CardPaperTop);
    paper.setColorAt(1.0, Theme::CardPaperBottom);
    QPainterPath path; path.addRoundedRect(rect, radius, radius);
    painter.setPen(Qt::NoPen);
    painter.setBrush(paper);
    painter.drawPath(path);

    // 双线描边：外线（装备牌换青绿）+ 内线浅金
    QColor outerColor = m_data.isEquipment ? Theme::CardEquipBorder : Theme::CardBorderOuter;
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(outerColor, m_data.isEquipment ? 2.0 : 1.5));
    painter.drawRoundedRect(rect, radius, radius);
    painter.setPen(QPen(Theme::CardBorderInner, 1));
    painter.drawRoundedRect(rect.adjusted(3, 3, -3, -3), radius - 2, radius - 2);

    // 大花色水印（居中偏下，半透明；先画，随后被卡名覆盖在上层）
    {
        QFont wf(QStringLiteral("Segoe UI Symbol"), 40, QFont::Bold);
        painter.setFont(wf);
        painter.setPen(QColor(suitColor.red(), suitColor.green(), suitColor.blue(), 26));
        painter.drawText(rect.adjusted(0, 14, 0, -2), Qt::AlignCenter, m_data.suitSymbol);
    }

    // 左上角标：数字在上、花色在下（竖排）
    {
        QFont numF(QStringLiteral("Microsoft YaHei"), 10, QFont::Bold);
        painter.setFont(numF);
        painter.setPen(suitColor);
        painter.drawText(QRect(rect.left() + 5, rect.top() + 3, 22, 15),
                         Qt::AlignLeft | Qt::AlignVCenter, m_data.numberString);
        QFont suitF(QStringLiteral("Segoe UI Symbol"), 11, QFont::Bold);
        painter.setFont(suitF);
        painter.drawText(QRect(rect.left() + 5, rect.top() + 17, 22, 16),
                         Qt::AlignLeft | Qt::AlignVCenter, m_data.suitSymbol);
    }

    // 卡名居中
    {
        QString name = m_data.cardName;
        int sz = name.length() <= 2 ? 15 : (name.length() <= 3 ? 12 : 10);
        QFont nameF(QStringLiteral("Microsoft YaHei"), sz, QFont::Bold);
        painter.setFont(nameF);
        painter.setPen(nameColor);
        painter.drawText(QRect(rect.left() + 2, rect.top() + 30, rect.width() - 4, rect.height() - 52),
                         Qt::AlignCenter | Qt::TextWordWrap, name);
    }

    // 底部类型徽章（圆角胶囊，居中）
    QString typeLabel;
    QColor badgeBg = Theme::BadgeBasicBg;
    if (m_data.isBasic) {
        typeLabel = QStringLiteral("基本");
        badgeBg = Theme::BadgeBasicBg;
    } else if (m_data.isStrategy) {
        typeLabel = QStringLiteral("锦囊");
        badgeBg = Theme::BadgeStrategyBg;
    } else if (m_data.isEquipment) {
        typeLabel = QStringLiteral("装备");
        badgeBg = Theme::BadgeEquipBg;
        if (m_data.equipSlot == 0)        // EquipSlot::Weapon
            typeLabel = QStringLiteral("⚔ ") + typeLabel;
        else if (m_data.equipSlot == 1)   // EquipSlot::Armor
            typeLabel = QStringLiteral("🛡 ") + typeLabel;
        if (m_data.equipSlot == 0 && m_data.attackRange > 0)   // 武器攻击范围
            typeLabel += QStringLiteral(" +%1").arg(m_data.attackRange);
    }
    if (!typeLabel.isEmpty()) {
        QFont bf(QStringLiteral("Microsoft YaHei"), 7, QFont::Bold);
        painter.setFont(bf);
        int tw = painter.fontMetrics().horizontalAdvance(typeLabel);
        int bw = qMin(rect.width() - 8, tw + 12);
        int bh = 15;
        QRect badge(rect.center().x() - bw / 2, rect.bottom() - bh - 3, bw, bh);
        QPainterPath bp; bp.addRoundedRect(badge, 7, 7);
        painter.setPen(Qt::NoPen);
        painter.setBrush(badgeBg);
        painter.drawPath(bp);
        painter.setPen(QPen(QColor(Theme::CardBackGold.red(), Theme::CardBackGold.green(),
                                    Theme::CardBackGold.blue(), 90), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(bp);
        painter.setPen(Theme::BadgeText);
        painter.drawText(badge, Qt::AlignCenter, typeLabel);
    }
}

void CardWidget::drawStateOverlay(QPainter& painter, const QRect& rect, int radius)
{
    bool effectivePlayable = m_playable || (m_showPlayable && m_data.isPlayable);
    QPainterPath path; path.addRoundedRect(rect, radius, radius);

    // 可打出：绿色描边 + 外扩柔光晕
    if (effectivePlayable) {
        QColor g = Theme::OverlayPlayable;
        QRadialGradient glow(rect.center(), rect.width() / 2 + 6);
        glow.setColorAt(0.0, QColor(g.red(), g.green(), g.blue(), 34));
        glow.setColorAt(1.0, QColor(g.red(), g.green(), g.blue(), 0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(glow);
        painter.drawRoundedRect(rect.adjusted(-5, -5, 5, 5), radius + 3, radius + 3);
        painter.setPen(QPen(g, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }

    // 选中：蓝色双描边 + 极淡蓝罩
    if (m_selected) {
        QColor s = Theme::OverlaySelected;
        painter.setPen(QPen(s, 3));
        painter.setBrush(QColor(s.red(), s.green(), s.blue(), 20));
        painter.drawPath(path);
        painter.setPen(QPen(QColor(255, 255, 255, 90), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(rect.adjusted(2, 2, -2, -2), radius - 1, radius - 1);
    }

    // 悬停（未选中、非可打出）：暖光罩
    if (m_hovered && !m_selected && !effectivePlayable) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(Theme::OverlayHoverGlow);
        painter.drawPath(path);
    }
}
