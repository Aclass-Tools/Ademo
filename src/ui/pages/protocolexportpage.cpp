#include "protocolexportpage.h"
#include "ui_protocolexport.h"

ProtocolExportPage::ProtocolExportPage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::ProtocolExportPage)
{
    ui->setupUi(this);
    setTitleLabel(ui->titleLabel);
    applyDefaultPlaceholderStyle(ThemeManager::palette(ThemeKind::IndustrialBlue));
}

ProtocolExportPage::~ProtocolExportPage()
{
    delete ui;
}
