#include "pluginpage.h"
#include "ui_pluginpage.h"

PluginPage::PluginPage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::PluginPage)
{
    ui->setupUi(this);
    applyDefaultPlaceholderStyle(ThemeManager::palette(ThemeKind::IndustrialBlue));
}

PluginPage::~PluginPage()
{
    delete ui;
}


