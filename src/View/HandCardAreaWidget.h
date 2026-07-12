#ifndef HANDCAREAWIDGET_H
#define HANDCAREAWIDGET_H

#include <QWidget>
#include <vector>
#include <memory>

class CardViewModel;
class CardWidget;

/// 手牌区域 — 展示一名玩家的所有手牌，支持选中和交互
/// 己方：正面朝上，可交互；对手：牌背，仅展示
class HandCardAreaWidget : public QWidget {
    Q_OBJECT
public:
    explicit HandCardAreaWidget(QWidget* parent = nullptr);
    ~HandCardAreaWidget() override;

    /// 传入 CardViewModel 列表（转移所有权）并刷新显示
    void setCards(std::vector<std::unique_ptr<CardViewModel>> cards,
                  bool isOpponent = false);

    /// 刷新所有卡牌的可打出状态（从 CardViewModel 读取）
    void refreshPlayableState();

    /// 获取当前选中卡牌的 ID，-1 表示无选中
    int selectedCardId() const;
    void clearSelection();

    bool isOpponent() const { return m_isOpponent; }

    /// 通过 cardId 查找 CardViewModel（供外部显示查询）
    CardViewModel* cardVM(int cardId) const;

signals:
    /// 所有卡牌交互信号均使用 int cardId（不暴露 Model 指针）
    void cardClicked(int cardId);
    void cardDoubleClicked(int cardId);
    void selectionChanged(int cardId);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onCardWidgetClicked(int cardId);
    void onCardWidgetDoubleClicked(int cardId);

private:
    void arrangeCards();
    void rebuildWidgets();
    void clearWidgets();
    CardWidget* findWidgetByCardId(int cardId) const;

    bool m_isOpponent = false;
    std::vector<std::unique_ptr<CardViewModel>> m_ownedCardVMs; // 持有所有 CardVM
    std::vector<CardWidget*> m_cardWidgets;                     // 子控件（不持有）
    std::vector<CardViewModel*> m_cardViewModels;               // 指向 m_ownedCardVMs 中的元素

    static constexpr int CARD_WIDTH  = 80;
    static constexpr int CARD_HEIGHT = 112;
    static constexpr int CARD_OVERLAP = 20;           // 卡牌重叠像素
    static constexpr int MIN_SPACING = 4;              // 最小间距
    static constexpr int TOP_PADDING = 8;              // 顶部留白（选中上移空间）
};

#endif // HANDCAREAWIDGET_H
