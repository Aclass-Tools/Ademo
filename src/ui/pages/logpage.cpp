#include "logpage.h"
#include "ui_logpage.h"

LogPage::LogPage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::LogPage)
{
    ui->setupUi(this);
    applyDefaultPlaceholderStyle(ThemeManager::palette(ThemeKind::IndustrialBlue));
}

LogPage::~LogPage()
{
    delete ui;
}


