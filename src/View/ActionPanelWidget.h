#ifndef ACTIONPANELWIDGET_H
#define ACTIONPANELWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include "Common/CommonTypes.h"

struct PendingActionVM;

/// 操作按钮面板 — 根据当前游戏阶段显示对应的操作按钮
class ActionPanelWidget : public QWidget {
    Q_OBJECT
public:
    explicit ActionPanelWidget(QWidget* parent = nullptr);

    /// 根据当前游戏阶段更新按钮显示
    void updateForPhase(PhaseType phase, bool hasPendingAction);

    /// 有待定响应时更新显示
    void updateForPendingAction(const PendingActionVM& info);

    /// 设置提示文字
    void setHint(const QString& text);

signals:
    void playPhaseEnded();       // 结束出牌
    void respondSkipped();       // 跳过响应
    void discardConfirmed();     // 确认弃牌

private:
    void setupUi();
    void hideAllButtons();

    QLabel*      m_hintLabel;          // 提示文字区域
    QPushButton* m_endPlayBtn;         // "结束出牌"
    QPushButton* m_skipResponseBtn;    // "跳过"
    QPushButton* m_confirmDiscardBtn;  // "确认弃牌"
};

#endif // ACTIONPANELWIDGET_H
