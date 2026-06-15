#include "ui/CommandOperationPage.h"
#include "services/ProtocolParser.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QHeaderView>
#include "ui/PlotDialog.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QLineEdit>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStyledItemDelegate>
#include <QtEndian>
#include <cstring>
#include <limits>
#include <utility>
#include "ui_CommandOperationPage.h"
#include "components/qvalidator/RyQValidator.h"
#include "ui/delegates/ComboBoxDelegate.h"

namespace
{
	constexpr int kRoleDefaultValue = Qt::UserRole + 1;
	constexpr int kRoleMinValue     = Qt::UserRole + 2;
	constexpr int kRoleMaxValue     = Qt::UserRole + 3;
	constexpr int kRoleDataType     = Qt::UserRole + 4;

	bool isSignedIntegerType(const QString& type)
	{
		return type == QStringLiteral("int8_t") || type == QStringLiteral("int16_t") ||
				type == QStringLiteral("int32_t") || type == QStringLiteral("int64_t");
	}

	bool isUnsignedIntegerType(const QString& type)
	{
		return type == QStringLiteral("uint8_t") || type == QStringLiteral("uint16_t") ||
				type == QStringLiteral("uint32_t") || type == QStringLiteral("uint64_t");
	}

	bool isFloatingType(const QString& type)
	{
		return type == QStringLiteral("float") || type == QStringLiteral("double");
	}

	class ValueEditDelegate final : public QStyledItemDelegate
	{
	public:
		explicit ValueEditDelegate(QObject* parent = nullptr)
			: QStyledItemDelegate(parent)
		{
		}

		QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const override
		{
			auto*         editor       = new QLineEdit(parent);
			const QString defaultValue = index.data(kRoleDefaultValue).toString().trimmed();
			const QString minValue     = index.data(kRoleMinValue).toString().trimmed();
			const QString maxValue     = index.data(kRoleMaxValue).toString().trimmed();
			const QString type         = index.data(kRoleDataType).toString().trimmed().toLower();
			QStringList   placeholders;
			if (!defaultValue.isEmpty())
			{
				placeholders << QStringLiteral("默认:%1").arg(defaultValue);
			}
			if (!minValue.isEmpty() || !maxValue.isEmpty())
			{
				placeholders << QStringLiteral("范围:%1~%2").arg(minValue.isEmpty() ? QStringLiteral("-") : minValue,
				                                               maxValue.isEmpty() ? QStringLiteral("-") : maxValue);
			}
			editor->setPlaceholderText(placeholders.join(QStringLiteral("  ")));

			if (isUnsignedIntegerType(type))
			{
				qulonglong low = 0;
				qulonglong top = std::numeric_limits<quint64>::max();
				if (type == QStringLiteral("uint8_t"))
				{
					top = std::numeric_limits<quint8>::max();
				}
				else if (type == QStringLiteral("uint16_t"))
				{
					top = std::numeric_limits<quint16>::max();
				}
				else if (type == QStringLiteral("uint32_t"))
				{
					top = std::numeric_limits<quint32>::max();
				}

				if (!minValue.isEmpty())
				{
					bool             minOk         = false;
					const qulonglong configuredMin = minValue.toULongLong(&minOk);
					if (minOk)
					{
						low = qMax(low, configuredMin);
					}
				}
				if (!maxValue.isEmpty())
				{
					bool             maxOk         = false;
					const qulonglong configuredMax = maxValue.toULongLong(&maxOk);
					if (maxOk)
					{
						top = qMin(top, configuredMax);
					}
				}

				auto* validator = new RyQIntValidator(static_cast<int>(low), static_cast<int>(top), editor);
				validator->setValueCanIsNull(true);
				editor->setValidator(validator);
			}
			else if (isSignedIntegerType(type))
			{
				qlonglong low = std::numeric_limits<qint64>::min();
				qlonglong top = std::numeric_limits<qint64>::max();
				if (type == QStringLiteral("int8_t"))
				{
					low = std::numeric_limits<qint8>::min();
					top = std::numeric_limits<qint8>::max();
				}
				else if (type == QStringLiteral("int16_t"))
				{
					low = std::numeric_limits<qint16>::min();
					top = std::numeric_limits<qint16>::max();
				}
				else if (type == QStringLiteral("int32_t"))
				{
					low = std::numeric_limits<qint32>::min();
					top = std::numeric_limits<qint32>::max();
				}

				if (!minValue.isEmpty())
				{
					bool            minOk         = false;
					const qlonglong configuredMin = minValue.toLongLong(&minOk);
					if (minOk)
					{
						low = qMax(low, configuredMin);
					}
				}

				if (!maxValue.isEmpty())
				{
					bool            maxOk         = false;
					const qlonglong configuredMax = maxValue.toLongLong(&maxOk);
					if (maxOk)
					{
						top = qMin(top, configuredMax);
					}
				}

				auto* validator = new RyQIntValidator(static_cast<int>(low), static_cast<int>(top), editor);
				validator->setValueCanIsNull(true);
				editor->setValidator(validator);
			}
			else if (isFloatingType(type))
			{
				double low = std::numeric_limits<double>::lowest();
				double top = std::numeric_limits<double>::max();
				if (!minValue.isEmpty())
				{
					bool         minOk         = false;
					const double configuredMin = minValue.toDouble(&minOk);
					if (minOk)
					{
						low = qMax(low, configuredMin);
					}
				}
				if (!maxValue.isEmpty())
				{
					bool         maxOk         = false;
					const double configuredMax = maxValue.toDouble(&maxOk);
					if (maxOk)
					{
						top = qMin(top, configuredMax);
					}
				}

				auto* validator = new RyQDoubleValidator(low, top, 15, false, editor);
				validator->setValueCanIsNull(true);
				editor->setValidator(validator);
			}
			return editor;
		}

		void setEditorData(QWidget* editor, const QModelIndex& index) const override
		{
			if (auto* lineEdit = qobject_cast<QLineEdit*>(editor))
			{
				lineEdit->setText(index.data(Qt::EditRole).toString());
				lineEdit->selectAll();
			}
		}

		void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
		{
			if (auto* lineEdit = qobject_cast<QLineEdit*>(editor))
			{
				const QString text     = lineEdit->text().trimmed();
				const QString type     = index.data(kRoleDataType).toString().trimmed().toLower();
				const QString minText  = index.data(kRoleMinValue).toString().trimmed();
				const QString maxText  = index.data(kRoleMaxValue).toString().trimmed();
				const bool    hasLimit = !minText.isEmpty() || !maxText.isEmpty();

				if (type == QStringLiteral("string"))
				{
					model->setData(index, text, Qt::EditRole);
					return;
				}
				if (text.isEmpty())
				{
					// 没有配置上下限时，空值失焦自动回填 0
					if (!hasLimit)
					{
						model->setData(index, QStringLiteral("0"), Qt::EditRole);
					}
					else
					{
						model->setData(index, text, Qt::EditRole);
					}
					return;
				}

				if (isSignedIntegerType(type))
				{
					bool            ok    = false;
					const qlonglong value = text.toLongLong(&ok);
					if (!ok)
					{
						return;
					}
					qlonglong typeMin = std::numeric_limits<qint64>::min();
					qlonglong typeMax = std::numeric_limits<qint64>::max();
					if (type == QStringLiteral("int8_t"))
					{
						typeMin = std::numeric_limits<qint8>::min();
						typeMax = std::numeric_limits<qint8>::max();
					}
					else if (type == QStringLiteral("int16_t"))
					{
						typeMin = std::numeric_limits<qint16>::min();
						typeMax = std::numeric_limits<qint16>::max();
					}
					else if (type == QStringLiteral("int32_t"))
					{
						typeMin = std::numeric_limits<qint32>::min();
						typeMax = std::numeric_limits<qint32>::max();
					}

					if (value < typeMin || value > typeMax)
					{
						return;
					}
					if (!minText.isEmpty())
					{
						bool            minOk    = false;
						const qlonglong minValue = minText.toLongLong(&minOk);
						if (minOk && value < minValue)
						{
							return;
						}
					}
					if (!maxText.isEmpty())
					{
						bool            maxOk    = false;
						const qlonglong maxValue = maxText.toLongLong(&maxOk);
						if (maxOk && value > maxValue)
						{
							return;
						}
					}
					model->setData(index, QString::number(value), Qt::EditRole);
					return;
				}

				if (isUnsignedIntegerType(type))
				{
					bool             ok    = false;
					const qulonglong value = text.toULongLong(&ok);
					if (!ok)
					{
						return;
					}
					qulonglong typeMax = std::numeric_limits<quint64>::max();
					if (type == QStringLiteral("uint8_t"))
					{
						typeMax = std::numeric_limits<quint8>::max();
					}
					else if (type == QStringLiteral("uint16_t"))
					{
						typeMax = std::numeric_limits<quint16>::max();
					}
					else if (type == QStringLiteral("uint32_t"))
					{
						typeMax = std::numeric_limits<quint32>::max();
					}
					if (value > typeMax)
					{
						return;
					}
					if (!minText.isEmpty())
					{
						bool             minOk    = false;
						const qulonglong minValue = minText.toULongLong(&minOk);
						if (minOk && value < minValue)
						{
							return;
						}
					}
					if (!maxText.isEmpty())
					{
						bool             maxOk    = false;
						const qulonglong maxValue = maxText.toULongLong(&maxOk);
						if (maxOk && value > maxValue)
						{
							return;
						}
					}
					model->setData(index, QString::number(value), Qt::EditRole);
					return;
				}

				if (isFloatingType(type))
				{
					bool         ok    = false;
					const double value = text.toDouble(&ok);
					if (!ok)
					{
						return;
					}
					if (!minText.isEmpty())
					{
						bool         minOk    = false;
						const double minValue = minText.toDouble(&minOk);
						if (minOk && value < minValue)
						{
							return;
						}
					}
					if (!maxText.isEmpty())
					{
						bool         maxOk    = false;
						const double maxValue = maxText.toDouble(&maxOk);
						if (maxOk && value > maxValue)
						{
							return;
						}
					}
					model->setData(index, text, Qt::EditRole);
					return;
				}

				model->setData(index, text, Qt::EditRole);
			}
		}
	};
}

CommandOperationPage::CommandOperationPage(QWidget* parent)
	: QWidget(parent)
	  , m_ui(std::make_unique<Ui::CommandOperationPage>())
{
	m_ui->setupUi(this);
	m_groupTabs = m_ui->groupTabs;
	m_ui->titleLabel->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: 700;"));
	m_groupTabs->setDocumentMode(true);
	m_groupTabs->setStyleSheet(QStringLiteral(
		"QTabBar::tab { min-width: 92px; padding: 6px 14px; background: #f4f7fb; border: 1px solid #d8e4ec; }"
		"QTabBar::tab:selected { background: #ffffff; }"
		"QTabWidget::pane { border: 1px solid #d8e4ec; background: white; }"));

	// 初始化数据类型 delegate
	m_dataTypeDelegate = new ComboBoxDelegate(this);
	m_dataTypeDelegate->setOptions({
		QStringLiteral("uint8_t"),
		QStringLiteral("int8_t"),
		QStringLiteral("uint16_t"),
		QStringLiteral("int16_t"),
		QStringLiteral("uint32_t"),
		QStringLiteral("int32_t"),
		QStringLiteral("float"),
		QStringLiteral("double"),
		QStringLiteral("int64_t"),
		QStringLiteral("uint64_t"),
		QStringLiteral("string")
	});
	m_valueEditDelegate = new ValueEditDelegate(this);
}

CommandOperationPage::~CommandOperationPage()
{
	// 清理所有打开的绘图对话框，防止程序退出时崩溃
	for (auto it = m_activePlotDialogs.begin(); it != m_activePlotDialogs.end(); ++it)
	{
		PlotDialog* dialog = it.value();
		if (dialog)
		{
			// 断开连接，防止访问无效内存
			dialog->disconnect(this);
			// 如果对话框还显示，先关闭它
			if (dialog->isVisible())
			{
				dialog->close();
			}
			// 删除对话框对象
			delete dialog;
		}
	}
	m_activePlotDialogs.clear();
	m_startTimestamps.clear();
}

void CommandOperationPage::setRecords(const QVector<ParameterRecord>& records)
{
	m_records = records;
	m_valueBindingMap.clear();
	m_groupTabs->clear();

	QMap<quint8, QVector<ParameterRecord>> protocolGrouped;
	for (const ParameterRecord& record : records)
	{
		protocolGrouped[record.protocolType].push_back(record);
	}

	for (auto protocolIt = protocolGrouped.cbegin(); protocolIt != protocolGrouped.cend(); ++protocolIt)
	{
		QMap<int, QVector<ParameterRecord>> grouped;
		QMap<int, QString>                  groupDes;
		for (const ParameterRecord& record : protocolIt.value())
		{
			grouped[record.groupId].push_back(record);
			groupDes[record.groupId] = record.groupName;
		}

		for (auto it = grouped.cbegin(); it != grouped.cend(); ++it)
		{
			QVector<ParameterRecord> sortedRecords = it.value();
			std::sort(sortedRecords.begin(), sortedRecords.end(),
			          [](const ParameterRecord& left, const ParameterRecord& right)
			          {
				          if (left.commandId != right.commandId)
				          {
					          return left.commandId < right.commandId;
				          }
				          return left.readIndex < right.readIndex;
			          });

			const QString protocolText =
					QStringLiteral("%1").arg(protocolIt.key(), 2, 16, QChar('0')).toUpper();
			const QString tabTitle = QStringLiteral("%1-%2").arg(protocolText, groupDes.value(it.key()));
			m_groupTabs->addTab(createTable(sortedRecords), tabTitle);
		}
	}
}

void CommandOperationPage::setActionHandler(std::function<void(const ParameterRecord&, ParameterAction)> handler)
{
	m_actionHandler = std::move(handler);
}

bool CommandOperationPage::isA3Protocol(quint8 protocolType)
{
	return protocolType == PROTOCOL_A3;
}

QTableWidget* CommandOperationPage::createTable(const QVector<ParameterRecord>& records)
{
	auto* table = new QTableWidget(this);
	table->setColumnCount(7);
	table->setHorizontalHeaderLabels({
		QStringLiteral("名称"),
		QStringLiteral("类型"),
		QStringLiteral("说明"),
		QStringLiteral("值"),
		QStringLiteral("单位"),
		QStringLiteral("操作"),
		QStringLiteral("更新时间")
	});
	table->setRowCount(records.size());
	table->verticalHeader()->setVisible(false);
	table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	table->horizontalHeader()->setSectionsMovable(true);
	table->setAlternatingRowColors(true);
	table->setEditTriggers(QAbstractItemView::DoubleClicked);
	table->setStyleSheet(QStringLiteral(
		"QTableWidget { border: none; background: white; gridline-color: #d9e2ec; }"
		"QHeaderView::section { background: #a8d3e3; color: #0f172a; padding: 6px; border: 1px solid #7faec0; }"));

	// 为值列设置双击编辑 delegate（占位提示显示默认值和上下限）
	table->setItemDelegateForColumn(3, m_valueEditDelegate);

	const QString now  = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
	const bool    isA3 = !records.isEmpty() && isA3Protocol(records.first().protocolType);

	for (int row = 0; row < records.size(); ++row)
	{
		const ParameterRecord& record = records.at(row);

		auto* nameItem  = new QTableWidgetItem(record.parameterName);
		auto* typeItem  = new QTableWidgetItem(record.dataType);
		auto* descItem  = new QTableWidgetItem(record.description);
		auto* valueItem = new QTableWidgetItem(QString());
		auto* unitItem  = new QTableWidgetItem(record.unit);
		auto* timeItem  = new QTableWidgetItem(now);

		// 默认全部不可编辑
		nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
		typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
		descItem->setFlags(descItem->flags() & ~Qt::ItemIsEditable);
		valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
		unitItem->setFlags(unitItem->flags() & ~Qt::ItemIsEditable);
		timeItem->setFlags(timeItem->flags() & ~Qt::ItemIsEditable);

		// A0 协议仅“值”列在可写权限下可编辑；A3 全部不可编辑
		if (!isA3 && record.permission.contains(QLatin1Char('W')))
		{
			valueItem->setFlags(valueItem->flags() | Qt::ItemIsEditable);
		}
		valueItem->setData(kRoleDefaultValue, record.defaultValue);
		valueItem->setData(kRoleMinValue, record.minValue);
		valueItem->setData(kRoleMaxValue, record.maxValue);
		valueItem->setData(kRoleDataType, record.dataType.trimmed().toLower());

		table->setItem(row, 0, nameItem);
		table->setItem(row, 1, typeItem);
		table->setItem(row, 2, descItem);
		table->setItem(row, 3, valueItem);
		table->setItem(row, 4, unitItem);
		table->setCellWidget(row, 5, createActionCell(table, row, record));
		table->setItem(row, 6, timeItem);

		const quint32 key = makeCommandKey(record.protocolType, record.groupId, record.commandId);
		m_valueBindingMap[key].push_back(ValueBinding{table, row, record});
	}

	int row = 0;
	while (row < records.size())
	{
		const QString currentName = records.at(row).parameterName;
		int           spanCount   = 1;
		while (row + spanCount < records.size() && records.at(row + spanCount).parameterName == currentName)
		{
			++spanCount;
		}

		if (spanCount >= 2)
		{
			int colsArray[] = {0, 1, 4, 5, 6};
			int len         = sizeof(colsArray) / sizeof(int);
			for (int i = 0; i < len; i++)
			{
				table->setSpan(row, colsArray[i], spanCount, 1);
				for (int subRow = row + 1; subRow < row + spanCount; ++subRow)
				{
					if (table->item(subRow, 0) != nullptr)
					{
						table->item(subRow, 0)->setText(QString());
					}
				}
			}
		}

		row += spanCount;
	}

	return table;
}

QWidget* CommandOperationPage::createActionCell(QTableWidget* table, int row, const ParameterRecord& record)
{
	auto* container = new QWidget(this);
	auto* layout    = new QHBoxLayout(container);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(6);

	bool isA3       = isA3Protocol(record.protocolType);
	bool shouldPlot = isA3 && (record.remark == QStringLiteral("绘图"));

	if (shouldPlot)
	{
		// A3协议且remark=="绘图"：显示数据排列选择和绘图按钮
		auto* layoutCombo = new QHBoxLayout();
		layoutCombo->setSpacing(2);

		auto* layoutComboLabel = new QLabel(QStringLiteral("排列:"), container);
		layoutComboLabel->setStyleSheet(QStringLiteral("font-size: 10px; color: #666;"));

		auto* layoutComboBox = new QComboBox(container);
		layoutComboBox->setStyleSheet(QStringLiteral("font-size: 10px; padding: 2px;"));
		layoutComboBox->addItem(QStringLiteral("交错"), static_cast<int>(DataLayout::Interleaved));
		layoutComboBox->addItem(QStringLiteral("分块"), static_cast<int>(DataLayout::Blocked));
		layoutComboBox->setToolTip(
			QStringLiteral("数据排列方式：\n交错: [x0,y0,z0,x1,y1,z1...]\n分块: [x0,x1...y0,y1...,z0,z1...]"));

		layoutCombo->addWidget(layoutComboLabel);
		layoutCombo->addWidget(layoutComboBox);

		auto* plotButton = new QPushButton(QStringLiteral("绘图"), container);
		plotButton->setStyleSheet(
			QStringLiteral("background: #9b59b6; color: white; border: none; padding: 4px 12px;"));
		QObject::connect(plotButton, &QPushButton::clicked, container, [this, record, layoutComboBox]()
		{
			quint32 key = makeCommandKey(record.protocolType, record.groupId, record.commandId);

			// 保存数据排列方式
			DataLayout layout  = static_cast<DataLayout>(layoutComboBox->currentData().toInt());
			m_dataLayouts[key] = layout;

			// 检查是否已经有绘图对话框打开
			if (m_activePlotDialogs.contains(key))
			{
				// 已经有对话框，将其置顶
				m_activePlotDialogs[key]->activateWindow();
				m_activePlotDialogs[key]->raise();
			}
			else
			{
				// 获取同一命令的所有参数记录，生成legend名称
				QVector<ParameterRecord> commandRecords = getCommandRecords(key);
				QStringList              channelNames;
				int                      emptyDescCount = 0;

				for (const ParameterRecord& r : commandRecords)
				{
					if (r.description.trimmed().isEmpty())
					{
						channelNames << QString::number(++emptyDescCount);
					}
					else
					{
						channelNames << r.description;
					}
				}

				// 如果没有找到任何记录，至少添加一个默认通道
				if (channelNames.isEmpty())
				{
					channelNames << QStringLiteral("数据");
				}

				// 创建新的绘图对话框
				auto* plotDialog = new PlotDialog(record.parameterName, this);
				plotDialog->setTimeRange(10.0);            // 默认10秒显示
				plotDialog->setChannelNames(channelNames); // 设置自定义legend名称
				plotDialog->show();

				// 保存到映射中
				m_activePlotDialogs[key] = plotDialog;

				// 记录起始时间戳
				m_startTimestamps[key] = QDateTime::currentDateTime().toSecsSinceEpoch() +
						QDateTime::currentDateTime().time().msec() / 1000.0;

				// 连接对话框关闭信号 - 使用QDialog::finished信号更安全
				connect(plotDialog, &QDialog::finished, this, [this, key](int)
				{
					// 安全地移除对话框（使用QPointer会自动处理对象删除）
					m_activePlotDialogs.remove(key);
					m_startTimestamps.remove(key);
					m_dataLayouts.remove(key);
				});
			}

			// 触发读取操作以获取数据
			if (m_actionHandler)
			{
				m_actionHandler(record, ParameterAction::Read);
			}
		});
		layout->addLayout(layoutCombo);
		layout->addWidget(plotButton);
	}
	else if (isA3)
	{
		// A3协议但remark!="绘图"：操作列为空
		// 只添加一个空的stretch布局
		layout->addStretch(1);
	}
	else
	{
		// A0协议：正常显示读取/设置按钮
		if (record.permission.contains(QLatin1Char('R')))
		{
			auto* readButton = new QPushButton(QStringLiteral("读取"), container);
			readButton->
					setStyleSheet(
						QStringLiteral("background: #4a84d8; color: white; border: none; padding: 4px 12px;"));
			QObject::connect(readButton, &QPushButton::clicked, container, [this, record]()
			{
				emitAction(record, ParameterAction::Read);
			});
			layout->addWidget(readButton);
		}

		if (record.permission.contains(QLatin1Char('W')))
		{
			auto* writeButton = new QPushButton(QStringLiteral("设置"), container);
			writeButton->setStyleSheet(
				QStringLiteral("background: #4fd1c5; color: white; border: none; padding: 4px 12px;"));
			QObject::connect(writeButton, &QPushButton::clicked, container, [this, table, row, record]()
			{
				ParameterRecord editedRecord = record;
				if (table != nullptr && table->item(row, 3) != nullptr)
				{
					editedRecord.defaultValue = table->item(row, 3)->text();
				}
				if (table != nullptr && table->item(row, 6) != nullptr)
				{
					table->item(row, 6)->setText(
						QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
				}
				emitAction(editedRecord, ParameterAction::Write);
			});
			layout->addWidget(writeButton);
		}
	}

	layout->addStretch(1);
	return container;
}

void CommandOperationPage::emitAction(const ParameterRecord& record, ParameterAction action)
{
	if (m_actionHandler)
	{
		m_actionHandler(record, action);
	}
}

void CommandOperationPage::updateValues(quint8            protocolType, int groupId, int commandId,
                                        const QByteArray& payload)
{
	const quint32 key = makeCommandKey(protocolType, groupId, commandId);
	if (!m_valueBindingMap.contains(key) || payload.isEmpty())
	{
		return;
	}

	// A3协议数据进行频率限制
	if (isA3Protocol(protocolType))
	{
		QDateTime now = QDateTime::currentDateTime();
		if (m_lastUpdateTime.contains(key))
		{
			qint64 elapsed = m_lastUpdateTime[key].msecsTo(now);
			if (elapsed < MIN_UPDATE_INTERVAL_MS)
			{
				// 间隔太短，跳过本次更新
				return;
			}
		}
		m_lastUpdateTime[key] = now;
	}

	QVector<ValueBinding> bindings = m_valueBindingMap.value(key);
	std::sort(bindings.begin(), bindings.end(), [](const ValueBinding& left, const ValueBinding& right)
	{
		return left.record.readIndex < right.record.readIndex;
	});

	// 如果是A3协议且有打开的绘图对话框，解析并绘制数组数据
	bool isA3 = isA3Protocol(protocolType);
	if (isA3 && m_activePlotDialogs.contains(key) && m_dataLayouts.contains(key))
	{
		parseAndPlotA3Data(key, bindings, payload, m_dataLayouts[key]);
	}

	// 更新表格显示（单个值）
	int offset = 0;
	for (const ValueBinding& binding : bindings)
	{
		if (binding.table == nullptr || binding.row < 0 || binding.row >= binding.table->rowCount())
		{
			continue;
		}

		auto* valueItem = binding.table->item(binding.row, 3);
		auto* timeItem  = binding.table->item(binding.row, 6);
		if (valueItem == nullptr || timeItem == nullptr)
		{
			offset += qMax(0, binding.record.dataLength);
			continue;
		}

		valueItem->setText(decodeValue(binding.record, payload, offset));
		timeItem->setText(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
		offset += qMax(0, binding.record.dataLength);
	}
}

QVector<ParameterRecord> CommandOperationPage::getCommandRecords(quint32 key) const
{
	QVector<ParameterRecord> records;
	if (!m_valueBindingMap.contains(key))
		return records;

	QVector<ValueBinding> bindings = m_valueBindingMap.value(key);
	std::sort(bindings.begin(), bindings.end(), [](const ValueBinding& left, const ValueBinding& right)
	{
		return left.record.readIndex < right.record.readIndex;
	});

	for (const ValueBinding& binding : bindings)
	{
		records.push_back(binding.record);
	}

	return records;
}

void CommandOperationPage::parseAndPlotA3Data(quint32           key, const QVector<ValueBinding>& bindings,
                                              const QByteArray& payload, DataLayout               layout)
{
	if (bindings.isEmpty() || payload.isEmpty())
		return;

	PlotDialog* dialog = m_activePlotDialogs[key];
	if (!dialog)
		return;

	// 计算每个数据类型的大小
	QVector<int> dataSizes;
	int          totalSingleSize = 0;
	for (const ValueBinding& binding : bindings)
	{
		int size = qMax(1, binding.record.dataLength);
		dataSizes.push_back(size);
		totalSingleSize += size;
	}

	if (totalSingleSize == 0 || payload.size() < totalSingleSize)
		return;

	// 计算数据点数量
	int pointCount = payload.size() / totalSingleSize;
	if (pointCount == 0)
		return;

	double currentTime = QDateTime::currentDateTime().toSecsSinceEpoch() +
			QDateTime::currentDateTime().time().msec() / 1000.0;
	double startTime = m_startTimestamps[key];

	// 解析数据
	for (int i = 0; i < pointCount; ++i)
	{
		double timestamp = startTime + (currentTime - startTime) * (i + 1) / pointCount;

		for (int channel = 0; channel < bindings.size(); ++channel)
		{
			int offset = 0;
			if (layout == DataLayout::Interleaved)
			{
				// 交错排列: [x0,y0,z0,x1,y1,z1...]
				offset = i * totalSingleSize;
				for (int c = 0; c < channel; ++c)
				{
					offset += dataSizes[c];
				}
			}
			else
			{
				// 分块排列: [x0,x1...y0,y1...,z0,z1...]
				int channelOffset = 0;
				for (int c = 0; c < channel; ++c)
				{
					channelOffset += dataSizes[c] * pointCount;
				}
				offset = channelOffset + i * dataSizes[channel];
			}

			// 解码值
			if (offset + dataSizes[channel] <= payload.size())
			{
				QString valueStr = decodeValue(bindings[channel].record, payload, offset);
				bool    ok       = false;
				double  value    = valueStr.toDouble(&ok);
				if (ok)
				{
					dialog->addDataPoint(timestamp, value, channel);
				}
			}
		}
	}
}

QString CommandOperationPage::decodeValue(const ParameterRecord& record, const QByteArray& payload, int offset)
{
	const int length = qMax(0, record.dataLength);
	if (offset < 0 || offset >= payload.size())
	{
		return QString();
	}

	const QByteArray chunk = payload.mid(offset, length > 0 ? length : payload.size() - offset);
	const QString    type  = record.dataType.trimmed().toLower();

	if (type == QStringLiteral("uint8_t") && chunk.size() >= 1)
	{
		return QString::number(static_cast<quint8>(chunk.at(0)));
	}
	if (type == QStringLiteral("int8_t") && chunk.size() >= 1)
	{
		return QString::number(static_cast<qint8>(chunk.at(0)));
	}
	if (type == QStringLiteral("uint16_t") && chunk.size() >= 2)
	{
		return QString::number(qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(chunk.constData())));
	}
	if (type == QStringLiteral("int16_t") && chunk.size() >= 2)
	{
		return QString::number(qFromLittleEndian<qint16>(reinterpret_cast<const uchar*>(chunk.constData())));
	}
	if (type == QStringLiteral("uint32_t") && chunk.size() >= 4)
	{
		return QString::number(qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(chunk.constData())));
	}
	if (type == QStringLiteral("int32_t") && chunk.size() >= 4)
	{
		return QString::number(qFromLittleEndian<qint32>(reinterpret_cast<const uchar*>(chunk.constData())));
	}
	if (type == QStringLiteral("float") && chunk.size() >= 4)
	{
		const quint32 raw   = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(chunk.constData()));
		float         value = 0.0f;
		std::memcpy(&value, &raw, sizeof(float));
		return QString::number(value, 'f', 3);
	}
	if (type == QStringLiteral("double") && chunk.size() >= 8)
	{
		const quint64 raw   = qFromLittleEndian<quint64>(reinterpret_cast<const uchar*>(chunk.constData()));
		double        value = 0.0;
		std::memcpy(&value, &raw, sizeof(double));
		return QString::number(value, 'f', 6);
	}
	if (type == QStringLiteral("int64_t") && chunk.size() >= 8)
	{
		return QString::number(qFromLittleEndian<qint64>(reinterpret_cast<const uchar*>(chunk.constData())));
	}
	if (type == QStringLiteral("uint64_t") && chunk.size() >= 8)
	{
		return QString::number(qFromLittleEndian<quint64>(reinterpret_cast<const uchar*>(chunk.constData())));
	}
	if (type == QStringLiteral("string"))
	{
		return QString::fromLatin1(chunk).trimmed();
	}

	return QString::fromLatin1(chunk.toHex(' '));
}

void CommandOperationPage::onNewDataReceived(quint8 protocolType, int groupId, int commandId, double value)
{
	quint32 key = makeCommandKey(protocolType, groupId, commandId);

	// 检查是否有对应的绘图对话框打开
	if (m_activePlotDialogs.contains(key))
	{
		double currentTime = QDateTime::currentDateTime().toSecsSinceEpoch() +
				QDateTime::currentDateTime().time().msec() / 1000.0;
		double relativeTime = currentTime - m_startTimestamps[key];

		// 将数据点添加到绘图对话框
		m_activePlotDialogs[key]->addDataPoint(relativeTime, value);
	}
}

quint32 CommandOperationPage::makeCommandKey(quint8 protocolType, int groupId, int commandId)
{
	return (static_cast<quint32>(protocolType) << 16) |
			((static_cast<quint32>(groupId) & 0xFF) << 8) |
			(static_cast<quint32>(commandId) & 0xFF);
}
