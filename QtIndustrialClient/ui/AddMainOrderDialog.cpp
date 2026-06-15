#include "ui/AddMainOrderDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QStackedWidget>

AddMainOrderDialog::AddMainOrderDialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(QStringLiteral("添加主指令"));
	setMinimumSize(400, 200);
	initUI();
}

AddMainOrderDialog::~AddMainOrderDialog()
{
}

void AddMainOrderDialog::initUI()
{
	auto* mainLayout = new QVBoxLayout(this);

	// 表单布局
	auto* formLayout = new QFormLayout();

	// 主指令名称
	m_mainOrderNameEdit = new QLineEdit();
	m_mainOrderNameEdit->setPlaceholderText(QStringLiteral("请输入主指令名称"));
	formLayout->addRow(QStringLiteral("主指令名称:"), m_mainOrderNameEdit);

	m_mainOrderNumEdit = new QLineEdit();
	m_mainOrderNumEdit->setPlaceholderText(QStringLiteral("请输入主指令序号"));
	formLayout->addRow(QStringLiteral("主指令序号"), m_mainOrderNumEdit);

	// 协议类型
	m_protocolTypeComboBox = new QComboBox();
	m_protocolTypeComboBox->addItem(QStringLiteral("A0"));
	m_protocolTypeComboBox->addItem(QStringLiteral("A3"));
	formLayout->addRow(QStringLiteral("协议类型:"), m_protocolTypeComboBox);

	// 备注 - A3使用QComboBox，A0使用QLineEdit
	m_remarkLabel = new QLabel(QStringLiteral("备注:"));
	formLayout->addRow(m_remarkLabel);

	// A3备注下拉框
	m_remarkComboBox = new QComboBox();
	m_remarkComboBox->addItem(QStringLiteral("绘图"));
	m_remarkComboBox->addItem(QStringLiteral("不绘图"));
	m_remarkComboBox->setCurrentIndex(0); // 默认"绘图"
	formLayout->addRow(QStringLiteral("  "), m_remarkComboBox);

	// A0备注文本输入
	m_remarkEdit = new QLineEdit();
	m_remarkEdit->setPlaceholderText(QStringLiteral("请输入备注"));
	formLayout->addRow(QStringLiteral("  "), m_remarkEdit);

	// 连接协议类型切换信号
	connect(m_protocolTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
	        this, &AddMainOrderDialog::onProtocolTypeChanged);

	// 初始设置
	onProtocolTypeChanged(0);

	mainLayout->addLayout(formLayout);

	mainLayout->addStretch();

	// 按钮区域
	auto* buttonLayout = new QHBoxLayout();
	buttonLayout->addStretch();

	m_cancelButton = new QPushButton(QStringLiteral("取消"));
	connect(m_cancelButton, &QPushButton::clicked, this, &AddMainOrderDialog::reject);
	buttonLayout->addWidget(m_cancelButton);

	m_okButton = new QPushButton(QStringLiteral("确定"));
	m_okButton->setStyleSheet(
		QStringLiteral("background: #4CAF50; color: white; padding: 8px 16px; border: none; border-radius: 4px;"));
	connect(m_okButton, &QPushButton::clicked, this, [this]()
	{
		if (m_mainOrderNameEdit->text().trimmed().isEmpty())
		{
			QMessageBox::warning(this, QStringLiteral("输入错误"), QStringLiteral("主指令名称不能为空"));
			return;
		}

		if (m_mainOrderNumEdit->text().trimmed().isEmpty())
		{
			QMessageBox::warning(this, QStringLiteral("输入错误"), QStringLiteral("主指令序号不能为空"));
			return;
		}

		bool ok           = false;
		int  mainOrderNum = m_mainOrderNumEdit->text().toInt(&ok);
		if (!ok || mainOrderNum <= 1)
		{
			QMessageBox::warning(this, QStringLiteral("输入错误"), QStringLiteral("主指令序号必须是大于1的整数"));
			return;
		}
		accept();
	});
	buttonLayout->addWidget(m_okButton);

	mainLayout->addLayout(buttonLayout);
}

QString AddMainOrderDialog::getMainOrderName() const
{
	return m_mainOrderNameEdit->text().trimmed();
}

QString AddMainOrderDialog::getMainOrderNum() const
{
	return m_mainOrderNumEdit->text().trimmed();
}

QString AddMainOrderDialog::getProtocolType() const
{
	return m_protocolTypeComboBox->currentText();
}

void AddMainOrderDialog::onProtocolTypeChanged(int index)
{
	QString protocolType = m_protocolTypeComboBox->itemText(index);
	if (protocolType == QStringLiteral("A3"))
	{
		// A3协议: 显示QComboBox，隐藏QLineEdit
		m_remarkComboBox->setVisible(true);
		m_remarkEdit->setVisible(false);
		// 默认选择"绘图"
		if (m_remarkComboBox->currentIndex() < 0)
		{
			m_remarkComboBox->setCurrentIndex(0);
		}
	}
	else
	{
		// A0协议: 显示QLineEdit，隐藏QComboBox
		m_remarkComboBox->setVisible(false);
		m_remarkEdit->setVisible(true);
	}
}

QString AddMainOrderDialog::getRemark() const
{
	QString protocolType = m_protocolTypeComboBox->currentText();
	if (protocolType == QStringLiteral("A3"))
	{
		return m_remarkComboBox->currentText();
	}
	else
	{
		return m_remarkEdit->text().trimmed();
	}
}
