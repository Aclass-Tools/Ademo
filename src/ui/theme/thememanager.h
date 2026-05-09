// 主题管理器（ThemeManager）
// -------------------------------
// 主题色 token 与样式字符串生成器。
//
// 集中化收益：
// - 避免颜色硬编码分散在各个窗口/页面。
// - 支持快速切换主题（浅色 / 深色 / 工业蓝）。
// - 保持解耦组件之间的视觉一致性。

#pragma once

#include <QString>

struct ThemePalette {
    // 全局背景与分割线。
    QString windowBg;
    QString topBarBg;
    QString separator;
    QString stackBg;
    QString stackBorder;

    // 文本与语义状态色。
    QString textPrimary;
    QString textSecondary;
    QString accentPrimary;
    QString dangerText;

    // 状态栏与内容区面板。
    QString statusBg;
    QString statusBorder;
    QString panelBg;
    QString panelBorder;

    // 顶部导航按钮四态。
    QString navButtonBg;
    QString navButtonBorder;
    QString navButtonText;
    QString navButtonHoverBg;
    QString navButtonHoverBorder;
    QString navButtonPressedBg;
    QString navButtonCheckedBg;
    QString navButtonCheckedBorder;
    QString navButtonCheckedText;
    QString navButtonCheckedAccent;

    // 占位页标题色。
    QString placeholderTitle;
};

enum class ThemeKind {
    Light,
    Dark,
    IndustrialBlue
};

class ThemeManager
{
public:
    static ThemePalette palette(ThemeKind kind);

    // 主窗体外壳级别 QSS。
    static QString mainWindowStyle(const ThemePalette &p);

    // 顶部导航按钮 QSS。
    static QString navButtonStyle(const ThemePalette &p);

    // 通用占位页 QSS。
    static QString placeholderPageStyle(const ThemePalette &p);
};
