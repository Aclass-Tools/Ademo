#include "homepage.h"
#include "ui_homepage.h"

#include <QComboBox>

HomePage::HomePage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::HomePage)
{
    ui->setupUi(this);
    connect(ui->homeProjectSelectorCombo, &QComboBox::currentTextChanged, this, [this]() {
        refreshCurrentProjectLabel();
    });
    refreshCurrentProjectLabel();
}

HomePage::~HomePage()
{
    delete ui;
}

QString HomePage::currentProjectName() const
{
    return ui->homeProjectSelectorCombo->currentText().trimmed();
}

void HomePage::refreshCurrentProjectLabel()
{
    const QString currentText = currentProjectName();
    ui->homeCurrentProjectValueLabel->setText(
        currentText.isEmpty() ? QStringLiteral("未选择项目") : currentText
    );
    emit currentProjectChanged(currentText);
}
