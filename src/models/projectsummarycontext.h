// 项目摘要上下文（ProjectSummaryContext）
// ---------------------------------------
// 用于在主窗口与各业务页面之间共享“当前项目摘要信息”。
//
// 设计约束：
// - HomePage 负责写入（导入项目后更新）。
// - 其他页面只读访问，避免多处修改导致状态不一致。

#pragma once

#include <QString>

struct ProjectSummaryContext
{
    QString projectName;
    QString localDbAddress;
    QString remoteDbAddress;
    QString configVersion;
};

