#ifndef CARDWIDGET_H
#define CARDWIDGET_H

#include <QFrame>
#include "CardData.h"

class QVariantAnimation;

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

    /// 扇形绘制模式：控件外扩 2*FAN_MARGIN 容纳旋转/上浮溢出，牌面居中绘制并绕中心旋转
    /// rotationDeg：牌面旋转角（度，正=顺时针）；baseLift：该牌在扇形中的基线抬升（px，向上为正）
    /// 仅改变"绘制"，控件几何仍是正立矩形，点击命中不受影响。
    void setFanMode(bool enabled, double rotationDeg = 0.0, int baseLift = 0);
    bool fanMode() const { return m_fanMode; }

    /// 入场淡入：从 0 过渡到 1（错峰由调用方通过 delayMs 控制）
    void playEntranceFade(int delayMs = 0);

    static constexpr int CARD_WIDTH  = 80;
    static constexpr int CARD_HEIGHT = 112;
    static constexpr int FAN_MARGIN  = 10;   // 扇形模式四周外扩，容纳旋转四角与上浮溢出

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
    QRect cardFaceRect() const;      // 牌面绘制矩形（扇形模式居中，否则填满）
    int targetLift() const;          // 当前状态目标上浮量（选中/悬停/基线）
    void animateLift();              // 平滑过渡到 targetLift()

    CardData m_data;
    bool m_faceUp = true;
    bool m_selected = false;
    bool m_playable = false;
    bool m_hovered = false;
    bool m_showPlayable = false;

    bool m_fanMode = false;
    double m_rotationDeg = 0.0;
    int m_baseLift = 0;
    double m_liftAnim = 0.0;         // 动画中的当前上浮量（px）
    double m_opacity = 1.0;          // 入场淡入透明度
    QVariantAnimation* m_liftAnimation = nullptr;
    QVariantAnimation* m_fadeAnimation = nullptr;
};

#endif // CARDWIDGET_H
