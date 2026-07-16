#ifndef PLAYERINFOWIDGET_H
#define PLAYERINFOWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "PlayerData.h"

/// 玩家信息面板 — 只依赖 Common 层 PlayerData
class PlayerInfoWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlayerInfoWidget(QWidget* parent = nullptr);

    void setDisplayData(const PlayerData& data);
    void setCurrentPlayer(bool isCurrent);
    bool isCurrentPlayer() const { return m_isCurrent; }

    bool isTargetable() const { return m_targetable; }
    void setTargetable(bool targetable);

    int playerId() const { return m_playerId; }

signals:
    void clicked(int playerIndex);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    void setupUi();
    void refreshDisplay();

    int m_playerId = -1;
    bool m_isCurrent = false;
    bool m_targetable = false;

    QLabel* m_avatarLabel;
    QLabel* m_nameLabel;
    QLabel* m_hpLabel;
    QLabel* m_skillLabel;
    QLabel* m_handCountLabel;
    QLabel* m_turnIndicator;

    // 装备区
    QLabel* m_equipLabel;
    QLabel* m_attackRangeLabel;

    // 判定区
    QLabel* m_judgmentLabel;
};

#endif // PLAYERINFOWIDGET_H
