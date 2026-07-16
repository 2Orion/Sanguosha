#ifndef VIEW_THEME_H
#define VIEW_THEME_H

#include <QString>
#include <QColor>

/// 深色古风主题 — 仅 View 层使用的颜色常量与 QSS 片段工具（无逻辑、无状态）。
/// 所有跨控件的配色都从这里取值，避免十六进制色值散落在各 View 文件里漂移。
namespace Theme {

// ==================== 基础色板 ====================
// 牌桌
inline const QColor TableGreenLight(27, 64, 45);      // 桌面渐变亮端 #1B402D
inline const QColor TableGreenDark(13, 33, 23);       // 桌面渐变暗端 #0D2117
inline const QColor TableVignette(0, 0, 0, 90);       // 四周暗角

// 面板（暗木色）
inline const char* const PanelBg        = "#241C13";  // 面板底色
inline const char* const PanelBgLight   = "#2E241A";  // 面板渐变亮端
inline const char* const PanelBorder    = "#5A4632";  // 常态描边（暗铜）
inline const char* const TextPrimary    = "#E8DCC8";  // 主文字（米白）
inline const char* const TextMuted      = "#A89878";  // 次要文字（灰金）

// 点缀
inline const char* const Gold           = "#D4AF37";  // 金色主点缀
inline const char* const GoldDark       = "#9C7E24";
inline const char* const CurrentBg      = "#332714";  // 当前回合暖底
inline const char* const TargetGreen    = "#66BB6A";  // 可选目标绿
inline const char* const TargetBg       = "#1B3320";
inline const char* const DyingRed       = "#E53935";  // 濒死红
inline const char* const DyingBg        = "#3A1512";

// ==================== 玩家面板四态 ====================
inline QString panelNormal()
{
    return QStringLiteral(
        "PlayerInfoWidget { border: 1px solid %1; border-radius: 10px;"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %2, stop:1 %3); }")
        .arg(QLatin1String(PanelBorder), QLatin1String(PanelBgLight), QLatin1String(PanelBg));
}

inline QString panelCurrent()
{
    return QStringLiteral(
        "PlayerInfoWidget { border: 2px solid %1; border-radius: 10px;"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %2, stop:1 %3); }")
        .arg(QLatin1String(Gold), QLatin1String(CurrentBg), QLatin1String(PanelBg));
}

inline QString panelTargetable()
{
    return QStringLiteral(
        "PlayerInfoWidget { border: 2px solid %1; border-radius: 10px; background-color: %2; }"
        "PlayerInfoWidget:hover { background-color: #26482E; }")
        .arg(QLatin1String(TargetGreen), QLatin1String(TargetBg));
}

inline QString panelDying()
{
    return QStringLiteral(
        "PlayerInfoWidget { border: 2px solid %1; border-radius: 10px; background-color: %2; }")
        .arg(QLatin1String(DyingRed), QLatin1String(DyingBg));
}

// ==================== 通用小徽章 ====================
/// 深底小圆角徽章（装备/判定/攻击范围/回合指示共用形状，仅换色）
inline QString badge(const char* fg, const char* bg, const char* border, int fontPx = 11)
{
    return QStringLiteral(
        "font-size: %4px; color: %1; font-weight: bold;"
        " background: %2; border: 1px solid %3; border-radius: 4px; padding: 1px 5px;")
        .arg(QLatin1String(fg), QLatin1String(bg), QLatin1String(border)).arg(fontPx);
}

// ==================== 按钮 ====================
/// 主题渐变按钮：top/bottom 为渐变两端，hoverTop/hoverBottom 悬停两端，pressed 按下纯色
inline QString gradientButton(const char* top, const char* bottom,
                              const char* hoverTop, const char* hoverBottom,
                              const char* pressed, int fontPx = 13)
{
    return QStringLiteral(
        "QPushButton { font-size: %6px; font-weight: bold; color: #F5EFDF;"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %1, stop:1 %2);"
        "  border: 1px solid rgba(212,175,55,90); border-radius: 5px; padding: 6px 16px; }"
        "QPushButton:hover { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %3, stop:1 %4); }"
        "QPushButton:pressed { background: %5; }"
        "QPushButton:disabled { background: #3A342A; color: #7A705C; border-color: #4A4436; }")
        .arg(QLatin1String(top), QLatin1String(bottom), QLatin1String(hoverTop),
             QLatin1String(hoverBottom), QLatin1String(pressed)).arg(fontPx);
}

/// 次要按钮（深色描边扁平，用于「跳过」/「取消」类）
inline QString flatButton(int fontPx = 13)
{
    return QStringLiteral(
        "QPushButton { font-size: %1px; color: %2;"
        "  background: #2A241C; border: 1px solid %3; border-radius: 5px; padding: 6px 16px; }"
        "QPushButton:hover { background: #383024; border-color: %4; }"
        "QPushButton:pressed { background: #1E1913; }")
        .arg(fontPx).arg(QLatin1String(TextPrimary), QLatin1String(PanelBorder), QLatin1String(GoldDark));
}

// ==================== 提示/日志条 ====================
inline QString hintBar(int fontPx = 13)
{
    return QStringLiteral(
        "QLabel { font-size: %1px; color: %2; padding: 4px 10px;"
        "  background: rgba(0,0,0,110); border: 1px solid %3; border-radius: 5px; }")
        .arg(fontPx).arg(QLatin1String(TextPrimary), QLatin1String(PanelBorder));
}

/// 判定结果横幅：effective=true 红（生效）/ false 绿（未生效），深色变体
inline QString judgmentBar(bool effective)
{
    return effective
        ? QStringLiteral(
              "QLabel { font-size: 13px; color: #FF8A80; font-weight: bold; padding: 6px 12px;"
              "  background: rgba(80,15,10,170); border: 1px solid #B71C1C; border-radius: 5px; }")
        : QStringLiteral(
              "QLabel { font-size: 13px; color: #A5D6A7; font-weight: bold; padding: 6px 12px;"
              "  background: rgba(15,60,25,170); border: 1px solid #2E7D32; border-radius: 5px; }");
}

// ==================== 选将页 / 对话框 ====================
// 玩家强调色（选将列标题/已选提示；深底上取亮色变体保证可读）
inline const char* const P1Accent = "#6FB1E0";    // 玩家 1 青蓝
inline const char* const P2Accent = "#E0925A";    // 玩家 2 赤橙

/// 页面深色底（选将页等顶层 QWidget）。需配合 setObjectName + WA_StyledBackground；
/// 用 objectName 限定选择器，避免背景级联到全部子控件。
inline QString pageBackground(const char* objectName)
{
    return QStringLiteral(
        "#%1 { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #221A12, stop:1 #120D08); }")
        .arg(QLatin1String(objectName));
}

/// 页面大标题（金色）
inline QString pageTitle(int fontPx = 28)
{
    return QStringLiteral(
        "font-size: %1px; font-weight: bold; color: %2; padding: 12px; background: transparent;")
        .arg(fontPx).arg(QLatin1String(Gold));
}

/// 页面副标题（灰金次要文字）
inline QString pageSubtitle(int fontPx = 14)
{
    return QStringLiteral("font-size: %1px; color: %2; background: transparent;")
        .arg(fontPx).arg(QLatin1String(TextMuted));
}

/// 强调文字（选将列标题/已选提示/联网标签）
inline QString accentText(const char* color, int fontPx = 12)
{
    return QStringLiteral("font-size: %1px; color: %2; font-weight: bold; background: transparent;")
        .arg(fontPx).arg(QLatin1String(color));
}

/// 次要文字（「未选择」占位等）
inline QString mutedText(int fontPx = 12, bool italic = false)
{
    return QStringLiteral("font-size: %1px; color: %2; background: transparent;%3")
        .arg(fontPx)
        .arg(QLatin1String(TextMuted),
             italic ? QStringLiteral(" font-style: italic;") : QString());
}

/// 选将卡片（QGroupBox）：暗木渐变 + 悬停金框，内部单选/说明文字级联配色
/// （悬停只换 border-color 不换宽度，避免内容几何抖动）
inline QString charCard()
{
    return QStringLiteral(
        "QGroupBox { border: 1px solid %1; border-radius: 8px;"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %2, stop:1 %3); padding: 8px; }"
        "QGroupBox:hover { border-color: %4; background: %5; }"
        "QRadioButton { font-size: 14px; font-weight: bold; color: %6; background: transparent; }"
        "QRadioButton::indicator { width: 16px; height: 16px; }"
        "QLabel { font-size: 12px; color: %7; background: transparent; }")
        .arg(QLatin1String(PanelBorder), QLatin1String(PanelBgLight), QLatin1String(PanelBg),
             QLatin1String(Gold), QLatin1String(CurrentBg),
             QLatin1String(TextPrimary), QLatin1String(TextMuted));
}

/// 信息条（联网状态/等待对手等提示）：深底 + 主题描边，比 badge 更大
inline QString infoBar(const char* fg, const char* bg, const char* border, int fontPx = 13)
{
    return QStringLiteral(
        "font-size: %4px; color: %1; font-weight: bold;"
        " background: %2; border: 1px solid %3; border-radius: 5px; padding: 6px 12px;")
        .arg(QLatin1String(fg), QLatin1String(bg), QLatin1String(border)).arg(fontPx);
}

/// 分割线（QFrame HLine）
inline QString separator()
{
    return QStringLiteral(
        "QFrame { background: %1; border: none; max-height: 1px; margin: 8px 0; }")
        .arg(QLatin1String(PanelBorder));
}

/// 深色输入框（QLineEdit / QSpinBox 共用）
inline QString inputField(int fontPx = 13)
{
    return QStringLiteral(
        "QLineEdit, QSpinBox { padding: 6px; font-size: %1px; color: %2;"
        "  background: #1A140E; border: 1px solid %3; border-radius: 4px;"
        "  selection-background-color: %4; selection-color: #1A140E; }"
        "QLineEdit:focus, QSpinBox:focus { border: 1px solid %5; }")
        .arg(fontPx)
        .arg(QLatin1String(TextPrimary), QLatin1String(PanelBorder),
             QLatin1String(GoldDark), QLatin1String(Gold));
}

/// 对话框深色底 + 表单标签级联配色（需 WA_StyledBackground）
inline QString dialogBase()
{
    return QStringLiteral(
        "QDialog { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #221A12, stop:1 #120D08); }"
        "QLabel { color: %1; background: transparent; }")
        .arg(QLatin1String(TextPrimary));
}

// ==================== 卡牌自绘配色（CardWidget QPainter 直接使用，非 QSS）====================
// 牌面纸质暖白渐变
inline const QColor CardPaperTop(250, 245, 231);      // 纸面渐变亮端（暖白）
inline const QColor CardPaperBottom(231, 220, 197);   // 纸面渐变暗端（米色）
inline const QColor CardPaperEmpty(238, 232, 216);    // 空占位牌底色
// 双线描边
inline const QColor CardBorderOuter(88, 66, 40);      // 外描边（暗棕）
inline const QColor CardBorderInner(207, 190, 150);   // 内描边（浅金）
inline const QColor CardEquipBorder(38, 122, 110);    // 装备牌外描边（青绿）
// 花色 / 卡名
inline const QColor CardSuitRed(188, 42, 38);         // 红花色 / 红角标
inline const QColor CardSuitBlack(44, 40, 34);        // 黑花色 / 黑角标
inline const QColor CardNameRed(170, 38, 32);         // 红牌卡名
inline const QColor CardNameBlack(48, 42, 34);        // 黑牌卡名
// 牌背金纹
inline const QColor CardBackTop(126, 32, 30);         // 牌背暗红亮端
inline const QColor CardBackBottom(84, 18, 18);       // 牌背暗红暗端
inline const QColor CardBackGold(214, 176, 92);       // 牌背金纹
// 底部类型徽章
inline const QColor BadgeBasicBg(74, 60, 40);         // 基本牌徽章底（暗棕）
inline const QColor BadgeStrategyBg(60, 46, 76);      // 锦囊徽章底（暗紫）
inline const QColor BadgeEquipBg(22, 76, 70);         // 装备徽章底（暗青绿）
inline const QColor BadgeText(240, 228, 200);         // 徽章文字（暖白）
// 状态覆盖层
inline const QColor OverlayPlayable(78, 188, 94);     // 可打出绿
inline const QColor OverlaySelected(60, 148, 222);    // 选中蓝
inline const QColor OverlayHoverGlow(255, 244, 200, 42); // 悬停暖光（带 alpha）

} // namespace Theme

#endif // VIEW_THEME_H
