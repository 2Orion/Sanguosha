#include "HandCardAreaWidget.h"
#include "CardWidget.h"
#include "CardViewModel.h"
#include "Card.h"

#include <QResizeEvent>
#include <algorithm>

// ==================== 构造 / 析构 ====================

HandCardAreaWidget::HandCardAreaWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(CARD_HEIGHT + TOP_PADDING);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

HandCardAreaWidget::~HandCardAreaWidget()
{
    clearWidgets();
}

// ==================== 公开接口 ====================

void HandCardAreaWidget::setCards(std::vector<std::unique_ptr<CardViewModel>> cards,
                                   bool isOpponent)
{
    m_isOpponent = isOpponent;

    // 必须先销毁旧控件（它们持有指向旧 CardViewModel 的指针）
    clearWidgets();

    // 转移所有权（旧 CardViewModel 在此步析构，但旧控件已删除，不会有悬垂访问）
    m_ownedCardVMs = std::move(cards);

    // 重建原始指针列表
    m_cardViewModels.clear();
    for (const auto& cvm : m_ownedCardVMs) {
        m_cardViewModels.push_back(cvm.get());
    }

    // 创建新控件
    rebuildWidgets();
}

void HandCardAreaWidget::refreshPlayableState()
{
    for (auto* cw : m_cardWidgets) {
        if (auto* cvm = findWidgetByCardId(cw->cardId())) {
            // 用 CardWidget 的原始指针反查 CVM 比较麻烦，遍历
        }
    }
    // 更简单的做法：遍历 CardWidget，从对应的 CardViewModel 同步 playable 状态
    for (size_t i = 0; i < m_cardWidgets.size() && i < m_cardViewModels.size(); ++i) {
        m_cardWidgets[i]->setPlayable(m_cardViewModels[i]->isPlayable());
    }
}

Card* HandCardAreaWidget::selectedCard() const
{
    for (size_t i = 0; i < m_cardWidgets.size() && i < m_cardViewModels.size(); ++i) {
        if (m_cardWidgets[i]->isSelected()) {
            return m_cardViewModels[i]->card();
        }
    }
    return nullptr;
}

void HandCardAreaWidget::clearSelection()
{
    for (auto* cw : m_cardWidgets) {
        cw->setSelected(false);
    }
}

// ==================== 事件处理 ====================

void HandCardAreaWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    arrangeCards();
}

void HandCardAreaWidget::onCardWidgetClicked(int cardId)
{
    // 切换选中状态
    for (size_t i = 0; i < m_cardWidgets.size() && i < m_cardViewModels.size(); ++i) {
        if (m_cardWidgets[i]->cardId() == cardId) {
            bool newlySelected = !m_cardWidgets[i]->isSelected();
            // 单选：取消其他选中
            if (newlySelected) {
                for (auto* cw : m_cardWidgets) {
                    if (cw->cardId() != cardId) {
                        cw->setSelected(false);
                    }
                }
            }
            m_cardWidgets[i]->setSelected(newlySelected);
            emit selectionChanged(m_cardViewModels[i]->card());
            emit cardClicked(m_cardViewModels[i]->card());
            break;
        }
    }
}

void HandCardAreaWidget::onCardWidgetDoubleClicked(int cardId)
{
    for (size_t i = 0; i < m_cardWidgets.size() && i < m_cardViewModels.size(); ++i) {
        if (m_cardWidgets[i]->cardId() == cardId) {
            emit cardDoubleClicked(m_cardViewModels[i]->card());
            break;
        }
    }
}

// ==================== 内部方法 ====================

void HandCardAreaWidget::rebuildWidgets()
{
    clearWidgets();

    // 为每个 CardViewModel 创建 CardWidget
    for (auto* cvm : m_cardViewModels) {
        CardWidget* cw = new CardWidget(cvm, !m_isOpponent, this);
        cw->setVisible(true);

        // 连接信号
        connect(cw, &CardWidget::clicked, this, &HandCardAreaWidget::onCardWidgetClicked);
        connect(cw, &CardWidget::doubleClicked, this, &HandCardAreaWidget::onCardWidgetDoubleClicked);

        m_cardWidgets.push_back(cw);
    }

    arrangeCards();
    updateGeometry();
}

void HandCardAreaWidget::clearWidgets()
{
    // 立即删除旧控件（不同于 deleteLater，防止旧控件残留的绘制访问已销毁的 CardViewModel）
    for (auto* cw : m_cardWidgets) {
        disconnect(cw, nullptr, this, nullptr);
        cw->setParent(nullptr);
        delete cw;
    }
    m_cardWidgets.clear();
}

void HandCardAreaWidget::arrangeCards()
{
    if (m_cardWidgets.empty()) return;

    const int widgetWidth = width();
    const int count = static_cast<int>(m_cardWidgets.size());

    // 计算需要的总宽度
    int totalWidth = CARD_WIDTH + (count - 1) * (CARD_WIDTH - CARD_OVERLAP);
    int spacing = CARD_WIDTH - CARD_OVERLAP; // 默认间距 = 60

    // 如果超出可用宽度，压缩间距
    if (totalWidth > widgetWidth && count > 1) {
        spacing = std::max(MIN_SPACING,
                           (widgetWidth - CARD_WIDTH) / (count - 1));
    }

    // 居中偏移
    int usedWidth = CARD_WIDTH + (count - 1) * spacing;
    int startX = std::max(0, (widgetWidth - usedWidth) / 2);

    // 定位每张卡牌
    for (int i = 0; i < count; ++i) {
        int x = startX + i * spacing;
        m_cardWidgets[i]->setGeometry(x, TOP_PADDING, CARD_WIDTH, CARD_HEIGHT);
    }
}

CardWidget* HandCardAreaWidget::findWidgetByCardId(int cardId) const
{
    for (auto* cw : m_cardWidgets) {
        if (cw->cardId() == cardId) return cw;
    }
    return nullptr;
}
