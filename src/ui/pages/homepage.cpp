#include "homepage.h"
#include "ui_homepage.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QPushButton>
#include <QTextBrowser>
#include <QUrl>

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

    // “刷新”按钮：从本地 JSON 重新加载项目名，并回填下拉框。
    // 该按钮不负责右侧 JSON 预览展示，预览由“导入项目”按钮负责。
    connect(ui->homeProjectLoadRefreshButton,
        &QPushButton::clicked, this, &HomePage::onLoadRefreshButtonClicked);

    // “导入项目”按钮：读取本地 JSON，显示格式化文本，并同步当前项目名。
    connect(ui->homeImportProjectButton,
        &QPushButton::clicked, this, &HomePage::onImportProjectButtonClicked);
    connect(ui->homeOverviewplainTextEdit, &QTextBrowser::anchorClicked,
        this, &HomePage::onOverviewAnchorClicked);

    // 页面初始化时主动执行一次“刷新项目内容”：
    // - 从本地固定目录读取 JSON
    // - 在右侧文本框展示格式化 JSON
    onLoadRefreshButtonClicked();
}

HomePage::~HomePage()
{
    delete ui;
}

void HomePage::onLoadRefreshButtonClicked()
{
    // 步骤 1：读取本地 JSON 文件内容。
    const QString filePath = resolveProjectJsonPath();
    if (!m_jsonPreviewParser.load(filePath)) {
        // 不改动当前下拉框内容，仅向上层发出刷新事件占位。
        emit jsonRefreshRequested();
        return;
    }

    // 步骤 2：通过模型层解析器提取“项目显示名列表”。
    // 约定解析逻辑在 JsonPreviewParser 内部，不在页面层写 QJson 细节。
    QStringList names = m_jsonPreviewParser.projectNames();

    // 步骤 3：在首位补一个固定占位项，允许用户回到“未选择项目”状态。
    const QString kUnselectedProject = QStringLiteral("未选择项目");
    if (!names.contains(kUnselectedProject)) {
        names.prepend(kUnselectedProject);
    }

    // 步骤 4：刷新下拉框数据并尽量恢复用户旧选择。
    // 刷新期间屏蔽信号，避免 clear/addItems/setCurrentIndex 触发多次联动。
    const QString previous = ui->homeProjectSelectorCombo->currentText();
    ui->homeProjectSelectorCombo->blockSignals(true);
    ui->homeProjectSelectorCombo->clear();
    ui->homeProjectSelectorCombo->addItems(names);

    const int restoreIndex = ui->homeProjectSelectorCombo->findText(previous);
    ui->homeProjectSelectorCombo->setCurrentIndex(restoreIndex >= 0 ? restoreIndex : 0);
    ui->homeProjectSelectorCombo->blockSignals(false);

    // 步骤 5：通知上层“已完成一次本地刷新动作”。
    emit jsonRefreshRequested();
}

void HomePage::onImportProjectButtonClicked()
{
    // 根据下拉框当前选中的项目名称，仅展示该项目对应的 JSON 内容。
    const QString selectedProject = ui->homeProjectSelectorCombo->currentText().trimmed();
    if (selectedProject != m_lastImportedProjectName) {
        m_lastImportedProjectName = selectedProject;
        m_expandedNodeIds.clear();
    }

    // 默认展开根节点，首次导入时即可看到第一层内容。
    m_expandedNodeIds.insert(QStringLiteral("cm9vdA"));

    const JsonPreviewParser::ProjectPreviewResult previewResult =
        m_jsonPreviewParser.formattedTextForProject(selectedProject, m_expandedNodeIds);
    if (!previewResult.ok || previewResult.html.isEmpty()) {
        ui->homeOverviewplainTextEdit->setHtml(
            QStringLiteral("未能展示该项目内容，请先点击“刷新”并确认已选择有效项目。"));
        emit jsonRefreshRequested();
        return;
    }
    ui->homeOverviewplainTextEdit->setHtml(previewResult.html);
    emit projectDbConfigChanged(previewResult.localDbAddress, previewResult.remoteDbAddress);

    // 当前项目标签和对外信号同步为“用户当前选择的项目”。
    const QString projectName = selectedProject;
    ui->homeCurrentProjectValueLabel->setText(
        projectName.isEmpty() ? QStringLiteral("未选择项目") : projectName);
    emit currentProjectChanged(projectName);

    // 保持原有行为：导入后仍通知上层执行统一刷新流程。
    emit jsonRefreshRequested();
}

void HomePage::onOverviewAnchorClicked(const QUrl &url)
{
    // 仅处理解析器输出的 toggle 链接。
    const QString link = url.toString();
    if (!link.startsWith(QStringLiteral("toggle:"))) {
        return;
    }

    const QString nodeId = link.mid(QStringLiteral("toggle:").size());
    if (nodeId.isEmpty()) {
        return;
    }

    // 点击后切换当前节点展开状态。
    if (m_expandedNodeIds.contains(nodeId)) {
        m_expandedNodeIds.remove(nodeId);
    } else {
        m_expandedNodeIds.insert(nodeId);
    }

    if (m_lastImportedProjectName.isEmpty()) {
        return;
    }

    // 基于当前项目和最新展开集合重绘 HTML。
    const JsonPreviewParser::ProjectPreviewResult previewResult =
        m_jsonPreviewParser.formattedTextForProject(m_lastImportedProjectName, m_expandedNodeIds);
    if (previewResult.ok && !previewResult.html.isEmpty()) {
        ui->homeOverviewplainTextEdit->setHtml(previewResult.html);
        emit projectDbConfigChanged(previewResult.localDbAddress, previewResult.remoteDbAddress);
    }
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

