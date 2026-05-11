// 首页页面（HomePage）
// ----------------------
// 首页占位页组件。

#pragma once

#include "placeholderpagebase.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class HomePage;
}
QT_END_NAMESPACE

class HomePage : public PlaceholderPageBase
{
    Q_OBJECT

public:
    explicit HomePage(QWidget *parent = nullptr);
    ~HomePage();
    QString currentProjectName() const;

signals:
    void currentProjectChanged(const QString &projectName);

private:
    void refreshCurrentProjectLabel();

    Ui::HomePage *ui;
};
