// 占位页基类（PlaceholderPageBase）
// -----------------------------------
// 简单占位页的统一基类。
//
// 存在意义：
// - 避免多个占位页重复实现标题绑定。
// - 提供统一占位页样式入口。

#pragma once

#include <QWidget>
#include <memory>
#include "ui/theme/thememanager.h"

struct ProjectSummaryContext;

class PlaceholderPageBase : public QWidget
{
    Q_OBJECT

public:
    explicit PlaceholderPageBase(QWidget *parent = nullptr);
    ~PlaceholderPageBase() override = default;
    // 注入跨页面共享的“项目摘要上下文”（只读）。
    // 约束：基类与普通页面只保留只读指针，不直接修改内容。
    void setProjectContext(const std::shared_ptr<const ProjectSummaryContext> &projectContext);

    // 应用统一占位页样式。
protected:
    void applyDefaultPlaceholderStyle(const ThemePalette &palette);
    // 供子类按需读取共享上下文。
    std::shared_ptr<const ProjectSummaryContext> projectContext() const;

private:
    std::shared_ptr<const ProjectSummaryContext> m_projectContext;
};
