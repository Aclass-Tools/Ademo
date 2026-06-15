#include "ui/delegates/RemarkComboDelegate.h"
#include "services/ProtocolParser.h"
#include <QEvent>
#include <QAbstractItemView>

RemarkComboDelegate::RemarkComboDelegate(QObject* parent)
	: QStyledItemDelegate(parent)
{
}

RemarkComboDelegate::~RemarkComboDelegate()
{
}

QWidget* RemarkComboDelegate::createEditor(QWidget*           parent, const QStyleOptionViewItem& option,
                                           const QModelIndex& index) const
{
	quint8 protocolType = getProtocolType(index);

	if (protocolType == PROTOCOL_A3)
	{
		// A3协议: 使用QComboBox
		QComboBox* combo = new QComboBox(parent);
		combo->addItem(QStringLiteral("绘图"));
		combo->addItem(QStringLiteral("不绘图"));
		combo->setEditable(false);
		return combo;
	}
	else
	{
		// A0协议: 使用QLineEdit
		QLineEdit* lineEdit = new QLineEdit(parent);
		return lineEdit;
	}
}

void RemarkComboDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	quint8  protocolType = getProtocolType(index);
	QString currentText  = index.data(Qt::EditRole).toString();

	if (protocolType == PROTOCOL_A3)
	{
		QComboBox* combo = qobject_cast<QComboBox*>(editor);
		if (combo)
		{
			int idx = combo->findText(currentText, Qt::MatchExactly);
			if (idx >= 0)
			{
				combo->setCurrentIndex(idx);
			}
			else
			{
				// 默认选中"绘图"
				combo->setCurrentIndex(0);
			}
		}
	}
	else
	{
		QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
		if (lineEdit)
		{
			lineEdit->setText(currentText);
		}
	}
}

void RemarkComboDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
	quint8 protocolType = getProtocolType(index);

	if (protocolType == PROTOCOL_A3)
	{
		QComboBox* combo = qobject_cast<QComboBox*>(editor);
		if (combo)
		{
			model->setData(index, combo->currentText(), Qt::EditRole);
		}
	}
	else
	{
		QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
		if (lineEdit)
		{
			model->setData(index, lineEdit->text(), Qt::EditRole);
		}
	}
}

void RemarkComboDelegate::updateEditorGeometry(QWidget*           editor, const QStyleOptionViewItem& option,
                                               const QModelIndex& index) const
{
	editor->setGeometry(option.rect);

	quint8 protocolType = getProtocolType(index);
	if (protocolType == PROTOCOL_A3)
	{
		QComboBox* combo = qobject_cast<QComboBox*>(editor);
		if (combo && !combo->view()->isVisible())
		{
			combo->showPopup();
		}
	}
}

quint8 RemarkComboDelegate::getProtocolType(const QModelIndex& index)
{
	QVariant protocolTypeData = index.data(ProtocolTypeRole);
	if (protocolTypeData.isValid())
	{
		return static_cast<quint8>(protocolTypeData.toUInt());
	}
	return PROTOCOL_A0; // 默认A0
}
