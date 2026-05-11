// 占位页基类（PlaceholderPageBase）
// -----------------------------------
// 简单占位页的统一基类。
//
// 存在意义：
// - 避免多个占位页重复实现标题绑定。
// - 提供统一占位页样式入口。

#pragma once

#include <QWidget>
#include "ui/theme/thememanager.h"

class PlaceholderPageBase : public QWidget
{
    Q_OBJECT

public:
    explicit PlaceholderPageBase(QWidget *parent = nullptr);
    ~PlaceholderPageBase() override = default;

    // 应用统一占位页样式。
protected:
    void applyDefaultPlaceholderStyle(const ThemePalette &palette);
};
