#include "terminalpage.h"
#include "ui_terminalpage.h"

TerminalPage::TerminalPage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::TerminalPage)
{
    ui->setupUi(this);
    applyDefaultPlaceholderStyle(ThemeManager::palette(ThemeKind::IndustrialBlue));
}

TerminalPage::~TerminalPage()
{
    delete ui;
}


