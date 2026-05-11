#include "deviceupgradepage.h"
#include "ui_deviceupgrade.h"

DeviceUpgradePage::DeviceUpgradePage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::DeviceUpgradePage)
{
    ui->setupUi(this);
    setTitleLabel(ui->titleLabel);
    applyDefaultPlaceholderStyle(ThemeManager::palette(ThemeKind::IndustrialBlue));
}

DeviceUpgradePage::~DeviceUpgradePage()
{
    delete ui;
}
