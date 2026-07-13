#include "CardWidget.h"
#include "CardViewModel.h"
#include "CommonTypes.h"

#include <QPainter>
#include <QMouseEvent>
#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QPainterPath>

CardWidget::CardWidget(CardViewModel* cvm, bool faceUp, QWidget* parent)
    : QFrame(parent), m_cvm(cvm), m_faceUp(faceUp)
{
    setFixedSize(CARD_WIDTH, CARD_HEIGHT);
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);

    if (m_faceUp && m_cvm) {
        updateTooltip();
        m_selConn = connect(m_cvm, &CardViewModel::selectedChanged,
                             this, &CardWidget::onViewModelSelectedChanged);
        m_playableConn = connect(m_cvm, &CardViewModel::playableChanged,
                                  this, &CardWidget::onViewModelPlayableChanged);
    }
}

CardWidget::~CardWidget() = default;

int CardWidget::cardId() const { return m_cvm ? m_cvm->id() : -1; }
bool CardWidget::isSelected() const { return m_selected; }

void CardWidget::setSelected(bool sel)
{
    if (m_selected != sel) { m_selected = sel; update(); }
    if (m_cvm) m_cvm->setSelected(sel);
}

void CardWidget::setPlayable(bool playable)
{
    if (m_playable != playable) { m_playable = playable; update(); }
    if (m_cvm) m_cvm->setPlayable(playable);
}

void CardWidget::setFaceUp(bool faceUp)
{
    if (m_faceUp != faceUp) {
        m_faceUp = faceUp;
        if (m_faceUp && m_cvm) updateTooltip();
        update();
    }
}

void CardWidget::onViewModelSelectedChanged(bool selected)
{
    if (m_selected != selected) { m_selected = selected; update(); }
}

void CardWidget::onViewModelPlayableChanged(bool playable)
{
    if (m_playable != playable) { m_playable = playable; update(); }
}

void CardWidget::updateTooltip()
{
    if (!m_cvm) return;
    QString tip = m_cvm->cardName();
    if (!m_cvm->description().isEmpty())
        tip += QStringLiteral("\n") + m_cvm->description();
    setToolTip(tip);
}

void CardWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_cvm) emit clicked(m_cvm->id());
    QFrame::mousePressEvent(event);
}

void CardWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_cvm) emit doubleClicked(m_cvm->id());
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

    const int w = width(), h = height(), radius = 6;
    int yOffset = m_selected ? -10 : (m_hovered ? -3 : 0);
    QRect cardRect(1, 1, w - 2, h - 2);

    // 阴影
    QPainterPath shadowPath;
    shadowPath.addRoundedRect(QRect(3, 3, w - 2, h - 2).adjusted(0, 2 + yOffset, 0, 2 + yOffset), radius, radius);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 50));
    painter.drawPath(shadowPath);

    if (yOffset != 0) { painter.save(); painter.translate(0, yOffset); }
    if (!m_faceUp) drawCardBack(painter, cardRect, radius);
    else drawCardFront(painter, cardRect, radius);
    drawStateOverlay(painter, cardRect, radius);
    if (yOffset != 0) painter.restore();
}

// ==================== 绘制方法（不变） ====================

void CardWidget::drawCardBack(QPainter& painter, const QRect& rect, int radius)
{
    QLinearGradient bg(rect.topLeft(), rect.bottomRight());
    bg.setColorAt(0.0, QColor(160, 20, 20));
    bg.setColorAt(0.5, QColor(180, 30, 30));
    bg.setColorAt(1.0, QColor(140, 15, 15));
    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);
    painter.setPen(QPen(QColor(120, 10, 10), 1));
    painter.setBrush(bg);
    painter.drawPath(path);
    painter.setPen(QPen(QColor(220, 180, 80, 120), 1));
    int cx = rect.center().x(), cy = rect.center().y();
    painter.drawLine(rect.left() + rect.width() / 2, rect.top() + 8, rect.right() - 8, rect.top() + rect.height() / 2);
    painter.drawLine(rect.left() + 8, rect.top() + rect.height() / 2, rect.left() + rect.width() / 2, rect.bottom() - 8);
    painter.drawLine(rect.left() + rect.width() / 2, rect.top() + 8, rect.left() + 8, rect.top() + rect.height() / 2);
    painter.drawLine(rect.right() - 8, rect.top() + rect.height() / 2, rect.left() + rect.width() / 2, rect.bottom() - 8);
    QPainterPath diamond;
    diamond.moveTo(cx, cy - 14);
    diamond.lineTo(cx + 10, cy);
    diamond.lineTo(cx, cy + 14);
    diamond.lineTo(cx - 10, cy);
    diamond.closeSubpath();
    painter.setPen(QPen(QColor(220, 180, 80), 1));
    painter.setBrush(QColor(220, 180, 80, 60));
    painter.drawPath(diamond);
}

void CardWidget::drawCardFront(QPainter& painter, const QRect& rect, int radius)
{
    if (!m_cvm) {
        QPainterPath path;
        path.addRoundedRect(rect, radius, radius);
        painter.setPen(QPen(QColor(180, 180, 180), 1));
        painter.setBrush(QColor(240, 240, 240));
        painter.drawPath(path);
        return;
    }
    bool isRed = m_cvm->color() == CardColor::Red;
    QColor suitColor = isRed ? QColor(200, 30, 30) : QColor(30, 30, 30);
    QColor lightBg  = isRed ? QColor(255, 245, 245) : QColor(248, 248, 250);

    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);
    painter.setPen(QPen(QColor(140, 140, 140), 1));
    painter.setBrush(lightBg);
    painter.drawPath(path);

    QRect topBar(rect.left() + 1, rect.top() + 1, rect.width() - 2, 18);
    QLinearGradient barGrad(topBar.topLeft(), topBar.bottomLeft());
    barGrad.setColorAt(0.0, isRed ? QColor(200, 50, 50) : QColor(60, 60, 60));
    barGrad.setColorAt(1.0, isRed ? QColor(170, 30, 30) : QColor(30, 30, 30));
    QPainterPath barPath;
    barPath.addRoundedRect(topBar, 4, 4);
    QPainterPath clipPath;
    clipPath.addRoundedRect(rect.adjusted(1, 1, -1, -1), radius - 1, radius - 1);
    painter.setPen(Qt::NoPen);
    painter.setBrush(barGrad);
    painter.drawPath(barPath.intersected(clipPath));

    QString numStr = m_cvm->numberString();
    QString suitStr = m_cvm->suitSymbol();
    QFont numFont(QStringLiteral("Microsoft YaHei"), 9, QFont::Bold);
    painter.setFont(numFont);
    painter.setPen(Qt::white);
    painter.drawText(QRect(rect.left() + 4, rect.top() + 1, 20, 18), Qt::AlignLeft | Qt::AlignVCenter, numStr);
    QFont suitFont(QStringLiteral("Segoe UI Symbol"), 9, QFont::Bold);
    painter.setFont(suitFont);
    painter.drawText(QRect(rect.left() + 4 + painter.fontMetrics().horizontalAdvance(numStr) + 1, rect.top() + 1, 16, 18),
                     Qt::AlignLeft | Qt::AlignVCenter, suitStr);

    QString name = m_cvm->cardName();
    int nameFontSize = name.length() <= 2 ? 14 : (name.length() <= 3 ? 12 : 10);
    QFont nameFont(QStringLiteral("Microsoft YaHei"), nameFontSize, QFont::Bold);
    painter.setFont(nameFont);
    painter.setPen(suitColor);
    painter.drawText(QRect(rect.left() + 2, rect.top() + 30, rect.width() - 4, rect.height() - 50),
                     Qt::AlignCenter | Qt::TextWordWrap, name);

    QFont largeSuitFont(QStringLiteral("Segoe UI Symbol"), 32, QFont::Bold);
    painter.setFont(largeSuitFont);
    painter.setPen(QColor(suitColor.red(), suitColor.green(), suitColor.blue(), 35));
    painter.drawText(rect.adjusted(0, 0, -4, -8), Qt::AlignBottom | Qt::AlignRight, suitStr);

    QString typeLabel;
    if (m_cvm->isBasic()) typeLabel = QStringLiteral("基本");
    else if (m_cvm->isStrategy()) typeLabel = QStringLiteral("锦囊");
    else typeLabel = QStringLiteral("装备");
    QFont labelFont(QStringLiteral("Microsoft YaHei"), 7);
    painter.setFont(labelFont);
    painter.setPen(QColor(120, 120, 120));
    painter.drawText(QRect(rect.left() + 3, rect.bottom() - 16, rect.width() - 6, 14),
                     Qt::AlignLeft | Qt::AlignBottom, typeLabel);
}

void CardWidget::drawStateOverlay(QPainter& painter, const QRect& rect, int radius)
{
    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);
    if (m_playable) {
        painter.setPen(QPen(QColor(50, 180, 50), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
        QRadialGradient glow(rect.center(), rect.width() / 2);
        glow.setColorAt(0.0, QColor(50, 200, 50, 30));
        glow.setColorAt(1.0, QColor(50, 200, 50, 0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(glow);
        painter.drawRoundedRect(rect.adjusted(-4, -4, 4, 4), radius + 2, radius + 2);
    }
    if (m_selected) {
        painter.setPen(QPen(QColor(30, 120, 220), 3));
        painter.setBrush(QColor(30, 120, 220, 18));
        painter.drawPath(path);
    }
    if (m_hovered && !m_selected && !m_playable) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 200, 30));
        painter.drawPath(path);
    }
}
