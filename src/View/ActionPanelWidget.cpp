#include "ActionPanelWidget.h"
#include <QFont>

// ==================== 构造 ====================

ActionPanelWidget::ActionPanelWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

// ==================== UI 构建 ====================

void ActionPanelWidget::setupUi()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(8);

    // 提示文字
    m_hintLabel = new QLabel(this);
    m_hintLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 13px;"
        "  color: #555;"
        "  padding: 4px 8px;"
        "  background: #F5F5F5;"
        "  border: 1px solid #E0E0E0;"
        "  border-radius: 4px;"
        "}"
    );
    layout->addWidget(m_hintLabel, 1); // stretch = 1

    // 结束出牌
    m_endPlayBtn = new QPushButton(QStringLiteral("结束出牌"), this);
    m_endPlayBtn->setStyleSheet(
        "QPushButton {"
        "  font-size: 13px; font-weight: bold;"
        "  color: white;"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #FF7043, stop:1 #E64A19);"
        "  border: none; border-radius: 4px;"
        "  padding: 6px 16px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #FF8A65, stop:1 #F4511E);"
        "}"
        "QPushButton:pressed {"
        "  background: #BF360C;"
        "}"
    );
    connect(m_endPlayBtn, &QPushButton::clicked, this, &ActionPanelWidget::playPhaseEnded);
    layout->addWidget(m_endPlayBtn);

    // 跳过响应
    m_skipResponseBtn = new QPushButton(QStringLiteral("跳过"), this);
    m_skipResponseBtn->setStyleSheet(
        "QPushButton {"
        "  font-size: 13px;"
        "  color: #555;"
        "  background: #F5F5F5;"
        "  border: 1px solid #CCC; border-radius: 4px;"
        "  padding: 6px 16px;"
        "}"
        "QPushButton:hover {"
        "  background: #E0E0E0;"
        "}"
        "QPushButton:pressed {"
        "  background: #BDBDBD;"
        "}"
    );
    connect(m_skipResponseBtn, &QPushButton::clicked, this, &ActionPanelWidget::respondSkipped);
    layout->addWidget(m_skipResponseBtn);

    // 确认弃牌
    m_confirmDiscardBtn = new QPushButton(QStringLiteral("确认弃牌"), this);
    m_confirmDiscardBtn->setStyleSheet(
        "QPushButton {"
        "  font-size: 13px; font-weight: bold;"
        "  color: white;"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #66BB6A, stop:1 #388E3C);"
        "  border: none; border-radius: 4px;"
        "  padding: 6px 16px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #81C784, stop:1 #43A047);"
        "}"
        "QPushButton:pressed {"
        "  background: #1B5E20;"
        "}"
    );
    connect(m_confirmDiscardBtn, &QPushButton::clicked, this, &ActionPanelWidget::discardConfirmed);
    layout->addWidget(m_confirmDiscardBtn);

    // 初始状态：全部隐藏
    hideAllButtons();
    m_hintLabel->setText(QStringLiteral("准备中..."));
}

// ==================== 更新方法 ====================

void ActionPanelWidget::updateForPhase(PhaseType phase, bool hasPendingAction)
{
    hideAllButtons();

    if (hasPendingAction) {
        // 有响应待定时，updateForPendingAction 会接管显示
        m_hintLabel->setText(QStringLiteral("等待响应..."));
        return;
    }

    switch (phase) {
    case PhaseType::Prepare:
    case PhaseType::Judge:
    case PhaseType::Draw:
        m_hintLabel->setText(QStringLiteral("请等待...（自动阶段中）"));
        break;

    case PhaseType::Play:
        m_hintLabel->setText(QStringLiteral("出牌阶段 — 选择手牌使用"));
        m_endPlayBtn->setVisible(true);
        break;

    case PhaseType::Discard:
        m_hintLabel->setText(QStringLiteral("弃牌阶段 — 请选择要弃置的手牌"));
        m_confirmDiscardBtn->setVisible(true);
        break;

    case PhaseType::End:
        m_hintLabel->setText(QStringLiteral("回合结束"));
        break;
    }
}

void ActionPanelWidget::updateForPendingAction(const PendingActionData& info)
{
    hideAllButtons();

    m_hintLabel->setText(info.description);

    // 始终显示跳过按钮：玩家可选择打出牌或承担后果
    // canSkip=true 表示"无惩罚跳过"（如 AOE 跳过直接扣血）
    // canSkip=false 表示"不能跳过"（如杀→闪），
    // 但我们仍然显示跳过按钮让玩家选择不响应直接扣血
    m_skipResponseBtn->setVisible(true);
}

void ActionPanelWidget::setHint(const QString& text)
{
    m_hintLabel->setText(text);
}

// ==================== 内部辅助 ====================

void ActionPanelWidget::hideAllButtons()
{
    m_endPlayBtn->setVisible(false);
    m_skipResponseBtn->setVisible(false);
    m_confirmDiscardBtn->setVisible(false);
}
