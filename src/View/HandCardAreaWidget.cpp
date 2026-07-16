#include "HandCardAreaWidget.h"
#include "CardWidget.h"
#include <QResizeEvent>
#include <algorithm>

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
    for (const auto& cd : m_cards) {
        auto* cw = new CardWidget(m_faceUp, this);
        cw->setDisplayData(cd);
        cw->setVisible(true);
        connect(cw, &CardWidget::clicked, this, &HandCardAreaWidget::onCardWidgetClicked);
        connect(cw, &CardWidget::doubleClicked, this, &HandCardAreaWidget::onCardWidgetDoubleClicked);
        m_cardWidgets.push_back(cw);
    }
    arrangeCards();
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
    int totalWidth = CARD_WIDTH + (n - 1) * (CARD_WIDTH - CARD_OVERLAP);
    int spacing = CARD_WIDTH - CARD_OVERLAP;
    if (totalWidth > w && n > 1)
        spacing = std::max(MIN_SPACING, (w - CARD_WIDTH) / (n - 1));
    int used = CARD_WIDTH + (n - 1) * spacing;
    int startX = std::max(0, (w - used) / 2);
    for (int i = 0; i < n; ++i)
        m_cardWidgets[i]->setGeometry(startX + i * spacing, TOP_PADDING, CARD_WIDTH, CARD_HEIGHT);
}

CardWidget* HandCardAreaWidget::findWidgetByCardId(int id) const
{
    for (auto* w : m_cardWidgets)
        if (w->cardId() == id) return w;
    return nullptr;
}
