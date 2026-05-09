#include "logpage.h"
#include "ui_logpage.h"

LogPage::LogPage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::LogPage)
{
    ui->setupUi(this);
    setTitleLabel(ui->titleLabel);
    applyDefaultPlaceholderStyle(ThemeManager::palette(ThemeKind::IndustrialBlue));
}

LogPage::~LogPage()
{
    delete ui;
}
