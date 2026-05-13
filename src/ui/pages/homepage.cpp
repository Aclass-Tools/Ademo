#include "homepage.h"
#include "ui_homepage.h"
#include "jsonpreviewparser.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QPushButton>

HomePage::HomePage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::HomePage)
{
    // 构建并挂载 Designer 里定义的首页控件树。
    ui->setupUi(this);
    // 首页按全局主题应用内容区配色，保持与主窗体风格一致。
    setStyleSheet(ThemeManager::contentPageStyle(ThemeManager::palette(ThemeKind::IndustrialBlue)));
    // 下拉框先放一个初始占位项，表示“当前未选择具体项目”。
    ui->homeProjectSelectorCombo->addItem(QStringLiteral("未选择项目"));
    ui->homeProjectSelectorCombo->setCurrentIndex(0);

    // “刷新”按钮：仅通知上层执行刷新流程（如联网拉取），不改当前项目显示。
    connect(ui->homeProjectLoadRefreshButton,
        &QPushButton::clicked, this, &HomePage::onLoadRefreshButtonClicked);

    // “导入项目”按钮：读取本地 JSON、填充下拉框，并更新当前项目显示。
    connect(ui->homeImportProjectButton,
        &QPushButton::clicked, this, &HomePage::onImportProjectButtonClicked);

    // 页面初始化时主动执行一次“导入项目逻辑”：
    // - 从本地固定目录读取 JSON
    // - 在右侧文本框展示格式化 JSON
    onImportProjectButtonClicked();
}

HomePage::~HomePage()
{
    delete ui;
}

void HomePage::onLoadRefreshButtonClicked()
{
    // “刷新”按钮仅负责通知上层，不改变当前项目状态。
    emit jsonRefreshRequested();
}

void HomePage::onImportProjectButtonClicked()
{
    // “导入项目”按钮点击后：仅在右侧文本框展示解析后的 JSON 内容。
    const QString filePath = resolveProjectJsonPath();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) 
    {
        ui->homeOverviewplainTextEdit->setPlainText(
            QStringLiteral("读取失败：无法打开文件\n%1").arg(filePath));
        emit jsonRefreshRequested();
        return;
    }

    // 读取一次完整字节流，仅用于预览解析与展示。
    const QByteArray payload = file.readAll();

    // 右侧文本框显示“解析后的 JSON（格式化后）”。
    const JsonPreviewParser::Result preview = JsonPreviewParser::parse(payload);
    if (preview.ok) {
        ui->homeOverviewplainTextEdit->setPlainText(preview.formattedText);
    } else {
        ui->homeOverviewplainTextEdit->setPlainText(
            QStringLiteral("JSON 解析失败：%1").arg(preview.errorText));
        emit jsonRefreshRequested();
        return;
    }

    // 从导入的 JSON 中提取当前项目名，并同步给上层 MainWindow 维护状态栏。
    const QString projectName = extractProjectNameFromPayload(payload);
    ui->homeCurrentProjectValueLabel->setText(
        projectName.isEmpty() ? QStringLiteral("未选择项目") : projectName);
    emit currentProjectChanged(projectName);

    // 保持原有行为：导入后仍通知上层执行统一刷新流程。
    emit jsonRefreshRequested();
}

void HomePage::refreshCurrentProjectLabel()
{
    // 读取当前项目名：为空代表尚未选择项目。
    const QString currentText = ui->homeProjectSelectorCombo->currentText().trimmed();

    // 页面内可视化标签显示：空值时给出占位文案，便于用户识别当前状态。
    ui->homeCurrentProjectValueLabel->setText(
        currentText.isEmpty() ? QStringLiteral("未选择项目") : currentText
    );

    // 对外广播首页当前项目变化，供 MainWindow 等上层同步状态栏或其它区域显示。
    emit currentProjectChanged(currentText);
}

QString HomePage::resolveProjectJsonPath() const
{
    // 使用固定绝对路径读取本地项目 JSON。后期引用再改
    return QStringLiteral("C:/Users/81966/Documents/AclassTool/data/projects/sample_project.json");
}
