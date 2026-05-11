#include "homepage.h"
#include "ui_homepage.h"

#include <QComboBox>
#include <QPushButton>

HomePage::HomePage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::HomePage)
{
    // 构建并挂载 Designer 里定义的首页控件树。
    ui->setupUi(this);
    // 首页按全局主题应用内容区配色，保持与主窗体风格一致。
    setStyleSheet(ThemeManager::contentPageStyle(ThemeManager::palette(ThemeKind::IndustrialBlue)));

    // 当下拉框项目切换时，刷新“当前项目”显示，并向外部同步变更事件。
    connect(ui->homeProjectSelectorCombo, &QComboBox::currentTextChanged, this, [this]() {
        refreshCurrentProjectLabel();
    });

    // 首页“刷新”按钮：仅发出刷新请求信号，由上层决定如何访问后端。
    connect(ui->homeProjectLoadRefreshButton, &QPushButton::clicked, this, [this]() {
        emit jsonRefreshRequested();
    });

    // 初始化时先执行一次，保证页面首次显示就有正确文本与状态。
    refreshCurrentProjectLabel();
}

HomePage::~HomePage()
{
    delete ui;
}

QString HomePage::currentProjectName() const
{
    // 统一从下拉框读取当前项目名，并去掉首尾空白，作为页面标准输出。
    return ui->homeProjectSelectorCombo->currentText().trimmed();
}

void HomePage::refreshCurrentProjectLabel()
{
    // 读取当前项目名：为空代表尚未选择项目。
    const QString currentText = currentProjectName();

    // 页面内可视化标签显示：空值时给出占位文案，便于用户识别当前状态。
    ui->homeCurrentProjectValueLabel->setText(
        currentText.isEmpty() ? QStringLiteral("未选择项目") : currentText
    );

    // 对外广播首页当前项目变化，供 MainWindow 等上层同步状态栏或其它区域显示。
    emit currentProjectChanged(currentText);
}
