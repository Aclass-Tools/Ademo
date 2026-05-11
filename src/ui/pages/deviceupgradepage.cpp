#include "deviceupgradepage.h"
#include "ui_deviceupgrade.h"

DeviceUpgradePage::DeviceUpgradePage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::DeviceUpgradePage)
{
    ui->setupUi(this);
    applyDefaultPlaceholderStyle(ThemeManager::palette(ThemeKind::IndustrialBlue));
}

DeviceUpgradePage::~DeviceUpgradePage()
{
    delete ui;
}


