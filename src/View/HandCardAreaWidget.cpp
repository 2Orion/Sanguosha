#include "HandCardAreaWidget.h"
#include "CardWidget.h"
#include <QResizeEvent>
#include <algorithm>
#include <cmath>

HandCardAreaWidget::HandCardAreaWidget(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(CARD_HEIGHT + TOP_PADDING);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void HandCardAreaWidget::setCards(const CardList& cards, bool faceUp)
{
    m_cards = cards;
    m_faceUp = faceUp;
    rebuildWidgets();
}

void HandCardAreaWidget::setSelection(int cardId, bool selected)
{
    for (auto* w : m_cardWidgets)
        w->setSelected(w->cardId() == cardId && selected);
}

int HandCardAreaWidget::selectedCardId() const
{
    for (auto* w : m_cardWidgets)
        if (w->isSelected()) return w->cardId();
    return -1;
}

QVector<int> HandCardAreaWidget::selectedCardIds() const
{
    QVector<int> ids;
    for (auto* w : m_cardWidgets)
        if (w->isSelected()) ids.append(w->cardId());
    return ids;
}

void HandCardAreaWidget::clearSelection()
{
    for (auto* w : m_cardWidgets) w->setSelected(false);
}

void HandCardAreaWidget::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    arrangeCards();
}

void HandCardAreaWidget::onCardWidgetClicked(int cardId)
{
    if (m_multiSelect) {
        // 多选模式：切换选中状态
        for (auto* w : m_cardWidgets) {
            if (w->cardId() == cardId) w->setSelected(!w->isSelected());
        }
    } else {
        // 单选模式：选中当前点击的卡片，取消其他选中
        for (auto* w : m_cardWidgets) {
            w->setSelected(w->cardId() == cardId);
        }
    }
    emit cardClicked(cardId);
}

void HandCardAreaWidget::onCardWidgetDoubleClicked(int cardId)
{
    emit cardDoubleClicked(cardId);
}

void HandCardAreaWidget::rebuildWidgets()
{
    clearWidgets();
    int idx = 0;
    for (const auto& cd : m_cards) {
        auto* cw = new CardWidget(m_faceUp, this);
        cw->setDisplayData(cd);
        cw->setVisible(true);
        connect(cw, &CardWidget::clicked, this, &HandCardAreaWidget::onCardWidgetClicked);
        connect(cw, &CardWidget::doubleClicked, this, &HandCardAreaWidget::onCardWidgetDoubleClicked);
        m_cardWidgets.push_back(cw);
        ++idx;
    }
    arrangeCards();
    // 入场淡入：几何已就位（不影响命中/点击），仅绘制层错峰淡入
    for (size_t i = 0; i < m_cardWidgets.size(); ++i)
        m_cardWidgets[i]->playEntranceFade(static_cast<int>(i) * 40);
    updateGeometry();
}

void HandCardAreaWidget::clearWidgets()
{
    // 使用 deleteLater 而非 delete：防止在 CardWidget 的事件处理（如点击→出牌→刷新手牌）
    // 调用栈中删除正在处理事件的自身对象，导致 use-after-free。
    for (auto* w : m_cardWidgets) {
        disconnect(w, nullptr, this, nullptr);
        w->deleteLater();
    }
    m_cardWidgets.clear();
}

void HandCardAreaWidget::arrangeCards()
{
    if (m_cardWidgets.empty()) return;
    int w = width();
    int n = static_cast<int>(m_cardWidgets.size());

    // ≤ FAN_MAX 张：弧形扇形排列（下沉 + 旋转）；否则退回水平重叠。
    if (n <= FAN_MAX) {
        // 卡片牌面中心间距：接近正常摆放但略收拢，营造扇形；受可用宽度约束。
        int faceSpacing = CARD_WIDTH - CARD_OVERLAP + 6;   // 牌面中心步进
        int totalFace = CARD_WIDTH + (n - 1) * faceSpacing;
        if (totalFace > w && n > 1)
            faceSpacing = std::max(MIN_SPACING, (w - CARD_WIDTH) / (n - 1));
        int usedFace = CARD_WIDTH + (n - 1) * faceSpacing;
        int startFaceX = std::max(0, (w - usedFace) / 2);

        // 旋转角对称分布：跨度 ±((n-1)/2)*FAN_STEP，封顶 FAN_MAX_TILT
        double stepDeg = (n > 1) ? std::min<double>(FAN_STEP, (2.0 * FAN_MAX_TILT) / (n - 1)) : 0.0;
        double mid = (n - 1) / 2.0;

        for (int i = 0; i < n; ++i) {
            double angle = (i - mid) * stepDeg;
            // 弧形下沉：中间最高、两端下沉（抛物线），幅度控制在 margin 内
            double t = (n > 1) ? (i - mid) / mid : 0.0;   // [-1,1]
            int dip = static_cast<int>(FAN_DIP * t * t);   // >=0，两端更大
            m_cardWidgets[i]->setFanMode(true, angle, /*baseLift=*/-dip / 2);
            // 控件外扩 FAN_MARGIN：几何左上角 = 期望牌面左上角 - margin
            int faceX = startFaceX + i * faceSpacing;
            int geoX = faceX - CardWidget::FAN_MARGIN;
            int geoY = TOP_PADDING - CardWidget::FAN_MARGIN + dip;
            m_cardWidgets[i]->setGeometry(geoX, geoY,
                                          CARD_WIDTH + 2 * CardWidget::FAN_MARGIN,
                                          CARD_HEIGHT + 2 * CardWidget::FAN_MARGIN);
        }
        return;
    }

    // > FAN_MAX 张：水平重叠（无旋转外扩）
    int totalWidth = CARD_WIDTH + (n - 1) * (CARD_WIDTH - CARD_OVERLAP);
    int spacing = CARD_WIDTH - CARD_OVERLAP;
    if (totalWidth > w && n > 1)
        spacing = std::max(MIN_SPACING, (w - CARD_WIDTH) / (n - 1));
    int used = CARD_WIDTH + (n - 1) * spacing;
    int startX = std::max(0, (w - used) / 2);
    for (int i = 0; i < n; ++i) {
        m_cardWidgets[i]->setFanMode(false);
        m_cardWidgets[i]->setGeometry(startX + i * spacing, TOP_PADDING, CARD_WIDTH, CARD_HEIGHT);
    }
}

CardWidget* HandCardAreaWidget::findWidgetByCardId(int id) const
{
    for (auto* w : m_cardWidgets)
        if (w->cardId() == id) return w;
    return nullptr;
}
