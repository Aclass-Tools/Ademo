#include "homepage.h"
#include "ui_homepage.h"

HomePage::HomePage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::HomePage)
{
    ui->setupUi(this);
    // 将页面标题控件登记给基类，后续统一通过 setTitle() 修改。
    setTitleLabel(ui->titleLabel);
    applyDefaultPlaceholderStyle(ThemeManager::palette(ThemeKind::IndustrialBlue));
}

HomePage::~HomePage()
{
    delete ui;
}
