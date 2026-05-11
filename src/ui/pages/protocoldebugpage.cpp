#include "protocoldebugpage.h"
#include "ui_protocoldebugpage.h"

ProtocolDebugPage::ProtocolDebugPage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::ProtocolDebugPage)
{
    ui->setupUi(this);
    applyDefaultPlaceholderStyle(ThemeManager::palette(ThemeKind::IndustrialBlue));
}

ProtocolDebugPage::~ProtocolDebugPage()
{
    delete ui;
}


