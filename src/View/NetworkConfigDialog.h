#ifndef NETWORKCONFIGDIALOG_H
#define NETWORKCONFIGDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>

/// 联网配置对话框 — 纯 UI，不依赖 Network 层类型，通过信号传出 host/port
class NetworkConfigDialog : public QDialog {
    Q_OBJECT
public:
    explicit NetworkConfigDialog(QWidget* parent = nullptr);

signals:
    /// 用户点击「连接」时发射，携带目标地址和端口
    void connectRequested(const QString& host, quint16 port);

private slots:
    void onConnect();
    void onCancel();

private:
    QLineEdit*  m_hostEdit;
    QSpinBox*   m_portSpinBox;
    QPushButton* m_connectBtn;
    QPushButton* m_cancelBtn;
};

#endif // NETWORKCONFIGDIALOG_H
