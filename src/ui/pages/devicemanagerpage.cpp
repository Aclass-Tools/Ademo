#include "devicemanagerpage.h"
#include "ui_devicemanagerpage.h"

DeviceManagerPage::DeviceManagerPage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::DeviceManagerPage)
{
    ui->setupUi(this);
    setTitleLabel(ui->titleLabel);
    applyDefaultPlaceholderStyle(ThemeManager::palette(ThemeKind::IndustrialBlue));
}

DeviceManagerPage::~DeviceManagerPage()
{
    delete ui;
}
