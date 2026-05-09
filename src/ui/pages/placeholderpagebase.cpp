#include "placeholderpagebase.h"

#include <QLabel>
#include "ui/theme/thememanager.h"

PlaceholderPageBase::PlaceholderPageBase(QWidget *parent)
    : QWidget(parent)
{
}

void PlaceholderPageBase::setTitleLabel(QLabel *label)
{
    m_titleLabel = label;
}

void PlaceholderPageBase::setTitle(const QString &title)
{
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
}

QLabel *PlaceholderPageBase::titleLabel() const
{
    return m_titleLabel;
}

void PlaceholderPageBase::applyDefaultPlaceholderStyle(const ThemePalette &palette)
{
    // 样式限定在页面局部，避免污染全局 QLabel。
    setStyleSheet(ThemeManager::placeholderPageStyle(palette));
}
