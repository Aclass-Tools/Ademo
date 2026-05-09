// 日志页面（LogPage）
// --------------------
// 日志占位页组件。

#pragma once

#include "placeholderpagebase.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class LogPage;
}
QT_END_NAMESPACE

class LogPage : public PlaceholderPageBase
{
    Q_OBJECT

public:
    explicit LogPage(QWidget *parent = nullptr);
    ~LogPage();

private:
    Ui::LogPage *ui;
};
