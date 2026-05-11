// 协议导出页面（ProtocolExportPage）
// ----------------------------------
// 协议导出占位页组件。

#pragma once

#include "placeholderpagebase.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class ProtocolExportPage;
}
QT_END_NAMESPACE

class ProtocolExportPage : public PlaceholderPageBase
{
    Q_OBJECT

public:
    explicit ProtocolExportPage(QWidget *parent = nullptr);
    ~ProtocolExportPage();

private:
    Ui::ProtocolExportPage *ui;
};
