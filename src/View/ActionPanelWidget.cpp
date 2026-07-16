#include "ActionPanelWidget.h"
#include "Theme.h"
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
    m_hintLabel->setStyleSheet(Theme::hintBar());
    layout->addWidget(m_hintLabel, 1); // stretch = 1

    // 结束出牌（暗朱红）
    m_endPlayBtn = new QPushButton(QStringLiteral("结束出牌"), this);
    m_endPlayBtn->setStyleSheet(Theme::gradientButton(
        "#A03A28", "#71271A", "#B84A34", "#8C3020", "#571D12"));
    connect(m_endPlayBtn, &QPushButton::clicked, this, &ActionPanelWidget::playPhaseEnded);
    layout->addWidget(m_endPlayBtn);

    // 发动技能（暗紫）
    m_skillBtn = new QPushButton(QStringLiteral("发动技能"), this);
    m_skillBtn->setObjectName(QStringLiteral("skillButton"));
    m_skillBtn->setStyleSheet(Theme::gradientButton(
        "#6E4098", "#4A2870", "#8250B0", "#5C3488", "#38195A"));
    connect(m_skillBtn, &QPushButton::clicked, this, &ActionPanelWidget::skillRequested);
    layout->addWidget(m_skillBtn);

    // 跳过响应（深色扁平次要按钮）
    m_skipResponseBtn = new QPushButton(QStringLiteral("跳过"), this);
    m_skipResponseBtn->setStyleSheet(Theme::flatButton());
    connect(m_skipResponseBtn, &QPushButton::clicked, this, &ActionPanelWidget::respondSkipped);
    layout->addWidget(m_skipResponseBtn);

    // 确认弃牌（暗松绿）
    m_confirmDiscardBtn = new QPushButton(QStringLiteral("确认弃牌"), this);
    m_confirmDiscardBtn->setStyleSheet(Theme::gradientButton(
        "#3E7A46", "#285230", "#4E9458", "#33663C", "#1D3E24"));
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

void ActionPanelWidget::setSkillAvailable(bool available)
{
    m_skillBtn->setVisible(available);
}

void ActionPanelWidget::setSkillSelectionMode(bool selecting)
{
    m_skillBtn->setText(selecting ? QStringLiteral("确认技能")
                                  : QStringLiteral("发动技能"));
    if (selecting) m_skillBtn->setVisible(true);
}

void ActionPanelWidget::hideAllButtons()
{
    m_endPlayBtn->setVisible(false);
    m_skipResponseBtn->setVisible(false);
    m_confirmDiscardBtn->setVisible(false);
    m_skillBtn->setVisible(false);
}
