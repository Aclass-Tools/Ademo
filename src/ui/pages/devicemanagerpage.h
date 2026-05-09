// 设备管理页面（DeviceManagerPage）
// ------------------------------------
// 设备管理占位页组件。

#pragma once

#include "placeholderpagebase.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class DeviceManagerPage;
}
QT_END_NAMESPACE

class DeviceManagerPage : public PlaceholderPageBase
{
    Q_OBJECT

public:
    explicit DeviceManagerPage(QWidget *parent = nullptr);
    ~DeviceManagerPage();

private:
    Ui::DeviceManagerPage *ui;
};
