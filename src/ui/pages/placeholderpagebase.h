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

class QLabel;

class PlaceholderPageBase : public QWidget
{
    Q_OBJECT

public:
    explicit PlaceholderPageBase(QWidget *parent = nullptr);
    ~PlaceholderPageBase() override = default;

    // 绑定具体页面 ui_*.h 里的标题 QLabel。
    void setTitleLabel(QLabel *label);

    // 更新标题文本。
    void setTitle(const QString &title);

protected:
    QLabel *titleLabel() const;

    // 应用统一占位页样式。
    void applyDefaultPlaceholderStyle(const ThemePalette &palette);

private:
    QLabel *m_titleLabel = nullptr;
};
