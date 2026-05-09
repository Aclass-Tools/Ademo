// 终端页面（TerminalPage）
// -------------------------------
// 终端占位页组件。

#pragma once

#include "placeholderpagebase.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class TerminalPage;
}
QT_END_NAMESPACE

class TerminalPage : public PlaceholderPageBase
{
    Q_OBJECT

public:
    explicit TerminalPage(QWidget *parent = nullptr);
    ~TerminalPage();

private:
    Ui::TerminalPage *ui;
};
