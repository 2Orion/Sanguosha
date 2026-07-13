#ifndef CARDWIDGET_H
#define CARDWIDGET_H

#include <QFrame>
#include <QMetaObject>

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

private slots:
    void onViewModelSelectedChanged(bool selected);
    void onViewModelPlayableChanged(bool playable);

private:
    void updateTooltip();

    void drawCardBack(QPainter& painter, const QRect& rect, int radius);
    void drawCardFront(QPainter& painter, const QRect& rect, int radius);
    void drawStateOverlay(QPainter& painter, const QRect& rect, int radius);

    CardViewModel* m_cvm;
    QMetaObject::Connection m_selConn;
    QMetaObject::Connection m_playableConn;

    bool m_faceUp;
    bool m_selected  = false;
    bool m_playable  = false;
    bool m_hovered   = false;
};

#endif // CARDWIDGET_H
