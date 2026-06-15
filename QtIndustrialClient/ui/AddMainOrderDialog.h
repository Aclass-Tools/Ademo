#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>

class AddMainOrderDialog : public QDialog
{
	Q_OBJECT

public:
	explicit AddMainOrderDialog(QWidget* parent = nullptr);
	~AddMainOrderDialog() override;

	QString getMainOrderName() const;
	QString getMainOrderNum() const;
	QString getProtocolType() const;
	QString getRemark() const;

private:
	void initUI();
	void onProtocolTypeChanged(int index);

	QLineEdit*   m_mainOrderNameEdit    = nullptr;
	QLineEdit*   m_mainOrderNumEdit     = nullptr;
	QComboBox*   m_protocolTypeComboBox = nullptr;
	QComboBox*   m_remarkComboBox       = nullptr;  // A3使用
	QLineEdit*   m_remarkEdit           = nullptr;  // A0使用
	QLabel*      m_remarkLabel          = nullptr;
	QPushButton* m_okButton             = nullptr;
	QPushButton* m_cancelButton         = nullptr;
};
