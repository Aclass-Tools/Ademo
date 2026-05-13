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

signals:
    void currentProjectChanged(const QString &projectName);
    // 用户点击“刷新”后发出：请求上层重新拉取首页所需 JSON。
    void jsonRefreshRequested();

private:
    // void onProjectSelectionChanged(const QString &selectedProject);
    void onLoadRefreshButtonClicked();
    void onImportProjectButtonClicked();
    void refreshCurrentProjectLabel();
    QString resolveProjectJsonPath() const;

    Ui::HomePage *ui;
};
