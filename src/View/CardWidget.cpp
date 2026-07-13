#include "CardWidget.h"
#include "CardViewModel.h"
#include "Core/CommonTypes.h"

#include <QPainter>
#include <QMouseEvent>
#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QPainterPath>

// ==================== 构造 / 析构 ====================

CardWidget::CardWidget(CardViewModel* cvm, bool faceUp, QWidget* parent)
    : QFrame(parent)
    , m_cvm(cvm)
    , m_faceUp(faceUp)
{
    setFixedSize(CARD_WIDTH, CARD_HEIGHT);
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);

    if (m_faceUp && m_cvm) {
        updateTooltip();
        connectViewModel();
    }
}

CardWidget::~CardWidget()
{
    disconnectViewModel();
}

// ==================== 公开接口 ====================

int CardWidget::cardId() const
{
    return m_cvm ? m_cvm->id() : -1;
}

bool CardWidget::isSelected() const
{
    return m_selected;
}

void CardWidget::setSelected(bool sel)
{
    if (m_selected != sel) {
        m_selected = sel;
        update();
    }
    if (m_cvm) {
        m_cvm->setSelected(sel);
    }
}

void CardWidget::setPlayable(bool playable)
{
    if (m_playable != playable) {
        m_playable = playable;
        update();
    }
    if (m_cvm) {
        m_cvm->setPlayable(playable);
    }
}

void CardWidget::setFaceUp(bool faceUp)
{
    if (m_faceUp != faceUp) {
        m_faceUp = faceUp;
        if (m_faceUp && m_cvm) {
            updateTooltip();
            connectViewModel();
        }
        update();
    }
}

// ==================== ViewModel 事件桥接 ====================

void CardWidget::connectViewModel()
{
    if (!m_cvm) return;
    disconnectViewModel();

    m_selectedChangedId = m_cvm->selectedChanged.connect([this](bool sel) {
        QMetaObject::invokeMethod(this, [this, sel]() {
            if (m_selected != sel) {
                m_selected = sel;
                update();
            }
        }, Qt::QueuedConnection);
    });

    m_playableChangedId = m_cvm->playableChanged.connect([this](bool playable) {
        QMetaObject::invokeMethod(this, [this, playable]() {
            if (m_playable != playable) {
                m_playable = playable;
                update();
            }
        }, Qt::QueuedConnection);
    });
}

void CardWidget::disconnectViewModel()
{
    if (!m_cvm) return;
    m_cvm->selectedChanged.disconnect(m_selectedChangedId);
    m_cvm->playableChanged.disconnect(m_playableChangedId);
    m_selectedChangedId = 0;
    m_playableChangedId = 0;
}

void CardWidget::updateTooltip()
{
    if (!m_cvm) return;
    QString tip = QString::fromStdString(m_cvm->cardName());
    if (!m_cvm->description().empty()) {
        tip += "\n" + QString::fromStdString(m_cvm->description());
    }
    setToolTip(tip);
}

// ==================== 事件处理 ====================

void CardWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_cvm) {
            emit clicked(m_cvm->id());
        }
    }
    QFrame::mousePressEvent(event);
}

void CardWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_cvm) {
            emit doubleClicked(m_cvm->id());
        }
    }
    QFrame::mouseDoubleClickEvent(event);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void CardWidget::enterEvent(QEnterEvent* /*event*/)
#else
void CardWidget::enterEvent(QEvent* /*event*/)
#endif
{
    m_hovered = true;
    update();
}

void CardWidget::leaveEvent(QEvent* /*event*/)
{
    m_hovered = false;
    update();
}

// ==================== 绘制 ====================

void CardWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    const int w = width();
    const int h = height();
    const int radius = 6;

    // 状态偏移：选中上移 10px，悬停上移 3px
    int yOffset = 0;
    if (m_selected)
        yOffset = -10;
    else if (m_hovered)
        yOffset = -3;

    // 绘制区域
    QRect cardRect(1, 1, w - 2, h - 2);

    // ====== 阴影 ======
    {
        QRect shadowRect(3, 3 + (yOffset < 0 ? 0 : 0), w - 2, h - 2);
        QPainterPath shadowPath;
        shadowPath.addRoundedRect(shadowRect.adjusted(0, 2 + yOffset, 0, 2 + yOffset), radius, radius);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 50));
        painter.drawPath(shadowPath);
    }

    // 应用偏移
    if (yOffset != 0) {
        painter.save();
        painter.translate(0, yOffset);
    }

    // ====== 卡牌主体 ======
    if (!m_faceUp) {
        drawCardBack(painter, cardRect, radius);
    } else {
        drawCardFront(painter, cardRect, radius);
    }

    // ====== 状态覆盖层（选中 / 可打出） ======
    drawStateOverlay(painter, cardRect, radius);

    if (yOffset != 0) {
        painter.restore();
    }
}

// ==================== 牌背绘制 ====================

void CardWidget::drawCardBack(QPainter& painter, const QRect& rect, int radius)
{
    // 深红色渐变背景
    QLinearGradient bg(rect.topLeft(), rect.bottomRight());
    bg.setColorAt(0.0, QColor(160, 20, 20));
    bg.setColorAt(0.5, QColor(180, 30, 30));
    bg.setColorAt(1.0, QColor(140, 15, 15));

    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);
    painter.setPen(QPen(QColor(120, 10, 10), 1));
    painter.setBrush(bg);
    painter.drawPath(path);

    // 菱形花纹
    painter.setPen(QPen(QColor(220, 180, 80, 120), 1));
    painter.drawLine(rect.left() + rect.width() / 2, rect.top() + 8,
                     rect.right() - 8, rect.top() + rect.height() / 2);
    painter.drawLine(rect.left() + 8, rect.top() + rect.height() / 2,
                     rect.left() + rect.width() / 2, rect.bottom() - 8);
    painter.drawLine(rect.left() + rect.width() / 2, rect.top() + 8,
                     rect.left() + 8, rect.top() + rect.height() / 2);
    painter.drawLine(rect.right() - 8, rect.top() + rect.height() / 2,
                     rect.left() + rect.width() / 2, rect.bottom() - 8);

    // 中心菱形
    QPainterPath diamond;
    int cx = rect.center().x();
    int cy = rect.center().y();
    diamond.moveTo(cx, cy - 14);
    diamond.lineTo(cx + 10, cy);
    diamond.lineTo(cx, cy + 14);
    diamond.lineTo(cx - 10, cy);
    diamond.closeSubpath();
    painter.setPen(QPen(QColor(220, 180, 80), 1));
    painter.setBrush(QColor(220, 180, 80, 60));
    painter.drawPath(diamond);
}

// ==================== 牌面绘制 ====================

void CardWidget::drawCardFront(QPainter& painter, const QRect& rect, int radius)
{
    if (!m_cvm) {
        // 空数据：灰色占位
        QPainterPath path;
        path.addRoundedRect(rect, radius, radius);
        painter.setPen(QPen(QColor(180, 180, 180), 1));
        painter.setBrush(QColor(240, 240, 240));
        painter.drawPath(path);
        return;
    }

    // 判断花色颜色
    bool isRed = m_cvm->color() == CardColor::Red;
    QColor suitColor = isRed ? QColor(200, 30, 30) : QColor(30, 30, 30);
    QColor lightBg  = isRed ? QColor(255, 245, 245) : QColor(248, 248, 250);

    // ====== 卡牌背景 ======
    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);
    painter.setPen(QPen(QColor(140, 140, 140), 1));
    painter.setBrush(lightBg);
    painter.drawPath(path);

    // ====== 顶部花色色条 ======
    QRect topBar(rect.left() + 1, rect.top() + 1, rect.width() - 2, 18);
    QPainterPath barPath;
    barPath.addRoundedRect(topBar, 4, 4);
    // 只保留左上角和右上角的圆角
    QPainterPath clipPath;
    clipPath.addRoundedRect(rect.adjusted(1, 1, -1, -1), radius - 1, radius - 1);
    QPainterPath topBarClipped = barPath.intersected(clipPath);

    QLinearGradient barGrad(topBar.topLeft(), topBar.bottomLeft());
    if (isRed) {
        barGrad.setColorAt(0.0, QColor(200, 50, 50));
        barGrad.setColorAt(1.0, QColor(170, 30, 30));
    } else {
        barGrad.setColorAt(0.0, QColor(60, 60, 60));
        barGrad.setColorAt(1.0, QColor(30, 30, 30));
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(barGrad);
    painter.drawPath(topBarClipped);

    // ====== 左上角：点数 + 花色符号 ======
    QString numStr = QString::fromStdString(m_cvm->numberString());
    QString suitStr = QString::fromStdString(m_cvm->suitSymbol());

    // 点数（在色条上白色文字）
    QFont numFont("Microsoft YaHei", 9, QFont::Bold);
    painter.setFont(numFont);
    painter.setPen(Qt::white);
    QRect numRect(rect.left() + 4, rect.top() + 1, 20, 18);
    painter.drawText(numRect, Qt::AlignLeft | Qt::AlignVCenter, numStr);

    // 花色符号（点数右侧）
    QFont suitFont("Segoe UI Symbol", 9, QFont::Bold);
    painter.setFont(suitFont);
    QRect suitRect(rect.left() + 4 + painter.fontMetrics().horizontalAdvance(numStr) + 1,
                   rect.top() + 1, 16, 18);
    painter.drawText(suitRect, Qt::AlignLeft | Qt::AlignVCenter, suitStr);

    // ====== 中央：卡牌名称（中文） ======
    QString name = QString::fromStdString(m_cvm->cardName());

    // 根据名称长度选择合适的字号
    int nameFontSize;
    if (name.length() <= 2)
        nameFontSize = 14;
    else if (name.length() <= 3)
        nameFontSize = 12;
    else
        nameFontSize = 10;

    QFont nameFont("Microsoft YaHei", nameFontSize, QFont::Bold);
    painter.setFont(nameFont);
    painter.setPen(suitColor);
    QRect nameRect(rect.left() + 2, rect.top() + 30, rect.width() - 4, rect.height() - 50);
    painter.drawText(nameRect, Qt::AlignCenter | Qt::TextWordWrap, name);

    // ====== 右下角：小号花色符号 ======
    painter.setFont(QFont("Segoe UI Symbol", 16, QFont::Bold));
    painter.setPen(QColor(suitColor.red(), suitColor.green(), suitColor.blue(), 60));
    // 半透明的大号花色水印

    QFont largeSuitFont("Segoe UI Symbol", 32, QFont::Bold);
    painter.setFont(largeSuitFont);
    painter.setPen(QColor(suitColor.red(), suitColor.green(), suitColor.blue(), 35));
    painter.drawText(rect.adjusted(0, 0, -4, -8),
                     Qt::AlignBottom | Qt::AlignRight, suitStr);

    // ====== 左下：卡牌分类小标签 ======
    QString typeLabel;
    if (m_cvm->isBasic())
        typeLabel = QStringLiteral("基本");
    else if (m_cvm->isStrategy())
        typeLabel = QStringLiteral("锦囊");
    else
        typeLabel = QStringLiteral("装备");

    QFont labelFont("Microsoft YaHei", 7);
    painter.setFont(labelFont);
    painter.setPen(QColor(120, 120, 120));
    QRect labelRect(rect.left() + 3, rect.bottom() - 16, rect.width() - 6, 14);
    painter.drawText(labelRect, Qt::AlignLeft | Qt::AlignBottom, typeLabel);
}

// ==================== 状态覆盖层 ====================

void CardWidget::drawStateOverlay(QPainter& painter, const QRect& rect, int radius)
{
    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);

    // 可打出态：淡绿色边框
    if (m_playable) {
        QPen pen(QColor(50, 180, 50), 2);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);

        // 发光效果（内发光绿色光晕）
        QRadialGradient glow(rect.center(), rect.width() / 2);
        glow.setColorAt(0.0, QColor(50, 200, 50, 30));
        glow.setColorAt(1.0, QColor(50, 200, 50, 0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(glow);
        painter.drawRoundedRect(rect.adjusted(-4, -4, 4, 4), radius + 2, radius + 2);
    }

    // 选中态：蓝色边框 + 淡蓝覆盖
    if (m_selected) {
        // 外发光蓝色边框（3px）
        QPen pen(QColor(30, 120, 220), 3);
        painter.setPen(pen);
        painter.setBrush(QColor(30, 120, 220, 18));
        painter.drawPath(path);
    }

    // 悬停态：淡黄色覆盖
    if (m_hovered && !m_selected && !m_playable) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 200, 30));
        painter.drawPath(path);
    }
}
