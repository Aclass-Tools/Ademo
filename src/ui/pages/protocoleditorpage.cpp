#include "protocoleditorpage.h"
#include "ui_protocoleditorpage.h"

ProtocolEditorPage::ProtocolEditorPage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::ProtocolEditorPage)
{
    ui->setupUi(this);
    applyDefaultPlaceholderStyle(ThemeManager::palette(ThemeKind::IndustrialBlue));
}

ProtocolEditorPage::~ProtocolEditorPage()
{
    delete ui;
}


