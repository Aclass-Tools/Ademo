#include "ui/delegates/ComboBoxDelegate.h"
#include <QEvent>
#include <QMetaObject>
#include <QAbstractItemView>

ComboBoxDelegate::ComboBoxDelegate(QObject* parent)
	: QStyledItemDelegate(parent)
{
}

ComboBoxDelegate::~ComboBoxDelegate()
{
}

void ComboBoxDelegate::setOptions(const QStringList& options)
{
	m_options = options;
}

QWidget* ComboBoxDelegate::createEditor(QWidget*           parent, const QStyleOptionViewItem& option,
                                        const QModelIndex& index) const
{
	QComboBox* combo = new QComboBox(parent);
	combo->addItems(m_options);
	combo->setEditable(false);
	return combo;
}

bool ComboBoxDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option,
                                   const QModelIndex& index)
{
	if (event->type() == QEvent::MouseButtonDblClick)
	{
		// 双击事件，需要确保已经创建了编辑器
		// 这里可以调用默认实现来创建编辑器，或者自己处理
		return false; // 让默认编辑器创建过程继续
	}
	return QStyledItemDelegate::editorEvent(event, model, option, index);
}

void ComboBoxDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	QComboBox* combo = qobject_cast<QComboBox*>(editor);
	if (combo)
	{
		QString currentText = index.data(Qt::EditRole).toString();
		int     idx         = combo->findText(currentText, Qt::MatchExactly);
		if (idx >= 0)
		{
			combo->setCurrentIndex(idx);
		}
	}
}

void ComboBoxDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
	QComboBox* combo = qobject_cast<QComboBox*>(editor);
	if (combo)
	{
		model->setData(index, combo->currentText(), Qt::EditRole);
	}
}

void ComboBoxDelegate::updateEditorGeometry(QWidget*           editor, const QStyleOptionViewItem& option,
                                            const QModelIndex& index) const
{
	editor->setGeometry(option.rect);
	// 在位置确定后再显示下拉列表，这样位置会正确
	QComboBox* combo = qobject_cast<QComboBox*>(editor);
	if (combo && !combo->view()->isVisible())
	{
		// QMetaObject::invokeMethod(combo, "showPopup", Qt::QueuedConnection);
		combo->showPopup();
	}
}
