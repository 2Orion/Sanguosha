#ifndef ACTIONPANELWIDGET_H
#define ACTIONPANELWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include "CommonTypes.h"
#include "PendingActionVM.h"

/// 操作按钮面板 — 依赖 Common 层 PendingActionVM
class ActionPanelWidget : public QWidget {
    Q_OBJECT
public:
    explicit ActionPanelWidget(QWidget* parent = nullptr);

    void updateForPhase(PhaseType phase, bool hasPendingAction);
    void updateForPendingAction(const PendingActionVM& info);
    void setHint(const QString& text);

signals:
    void playPhaseEnded();
    void respondSkipped();
    void discardConfirmed();

private:
    void setupUi();
    void hideAllButtons();

    QLabel*      m_hintLabel;
    QPushButton* m_endPlayBtn;
    QPushButton* m_skipResponseBtn;
    QPushButton* m_confirmDiscardBtn;
};

#endif // ACTIONPANELWIDGET_H
