#include "thememanager.h"

ThemePalette ThemeManager::palette(ThemeKind kind)
{
    switch (kind) {
    case ThemeKind::Dark:
        return ThemePalette{
            "#1b1f24", "#242a32", "#3a4350", "#20262d", "#3a4350",
            "#e6edf3", "#9fb1c6", "#5fa8ff", "#ff8e8e",
            "#242a32", "#3a4350", "#1f242b", "#3a4350",
            "#2b313a", "#4a5568", "#d6e2f2", "#344055", "#5f7aa3", "#40506a", "#2b466f", "#4f8bd7",
            "#ffffff", "#dbe8ff"
        };
    case ThemeKind::IndustrialBlue:
        return ThemePalette{
            "#eef5fb", "#dff0fb", "#9ec3dd", "#ffffff", "#c9d8e6",
            "#1f2d3d", "#5b6b7c", "#0e7fbf", "#d64b4b",
            "#f8fbff", "#c9d8e6", "#ffffff", "#d8e4ef",
            "#f2f8fe", "#b7ccdd", "#2f4255", "#e6f2fb", "#8fb0c8", "#d5e8f7", "#1f6fa8", "#155a88",
            "#ffffff", "#ffffff"
        };
    case ThemeKind::Light:
    default:
        return ThemePalette{
            "#f7f9fc", "#ffffff", "#d9dee7", "#ffffff", "#d9dee7",
            "#222b3a", "#5c6a7d", "#3a78c7", "#d94444",
            "#ffffff", "#d9dee7", "#ffffff", "#d9dee7",
            "#ffffff", "#c7cfdb", "#2a3547", "#f0f4fa", "#9db0ca", "#e7eef9", "#355c8a", "#274869",
            "#ffffff", "#ffffff"
        };
    }
}

QString ThemeManager::mainWindowStyle(const ThemePalette &p)
{
    return QString(
        "QMainWindow { background: %1; }"
        "#navButtonBar { background: %2; border: 1px solid %3; border-radius: 4px; }"
        "#lineTopNav { color: %3; max-height: 1px; }"
        "#mainPageStack { background: %4; border: 1px solid %5; border-radius: 2px; }"
        "#mainPageStack > QWidget { background: %4; }"
        "QLabel { color: %6; }"
        "QStatusBar { background: %7; border-top: 1px solid %8; color: %9; }"
        "QStatusBar QLabel { color: %9; padding: 0 6px; }"
        "QStatusBar QLabel#dangerLabel { color: %10; font-weight: 600; }"
    ).arg(
        p.windowBg,
        p.topBarBg,
        p.separator,
        p.stackBg,
        p.stackBorder,
        p.textPrimary,
        p.statusBg,
        p.statusBorder,
        p.textSecondary,
        p.dangerText
    );
}

QString ThemeManager::navButtonStyle(const ThemePalette &p)
{
    return QString(
        "QToolButton {"
        "  border: 1px solid %1;"
        "  border-radius: 3px;"
        "  padding: 5px 8px;"
        "  background: %2;"
        "  color: %3;"
        "  font-size: 15px;"
        "  font-weight: 500;"
        "}"
        "QToolButton:hover {"
        "  background: %4;"
        "  border: 1px solid %5;"
        "}"
        "QToolButton:pressed {"
        "  background: %6;"
        "}"
        "QToolButton:checked {"
        "  border: 1px solid %7;"
        "  background: %8;"
        "  color: %9;"
        "  font-weight: 600;"
        "  border-bottom: 2px solid %10;"
        "}"
        "QToolButton:disabled {"
        "  color: %11;"
        "  background: %12;"
        "  border: 1px solid %13;"
        "}"
    ).arg(
        p.navButtonBorder,
        p.navButtonBg,
        p.navButtonText,
        p.navButtonHoverBg,
        p.navButtonHoverBorder,
        p.navButtonPressedBg,
        p.navButtonCheckedBorder,
        p.navButtonCheckedBg,
        p.navButtonCheckedText,
        p.navButtonCheckedAccent,
        p.textSecondary,
        p.panelBg,
        p.panelBorder
    );
}

QString ThemeManager::placeholderPageStyle(const ThemePalette &p)
{
    return QString(
        "QWidget {"
        "  background: %1;"
        "}"
        "#panelFrame {"
        "  background: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 2px;"
        "}"
        "QLabel {"
        "  color: %4;"
        "  font-size: 18px;"
        "  font-weight: 600;"
        "}"
        "QLabel#descLabel {"
        "  color: %5;"
        "  font-size: 13px;"
        "  font-weight: 400;"
        "}"
    ).arg(
        p.stackBg,
        p.panelBg,
        p.panelBorder,
        p.placeholderTitle,
        p.textSecondary
    );
}
