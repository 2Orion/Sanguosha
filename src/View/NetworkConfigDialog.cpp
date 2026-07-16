#include "NetworkConfigDialog.h"
#include "Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFont>
#include <QIntValidator>

NetworkConfigDialog::NetworkConfigDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("加入房间 — 网络设置"));
    setFixedSize(380, 200);
    setModal(true);
    setStyleSheet(Theme::dialogBase());

    // 主布局
    auto* main = new QVBoxLayout(this);
    main->setContentsMargins(20, 16, 20, 16);
    main->setSpacing(12);

    // 标题
    auto* title = new QLabel(QStringLiteral("连接到游戏服务器"), this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(Theme::accentText(Theme::Gold, 16));
    main->addWidget(title);

    // 表单
    auto* form = new QFormLayout;
    form->setSpacing(8);

    m_hostEdit = new QLineEdit(this);
    m_hostEdit->setPlaceholderText(QStringLiteral("服务器 IP 地址（如 192.168.1.100）"));
    m_hostEdit->setText(QStringLiteral("127.0.0.1"));
    m_hostEdit->setStyleSheet(Theme::inputField());
    form->addRow(QStringLiteral("服务器地址:"), m_hostEdit);

    m_portSpinBox = new QSpinBox(this);
    m_portSpinBox->setRange(1024, 65535);
    m_portSpinBox->setValue(9527);
    m_portSpinBox->setStyleSheet(Theme::inputField());
    form->addRow(QStringLiteral("端口:"), m_portSpinBox);

    main->addLayout(form);
    main->addStretch();

    // 按钮行
    auto* btnRow = new QHBoxLayout;
    btnRow->setSpacing(12);

    m_cancelBtn = new QPushButton(QStringLiteral("取消"), this);
    m_cancelBtn->setStyleSheet(Theme::flatButton());
    connect(m_cancelBtn, &QPushButton::clicked, this, &NetworkConfigDialog::onCancel);
    btnRow->addWidget(m_cancelBtn);

    m_connectBtn = new QPushButton(QStringLiteral("连接"), this);
    m_connectBtn->setStyleSheet(Theme::gradientButton(
        "#3E7A46", "#285230", "#4E9458", "#33663C", "#1D3E24"));
    connect(m_connectBtn, &QPushButton::clicked, this, &NetworkConfigDialog::onConnect);
    btnRow->addWidget(m_connectBtn);

    main->addLayout(btnRow);
}

void NetworkConfigDialog::onConnect()
{
    QString host = m_hostEdit->text().trimmed();
    if (host.isEmpty()) {
        m_hostEdit->setPlaceholderText(QStringLiteral("请输入服务器地址！"));
        m_hostEdit->setFocus();
        return;
    }
    emit connectRequested(host, static_cast<quint16>(m_portSpinBox->value()));
    accept();
}

void NetworkConfigDialog::onCancel()
{
    reject();
}
