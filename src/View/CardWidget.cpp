#include "CardWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QPainterPath>

CardWidget::CardWidget(bool faceUp, QWidget* parent)
    : QFrame(parent), m_faceUp(faceUp)
{
    setFixedSize(CARD_WIDTH, CARD_HEIGHT);
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
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
    if (m_selected != sel) { m_selected = sel; update(); }
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
void CardWidget::enterEvent(QEnterEvent*) { m_hovered = true; update(); }
#else
void CardWidget::enterEvent(QEvent*) { m_hovered = true; update(); }
#endif

void CardWidget::leaveEvent(QEvent*) { m_hovered = false; update(); }

void CardWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    int w = width(), h = height(), radius = 6;
    int yOffset = m_selected ? -10 : (m_hovered ? -3 : 0);
    QRect cardRect(1, 1, w - 2, h - 2);

    QPainterPath shadowPath;
    shadowPath.addRoundedRect(QRect(3, 3, w - 2, h - 2).adjusted(0, 2 + yOffset, 0, 2 + yOffset), radius, radius);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 50));
    painter.drawPath(shadowPath);

    if (yOffset) { painter.save(); painter.translate(0, yOffset); }
    if (!m_faceUp) drawCardBack(painter, cardRect, radius);
    else drawCardFront(painter, cardRect, radius);
    drawStateOverlay(painter, cardRect, radius);
    if (yOffset) painter.restore();
}

void CardWidget::drawCardBack(QPainter& painter, const QRect& rect, int radius)
{
    QLinearGradient bg(rect.topLeft(), rect.bottomRight());
    bg.setColorAt(0.0, QColor(160, 20, 20)); bg.setColorAt(0.5, QColor(180, 30, 30)); bg.setColorAt(1.0, QColor(140, 15, 15));
    QPainterPath p; p.addRoundedRect(rect, radius, radius);
    painter.setPen(QPen(QColor(120, 10, 10), 1)); painter.setBrush(bg); painter.drawPath(p);
    painter.setPen(QPen(QColor(220, 180, 80, 120), 1));
    int cx = rect.center().x(), cy = rect.center().y();
    painter.drawLine(rect.left() + rect.width()/2, rect.top()+8, rect.right()-8, rect.top()+rect.height()/2);
    painter.drawLine(rect.left()+8, rect.top()+rect.height()/2, rect.left()+rect.width()/2, rect.bottom()-8);
    painter.drawLine(rect.left()+rect.width()/2, rect.top()+8, rect.left()+8, rect.top()+rect.height()/2);
    painter.drawLine(rect.right()-8, rect.top()+rect.height()/2, rect.left()+rect.width()/2, rect.bottom()-8);
    QPainterPath d; d.moveTo(cx, cy-14); d.lineTo(cx+10, cy); d.lineTo(cx, cy+14); d.lineTo(cx-10, cy); d.closeSubpath();
    painter.setPen(QPen(QColor(220, 180, 80), 1)); painter.setBrush(QColor(220, 180, 80, 60)); painter.drawPath(d);
}

void CardWidget::drawCardFront(QPainter& painter, const QRect& rect, int radius)
{
    if (m_data.cardId < 0) {
        QPainterPath p; p.addRoundedRect(rect, radius, radius);
        painter.setPen(QPen(QColor(180,180,180),1)); painter.setBrush(QColor(240,240,240)); painter.drawPath(p);
        return;
    }
    bool isRed = m_data.color == CardColor::Red;
    QColor suitColor = isRed ? QColor(200,30,30) : QColor(30,30,30);
    QPainterPath path; path.addRoundedRect(rect, radius, radius);

    // 装备牌特殊边框
    if (m_data.isEquipment) {
        painter.setPen(QPen(QColor(0,150,136), 2));     // 青绿色边框
        painter.setBrush(isRed ? QColor(255,248,240) : QColor(248,248,240));
    } else {
        painter.setPen(QPen(QColor(140,140,140), 1));
        painter.setBrush(isRed ? QColor(255,245,245) : QColor(248,248,250));
    }
    painter.drawPath(path);

    QRect topBar(rect.left()+1, rect.top()+1, rect.width()-2, 18);
    QLinearGradient bg(topBar.topLeft(), topBar.bottomLeft());
    bg.setColorAt(0.0, isRed ? QColor(200,50,50) : QColor(60,60,60));
    bg.setColorAt(1.0, isRed ? QColor(170,30,30) : QColor(30,30,30));
    QPainterPath bar; bar.addRoundedRect(topBar, 4, 4);
    QPainterPath clip; clip.addRoundedRect(rect.adjusted(1,1,-1,-1), radius-1, radius-1);
    painter.setPen(Qt::NoPen); painter.setBrush(bg); painter.drawPath(bar.intersected(clip));

    QFont nf(QStringLiteral("Microsoft YaHei"), 9, QFont::Bold);
    painter.setFont(nf); painter.setPen(Qt::white);
    painter.drawText(QRect(rect.left()+4, rect.top()+1, 20, 18), Qt::AlignLeft|Qt::AlignVCenter, m_data.numberString);
    QFont sf(QStringLiteral("Segoe UI Symbol"), 9, QFont::Bold);
    painter.setFont(sf);
    painter.drawText(QRect(rect.left()+4+painter.fontMetrics().horizontalAdvance(m_data.numberString)+1, rect.top()+1, 16, 18),
                     Qt::AlignLeft|Qt::AlignVCenter, m_data.suitSymbol);

    QString name = m_data.cardName;
    int sz = name.length() <= 2 ? 14 : (name.length() <= 3 ? 12 : 10);
    QFont nf2(QStringLiteral("Microsoft YaHei"), sz, QFont::Bold);
    painter.setFont(nf2); painter.setPen(suitColor);
    painter.drawText(QRect(rect.left()+2, rect.top()+30, rect.width()-4, rect.height()-50), Qt::AlignCenter|Qt::TextWordWrap, name);

    QFont lsf(QStringLiteral("Segoe UI Symbol"), 32, QFont::Bold);
    painter.setFont(lsf); painter.setPen(QColor(suitColor.red(), suitColor.green(), suitColor.blue(), 35));
    painter.drawText(rect.adjusted(0,0,-4,-8), Qt::AlignBottom|Qt::AlignRight, m_data.suitSymbol);

    QString typeLabel;
    QColor typeColor(120, 120, 120);
    if (m_data.isBasic) {
        typeLabel = QStringLiteral("基本");
    } else if (m_data.isStrategy) {
        typeLabel = QStringLiteral("锦囊");
    } else if (m_data.isEquipment) {
        typeLabel = QStringLiteral("装备");
        typeColor = QColor(0, 150, 136);
        // 装备牌显示槽位图标
        if (m_data.equipSlot == 0)  // EquipSlot::Weapon
            typeLabel = QStringLiteral("⚔ ") + typeLabel;
        else if (m_data.equipSlot == 1)  // EquipSlot::Armor
            typeLabel = QStringLiteral("🛡 ") + typeLabel;
        // 武器显示攻击范围
        if (m_data.equipSlot == 0 && m_data.attackRange > 0)  // EquipSlot::Weapon
            typeLabel += QStringLiteral(" +%1").arg(m_data.attackRange);
    }
    QFont lf(QStringLiteral("Microsoft YaHei"), 7);
    painter.setFont(lf); painter.setPen(typeColor);
    painter.drawText(QRect(rect.left()+3, rect.bottom()-16, rect.width()-6, 14), Qt::AlignLeft|Qt::AlignBottom, typeLabel);
}

void CardWidget::drawStateOverlay(QPainter& painter, const QRect& rect, int radius)
{
    bool effectivePlayable = m_playable || (m_showPlayable && m_data.isPlayable);
    QPainterPath path; path.addRoundedRect(rect, radius, radius);
    if (effectivePlayable) {
        painter.setPen(QPen(QColor(50,180,50), 2)); painter.setBrush(Qt::NoBrush); painter.drawPath(path);
        QRadialGradient g(rect.center(), rect.width()/2);
        g.setColorAt(0.0, QColor(50,200,50,30)); g.setColorAt(1.0, QColor(50,200,50,0));
        painter.setPen(Qt::NoPen); painter.setBrush(g);
        painter.drawRoundedRect(rect.adjusted(-4,-4,4,4), radius+2, radius+2);
    }
    if (m_selected) {
        painter.setPen(QPen(QColor(30,120,220), 3)); painter.setBrush(QColor(30,120,220,18)); painter.drawPath(path);
    }
    if (m_hovered && !m_selected && !effectivePlayable) {
        painter.setPen(Qt::NoPen); painter.setBrush(QColor(255,255,200,30)); painter.drawPath(path);
    }
}
