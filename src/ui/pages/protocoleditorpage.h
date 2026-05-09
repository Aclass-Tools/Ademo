// 协议编辑页面（ProtocolEditorPage）
// ------------------------------------
// 协议编辑占位页组件。

#pragma once

#include "placeholderpagebase.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class ProtocolEditorPage;
}
QT_END_NAMESPACE

class ProtocolEditorPage : public PlaceholderPageBase
{
    Q_OBJECT

public:
    explicit ProtocolEditorPage(QWidget *parent = nullptr);
    ~ProtocolEditorPage();

private:
    Ui::ProtocolEditorPage *ui;
};
