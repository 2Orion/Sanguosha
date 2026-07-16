#ifndef HANDCAREAWIDGET_H
#define HANDCAREAWIDGET_H

#include <QWidget>
#include <QVector>
#include "CardData.h"

class CardWidget;

/// 手牌区域 — 只依赖 Common 层 CardData
class HandCardAreaWidget : public QWidget {
    Q_OBJECT
public:
    explicit HandCardAreaWidget(QWidget* parent = nullptr);

    void setCards(const CardList& cards, bool faceUp = true);
    void setSelection(int cardId, bool selected);
    void setMultiSelectMode(bool multi) { m_multiSelect = multi; }

    int selectedCardId() const;
    QVector<int> selectedCardIds() const;
    void clearSelection();
    int cardCount() const { return m_cards.size(); }

signals:
    void cardClicked(int cardId);
    void cardDoubleClicked(int cardId);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onCardWidgetClicked(int cardId);
    void onCardWidgetDoubleClicked(int cardId);

private:
    void arrangeCards();
    void rebuildWidgets();
    void clearWidgets();
    CardWidget* findWidgetByCardId(int id) const;

    CardList m_cards;
    bool m_faceUp = false;
    bool m_multiSelect = false;
    std::vector<CardWidget*> m_cardWidgets;

    static constexpr int CARD_WIDTH  = 80;
    static constexpr int CARD_HEIGHT = 112;
    static constexpr int CARD_OVERLAP = 20;
    static constexpr int MIN_SPACING = 4;
    static constexpr int TOP_PADDING = 8;

    static constexpr int    FAN_MAX      = 6;    // ≤ 此数量走扇形排列
    static constexpr double FAN_STEP     = 3.0;  // 相邻卡片旋转角步进（度）
    static constexpr double FAN_MAX_TILT = 9.0;  // 最外侧卡片旋转角封顶（度）
    static constexpr int    FAN_DIP      = 12;   // 两端相对中心的弧形下沉幅度（px，控制在 margin 内）
};

#endif // HANDCAREAWIDGET_H
