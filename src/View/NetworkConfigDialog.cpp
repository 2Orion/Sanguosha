#include "NetworkConfigDialog.h"
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

    // 主布局
    auto* main = new QVBoxLayout(this);
    main->setContentsMargins(20, 16, 20, 16);
    main->setSpacing(12);

    // 标题
    auto* title = new QLabel(QStringLiteral("连接到游戏服务器"), this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 16px; font-weight: bold; color: #333;");
    main->addWidget(title);

    // 表单
    auto* form = new QFormLayout;
    form->setSpacing(8);

    m_hostEdit = new QLineEdit(this);
    m_hostEdit->setPlaceholderText(QStringLiteral("服务器 IP 地址（如 192.168.1.100）"));
    m_hostEdit->setText(QStringLiteral("127.0.0.1"));
    m_hostEdit->setStyleSheet("QLineEdit { padding: 6px; font-size: 13px;"
        " border: 1px solid #CCC; border-radius: 4px; }");
    form->addRow(QStringLiteral("服务器地址:"), m_hostEdit);

    m_portSpinBox = new QSpinBox(this);
    m_portSpinBox->setRange(1024, 65535);
    m_portSpinBox->setValue(9527);
    m_portSpinBox->setStyleSheet("QSpinBox { padding: 6px; font-size: 13px;"
        " border: 1px solid #CCC; border-radius: 4px; }");
    form->addRow(QStringLiteral("端口:"), m_portSpinBox);

    main->addLayout(form);
    main->addStretch();

    // 按钮行
    auto* btnRow = new QHBoxLayout;
    btnRow->setSpacing(12);

    m_cancelBtn = new QPushButton(QStringLiteral("取消"), this);
    m_cancelBtn->setStyleSheet(
        "QPushButton { font-size: 13px; color: #555;"
        "  background: #F5F5F5; border: 1px solid #CCC; border-radius: 4px;"
        "  padding: 8px 24px; }"
        "QPushButton:hover { background: #E0E0E0; }");
    connect(m_cancelBtn, &QPushButton::clicked, this, &NetworkConfigDialog::onCancel);
    btnRow->addWidget(m_cancelBtn);

    m_connectBtn = new QPushButton(QStringLiteral("连接"), this);
    m_connectBtn->setStyleSheet(
        "QPushButton { font-size: 13px; font-weight: bold; color: white;"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #43A047, stop:1 #2E7D32);"
        "  border: none; border-radius: 4px; padding: 8px 24px; }"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #66BB6A, stop:1 #388E3C); }");
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
