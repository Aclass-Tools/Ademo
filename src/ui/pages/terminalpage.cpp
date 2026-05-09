#include "terminalpage.h"
#include "ui_terminalpage.h"

TerminalPage::TerminalPage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::TerminalPage)
{
    ui->setupUi(this);
    setTitleLabel(ui->titleLabel);
    applyDefaultPlaceholderStyle(ThemeManager::palette(ThemeKind::IndustrialBlue));
}

TerminalPage::~TerminalPage()
{
    delete ui;
}
