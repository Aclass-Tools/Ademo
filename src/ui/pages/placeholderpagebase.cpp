#include "placeholderpagebase.h"

#include "models/projectsummarycontext.h"
#include "ui/theme/thememanager.h"

PlaceholderPageBase::PlaceholderPageBase(QWidget *parent)
    : QWidget(parent)
{
}

void PlaceholderPageBase::applyDefaultPlaceholderStyle(const ThemePalette &palette)
{
    // 样式限定在页面局部，避免污染全局 QLabel。
    setStyleSheet(ThemeManager::placeholderPageStyle(palette));
}

void PlaceholderPageBase::setProjectContext(
    const std::shared_ptr<const ProjectSummaryContext> &projectContext)
{
    m_projectContext = projectContext;
}

std::shared_ptr<const ProjectSummaryContext> PlaceholderPageBase::projectContext() const
{
    return m_projectContext;
}
