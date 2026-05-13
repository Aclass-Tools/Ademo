// 首页页面（HomePage）
// ----------------------
// 首页占位页组件。

#pragma once

#include "placeholderpagebase.h"
#include "jsonpreviewparser.h"

#include <memory>
#include <QSet>
#include <QString>
#include <QUrl>

struct ProjectSummaryContext;

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
    void onPageActivated() override;

signals:
    // 统一状态汇总更新信号：HomePage 修改共享上下文后发出，通知外层刷新显示。
    void projectSummaryChanged();
    // 用户点击“刷新”后发出：请求上层重新拉取首页所需 JSON。
    void jsonRefreshRequested();

public:
    // 注入可写项目摘要上下文。
    // 约束：仅 HomePage 持有可写指针，其他页面通过基类只读访问。
    void setWritableProjectContext(const std::shared_ptr<ProjectSummaryContext> &projectContext);

private:
    // void onProjectSelectionChanged(const QString &selectedProject);
    void onLoadRefreshButtonClicked();
    void onImportProjectButtonClicked();
    void onOverviewAnchorClicked(const QUrl &url);
    void refreshCurrentProjectLabel();
    QString resolveProjectJsonPath() const;

    Ui::HomePage *ui;
    // 首页持有一个 JsonPreviewParser 成员，内部维护 payload 与解析状态。
    JsonPreviewParser m_jsonPreviewParser;
    // HomePage 专属可写共享上下文：导入成功后写入项目摘要四个字段。
    std::shared_ptr<ProjectSummaryContext> m_writableProjectContext;
    QString m_lastImportedProjectName;
    QSet<QString> m_expandedNodeIds;
};
