#ifndef PLAYERINFOWIDGET_H
#define PLAYERINFOWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QString>
#include <string>
#include <vector>

class PlayerViewModel;

/// 玩家信息面板 — 显示头像、姓名、体力（❤️）、武将、技能、手牌张数、当前回合标记
class PlayerInfoWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlayerInfoWidget(QWidget* parent = nullptr);
    ~PlayerInfoWidget() override;

    /// 绑定数据源，开始监听事件
    void bindViewModel(PlayerViewModel* pvm);

    /// 解除绑定（析构或切换玩家时调用）
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
    void connectViewModelEvents();
    void disconnectViewModelEvents();
    void setupUi();

    PlayerViewModel* m_pvm = nullptr;

    // UI 元素
    QLabel* m_avatarLabel;       // 武将头像（用文字/emoji代替）
    QLabel* m_nameLabel;         // 玩家名称
    QLabel* m_hpLabel;           // 体力 ❤🖤
    QLabel* m_skillLabel;        // 技能名 + 描述
    QLabel* m_handCountLabel;    // 手牌张数
    QLabel* m_turnIndicator;     // "当前回合" 标记

    bool m_isCurrent = false;
    bool m_targetable = false;

    // EventListener 连接 ID 列表（用于断开）
    std::vector<size_t> m_connectionIds;
};

#endif // PLAYERINFOWIDGET_H
