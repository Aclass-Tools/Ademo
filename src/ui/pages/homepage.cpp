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

void HomePage::refreshCurrentProjectLabel()
{
    const QString currentText = ui->homeProjectSelectorCombo->currentText().trimmed();
    ui->homeCurrentProjectValueLabel->setText(
        currentText.isEmpty() ? QStringLiteral("未选择项目") : currentText
    );
}
