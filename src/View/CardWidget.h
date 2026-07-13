#ifndef CARDWIDGET_H
#define CARDWIDGET_H

#include <QFrame>
#include <vector>

class CardViewModel;

/// 单张卡牌控件 — 支持选中、可打出高亮、悬停、牌背显示
class CardWidget : public QFrame {
    Q_OBJECT
public:
    explicit CardWidget(CardViewModel* cvm, bool faceUp = true, QWidget* parent = nullptr);
    ~CardWidget() override;

    int cardId() const;
    bool isSelected() const;
    void setSelected(bool sel);
    void setPlayable(bool playable);
    void setFaceUp(bool faceUp);

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
    void connectViewModel();
    void disconnectViewModel();
    void updateTooltip();

    // 绘制方法
    void drawCardBack(QPainter& painter, const QRect& rect, int radius);
    void drawCardFront(QPainter& painter, const QRect& rect, int radius);
    void drawStateOverlay(QPainter& painter, const QRect& rect, int radius);

    CardViewModel* m_cvm;

    // 连接 ID，用于析构时断开（按事件分别存储，避免跨事件 ID 冲突）
    size_t m_selectedChangedId = 0;
    size_t m_playableChangedId = 0;

    bool m_faceUp;
    bool m_selected  = false;
    bool m_playable  = false;
    bool m_hovered   = false;
};

#endif // CARDWIDGET_H
