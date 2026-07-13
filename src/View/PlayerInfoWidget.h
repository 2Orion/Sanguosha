#ifndef PLAYERINFOWIDGET_H
#define PLAYERINFOWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QString>
#include <QMetaObject>
#include <vector>

class PlayerViewModel;

/// 玩家信息面板 — 显示头像、姓名、体力（❤️）、武将、技能、手牌张数、当前回合标记
class PlayerInfoWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlayerInfoWidget(QWidget* parent = nullptr);
    ~PlayerInfoWidget() override;

    /// 绑定数据源，开始监听事件（通过 Qt 信号槽）
    void bindViewModel(PlayerViewModel* pvm);

    /// 解除绑定
    void unbindViewModel();

    bool isCurrentPlayer() const { return m_isCurrent; }
    void setCurrentPlayer(bool isCurrent);

    PlayerViewModel* viewModel() const { return m_pvm; }

    /// 标记该面板是否可作为目标选择（高亮边框）
    bool isTargetable() const { return m_targetable; }
    void setTargetable(bool targetable);

signals:
    /// 玩家点击此面板（用于目标选择）
    void clicked(int playerIndex);

private slots:
    void updateHp();
    void updateHandCount();
    void onDying();
    void refreshAll();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void setupUi();

    PlayerViewModel* m_pvm = nullptr;

    // UI 元素
    QLabel* m_avatarLabel;
    QLabel* m_nameLabel;
    QLabel* m_hpLabel;
    QLabel* m_skillLabel;
    QLabel* m_handCountLabel;
    QLabel* m_turnIndicator;

    bool m_isCurrent = false;
    bool m_targetable = false;

    // Qt 信号槽连接（断开用）
    std::vector<QMetaObject::Connection> m_connections;
};

#endif // PLAYERINFOWIDGET_H
