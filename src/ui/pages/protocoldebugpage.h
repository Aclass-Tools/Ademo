// 协议调试页面（ProtocolDebugPage）
// ---------------------------------
// 协议调试占位页组件。

#pragma once

#include "placeholderpagebase.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class ProtocolDebugPage;
}
QT_END_NAMESPACE

class ProtocolDebugPage : public PlaceholderPageBase
{
    Q_OBJECT

public:
    explicit ProtocolDebugPage(QWidget *parent = nullptr);
    ~ProtocolDebugPage();

private:
    Ui::ProtocolDebugPage *ui;
};
