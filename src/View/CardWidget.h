#ifndef CARDWIDGET_H
#define CARDWIDGET_H

#include <QFrame>
#include "CardData.h"

/// 单张卡牌控件 — 只依赖 Common 层值类型
class CardWidget : public QFrame {
    Q_OBJECT
public:
    explicit CardWidget(bool faceUp = true, QWidget* parent = nullptr);

    void setDisplayData(const CardData& data);
    void setFaceUp(bool faceUp, bool showPlayable = false);

    int cardId() const { return m_data.cardId; }
    bool isSelected() const { return m_selected; }
    void setSelected(bool sel);
    void setPlayable(bool playable);

    static constexpr int CARD_WIDTH  = 80;
    static constexpr int CARD_HEIGHT = 112;

signals:
    void clicked(int cardId);
    void doubleClicked(int cardId);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent* event) override;
#else
    void enterEvent(QEvent* event) override;
#endif
    void leaveEvent(QEvent* event) override;

private:
    void drawCardBack(QPainter& painter, const QRect& rect, int radius);
    void drawCardFront(QPainter& painter, const QRect& rect, int radius);
    void drawStateOverlay(QPainter& painter, const QRect& rect, int radius);

    CardData m_data;
    bool m_faceUp = true;
    bool m_selected = false;
    bool m_playable = false;
    bool m_hovered = false;
    bool m_showPlayable = false;
};

#endif // CARDWIDGET_H
