// 外挂页面（PluginPage）
// ----------------------
// 外挂占位页组件。

#pragma once

#include "placeholderpagebase.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class PluginPage;
}
QT_END_NAMESPACE

class PluginPage : public PlaceholderPageBase
{
    Q_OBJECT

public:
    explicit PluginPage(QWidget *parent = nullptr);
    ~PluginPage();

private:
    Ui::PluginPage *ui;
};
