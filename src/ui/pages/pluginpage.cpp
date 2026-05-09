#include "pluginpage.h"
#include "ui_pluginpage.h"

PluginPage::PluginPage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::PluginPage)
{
    ui->setupUi(this);
    setTitleLabel(ui->titleLabel);
    applyDefaultPlaceholderStyle(ThemeManager::palette(ThemeKind::IndustrialBlue));
}

PluginPage::~PluginPage()
{
    delete ui;
}
