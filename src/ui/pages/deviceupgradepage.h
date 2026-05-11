// 设备升级页面（DeviceUpgradePage）
// ---------------------------------
// 设备升级占位页组件。

#pragma once

#include "placeholderpagebase.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class DeviceUpgradePage;
}
QT_END_NAMESPACE

class DeviceUpgradePage : public PlaceholderPageBase
{
    Q_OBJECT

public:
    explicit DeviceUpgradePage(QWidget *parent = nullptr);
    ~DeviceUpgradePage();

private:
    Ui::DeviceUpgradePage *ui;
};
