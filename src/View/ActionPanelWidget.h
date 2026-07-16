#ifndef ACTIONPANELWIDGET_H
#define ACTIONPANELWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include "CommonTypes.h"
#include "PendingActionData.h"

/// 操作按钮面板 — 依赖 Common 层 PendingActionData
class ActionPanelWidget : public QWidget {
    Q_OBJECT
public:
    explicit ActionPanelWidget(QWidget* parent = nullptr);

    void updateForPhase(PhaseType phase, bool hasPendingAction);
    void updateForPendingAction(const PendingActionData& info);
    void setHint(const QString& text);
    void setSkillAvailable(bool available);

signals:
    void playPhaseEnded();
    void respondSkipped();
    void discardConfirmed();
    void skillRequested();

private:
    void setupUi();
    void hideAllButtons();

    QLabel*      m_hintLabel;
    QPushButton* m_endPlayBtn;
    QPushButton* m_skipResponseBtn;
    QPushButton* m_confirmDiscardBtn;
    QPushButton* m_skillBtn;
};

#endif // ACTIONPANELWIDGET_H
