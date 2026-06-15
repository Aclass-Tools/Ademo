#include "ui/ProtocolConfigPage.h"

#include <algorithm>

#include <QAbstractItemView>
#include <QHeaderView>
#include <QTabWidget>
#include <QTableWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QSignalBlocker>

#include "AddProtocolDialog.h"
#include "AddMainOrderDialog.h"
#include "ui_ProtocolConfigPage.h"
#include "services/ConfigService.h"
#include "services/ProtocolParser.h"
#include "ui/delegates/ComboBoxDelegate.h"
#include "ui/delegates/RemarkComboDelegate.h"
#include <QMouseEvent>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>

namespace
{
	QStringList kHeaders()
	{
		return {
			QStringLiteral("参数名称"),
			QStringLiteral("子命令"),
			QStringLiteral("权限"),
			QStringLiteral("读索引"),
			QStringLiteral("数据类型"),
			QStringLiteral("数据长度"),
			QStringLiteral("说明"),
			QStringLiteral("默认值"),
			QStringLiteral("单位"),
			QStringLiteral("存储位置"),
			QStringLiteral("上限值"),
			QStringLiteral("下限值"),
			QStringLiteral("步进"),
			QStringLiteral("枚举"),
			QStringLiteral("备注")
		};
	}

	QString protocolDisplayName(quint8 protocolType)
	{
		QString displayName;
		switch (protocolType)
		{
			case 0xA0:
				displayName = QStringLiteral("控制指令(A0)");
				break;
			case 0xA3:
				displayName = QStringLiteral("上传指令(A3)");
				break;
			default: break;
		}
		return displayName;
	}

	int defaultLengthForType(const QString& type)
	{
		const QString normalized = type.trimmed().toLower();
		if (normalized == QStringLiteral("uint8_t") || normalized == QStringLiteral("int8_t"))
		{
			return 1;
		}
		if (normalized == QStringLiteral("uint16_t") || normalized == QStringLiteral("int16_t"))
		{
			return 2;
		}
		if (normalized == QStringLiteral("uint32_t") || normalized == QStringLiteral("int32_t") ||
		    normalized == QStringLiteral("float"))
		{
			return 4;
		}
		if (normalized == QStringLiteral("uint64_t") || normalized == QStringLiteral("int64_t") ||
		    normalized == QStringLiteral("double"))
		{
			return 8;
		}
		return 0;
	}
}

QTabWidget* ProtocolConfigPage::createProtocolTabs(QWidget* parent)
{
	auto* tabs = new QTabWidget(parent);
	tabs->setDocumentMode(true);
	tabs->setStyleSheet(QStringLiteral(
		"QTabBar::tab { min-width: 92px; padding: 6px 14px; background: #f4f7fb; border: 1px solid #d8e4ec; }"
		"QTabBar::tab:selected { background: #ffffff; color: #0f172a; }"
		"QTabWidget::pane { border: 1px solid #d8e4ec; background: white; }"));
	// 为 tab bar 安装事件过滤器，捕获双击事件
	tabs->tabBar()->installEventFilter(this);
	return tabs;
}

ProtocolConfigPage::ProtocolConfigPage(QWidget* parent)
	: QWidget(parent)
	  , m_ui(std::make_unique<Ui::ProtocolConfigPage>())
{
	m_ui->setupUi(this);
	m_groupTabs = m_ui->groupTabs;
	m_groupTabs->setDocumentMode(true);
	m_groupTabs->setStyleSheet(QStringLiteral(
		"QTabBar::tab { min-width: 92px; padding: 6px 14px; background: #f4f7fb; border: 1px solid #d8e4ec; }"
		"QTabBar::tab:selected { background: #ffffff; color: #0f172a; }"
		"QTabWidget::pane { border: 1px solid #d8e4ec; background: white; }"));
	setStyleSheet(QStringLiteral("background: white;"));

	// 初始化 delegates
	m_permissionDelegate = new ComboBoxDelegate(this);
	m_permissionDelegate->setOptions(getPermissionOptions());

	m_dataTypeDelegate = new ComboBoxDelegate(this);
	m_dataTypeDelegate->setOptions(getDataTypeOptions());

	m_remarkDelegate = new RemarkComboDelegate(this);

	// // 连接按钮信号槽
	// connect(m_ui->saveButton, &QPushButton::clicked, this, &ProtocolConfigPage::saveProtocolConfigure,
	//         Qt::UniqueConnection);
	// connect(m_ui->restoreButton, &QPushButton::clicked, this, &ProtocolConfigPage::restoreProtocolConfigure);
	// connect(m_ui->btnAddMainOrder, &QPushButton::clicked, this, &ProtocolConfigPage::on_btnAddMainOrder_clicked);
	// connect(m_ui->btnDeleteMainOrder, &QPushButton::clicked, this, &ProtocolConfigPage::on_btnDeleteMainOrder_clicked);
	// connect(m_ui->btnAddSubOrder, &QPushButton::clicked, this, &ProtocolConfigPage::on_btnAddSubOrder_clicked);
	// connect(m_ui->btnClearAllOrder, &QPushButton::clicked, this, &ProtocolConfigPage::on_btnClearAllOrder_clicked);
	// connect(m_ui->btnDeleteSubOrder, &QPushButton::clicked, this, &ProtocolConfigPage::on_btnDeleteSubOrder_clicked);
}

ProtocolConfigPage::~ProtocolConfigPage() = default;

void ProtocolConfigPage::setRecords(const QVector<ParameterRecord>& records)
{
	m_records = records;
	m_groupTabs->clear();

	QMap<quint8, QVector<ParameterRecord>> protocolGrouped;
	for (const ParameterRecord& record : records)
	{
		protocolGrouped[record.protocolType].push_back(record);
	}

	auto keys = protocolGrouped.keys();
	std::sort(keys.begin(), keys.end());

	for (auto key : keys)
	{
		auto*                               protocolTabs = createProtocolTabs(this);
		QMap<int, QVector<ParameterRecord>> grouped;
		QMap<int, int>                      groupIds;
		QMap<int, QString>                  groupDes;
		for (const ParameterRecord& record : protocolGrouped[key])
		{
			grouped[record.groupId].push_back(record);
			groupIds[record.groupId] = record.groupId;
			groupDes[record.groupId] = record.groupName;
		}

		auto groupKeys = grouped.keys();
		std::sort(groupKeys.begin(), groupKeys.end());

		for (auto groupKey : groupKeys)
		{
			QVector<ParameterRecord>& sortedRecords = grouped[groupKey];
			std::sort(sortedRecords.begin(), sortedRecords.end(),
			          [](const ParameterRecord& left, const ParameterRecord& right)
			          {
				          if (left.commandId != right.commandId)
				          {
					          return left.commandId < right.commandId;
				          }
				          return left.readIndex < right.readIndex;
			          });

			QTableWidget* table = createTable(this);
			table->setRowCount(sortedRecords.size());
			for (int row = 0; row < sortedRecords.size(); ++row)
			{
				populateRow(table, row, sortedRecords.at(row));
			}

			int row = 0;
			while (row < sortedRecords.size())
			{
				const QString currentCommand = sortedRecords.at(row).commandName;
				int           spanCount      = 1;
				while (row + spanCount < sortedRecords.size() && sortedRecords.at(row + spanCount).commandName ==
					currentCommand)
				{
					++spanCount;
				}

				if (spanCount >= 2)
				{
					int colsArray[] = {0, 1, 2, 4, 8, 9, 14};
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

			protocolTabs->addTab(table, groupTabTitle(groupDes.value(groupKey), groupIds.value(groupKey)));
		}

		m_groupTabs->addTab(protocolTabs, protocolDisplayName(key));
	}
}

QVector<ParameterRecord> ProtocolConfigPage::records() const
{
	return m_records;
}

QStringList ProtocolConfigPage::getPermissionOptions()
{
	return {QStringLiteral("R"), QStringLiteral("W"), QStringLiteral("RW")};
}

QStringList ProtocolConfigPage::getDataTypeOptions()
{
	return {
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
	};
}

QTableWidget* ProtocolConfigPage::createTable(QWidget* parent)
{
	auto* table = new QTableWidget(parent);
	table->setColumnCount(kHeaders().size());
	table->setHorizontalHeaderLabels(kHeaders());
	table->setAlternatingRowColors(true);
	table->setSelectionMode(QAbstractItemView::SingleSelection);
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->verticalHeader()->setVisible(false);
	table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	table->horizontalHeader()->setSectionsMovable(true);
	table->horizontalHeader()->setSortIndicatorShown(false);
	table->setSortingEnabled(false);
	table->setEditTriggers(
		QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
	table->setStyleSheet(QStringLiteral(
		"QTableWidget { border: none; background: white; gridline-color: #d9e2ec; }"
		"QHeaderView::section { background: #a8d3e3; color: #0f172a; padding: 6px; border: 1px solid #7faec0; }"));

	// 为权限列（第2列）设置 delegate
	table->setItemDelegateForColumn(2, m_permissionDelegate);
	// 为数据类型列（第4列）设置 delegate
	table->setItemDelegateForColumn(4, m_dataTypeDelegate);
	// 为备注列（第14列）设置委托
	table->setItemDelegateForColumn(14, m_remarkDelegate);

	connect(table, &QTableWidget::itemChanged, this, [table](QTableWidgetItem* item)
	{
		if (item == nullptr || item->column() != 4)
		{
			return;
		}
		const int row = item->row();
		if (row < 0 || row >= table->rowCount())
		{
			return;
		}

		const int targetLength = defaultLengthForType(item->text());
		if (targetLength <= 0)
		{
			return;
		}
		QTableWidgetItem* lengthItem = table->item(row, 5);
		if (lengthItem == nullptr)
		{
			lengthItem = new QTableWidgetItem;
			table->setItem(row, 5, lengthItem);
		}
		QSignalBlocker blocker(table);
		lengthItem->setText(QString::number(targetLength));
	});

	return table;
}

void ProtocolConfigPage::populateRow(QTableWidget* table, int row, const ParameterRecord& record)
{
	const QStringList values = {
		record.parameterName,
		QString::number(record.commandId),
		record.permission,
		QString::number(record.readIndex),
		record.dataType,
		QString::number(record.dataLength),
		record.description,
		record.defaultValue,
		record.unit,
		record.storageLocation,
		record.maxValue,
		record.minValue,
		record.step,
		record.enumValues.join(QStringLiteral(" | ")),
		record.remark
	};

	for (int column = 0; column < values.size(); ++column)
	{
		auto* item = new QTableWidgetItem(values.at(column));
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		// 存储协议类型到UserRole，供RemarkComboDelegate使用
		item->setData(RemarkComboDelegate::ProtocolTypeRole, static_cast<int>(record.protocolType));
		table->setItem(row, column, item);
	}
}

QTableWidget* ProtocolConfigPage::findTableAt(int protocolIndex, int groupIndex) const
{
	QWidget* protocolWidget = m_groupTabs->widget(protocolIndex);
	if (!protocolWidget)
		return nullptr;

	QTabWidget* protocolTabs = qobject_cast<QTabWidget*>(protocolWidget);
	if (!protocolTabs)
		return nullptr;

	QWidget* groupWidget = protocolTabs->widget(groupIndex);
	if (!groupWidget)
		return nullptr;

	return qobject_cast<QTableWidget*>(groupWidget);
}

QVector<ParameterRecord> ProtocolConfigPage::collectRecordsFromTables() const
{
	QVector<ParameterRecord> records;

	for (int protocolIndex = 0; protocolIndex < m_groupTabs->count(); ++protocolIndex)
	{
		QTabWidget* protocolTabs = qobject_cast<QTabWidget*>(m_groupTabs->widget(protocolIndex));
		if (!protocolTabs)
			continue;

		QString protocolType = m_groupTabs->tabText(protocolIndex);
		// 去除显示名称的前缀，获取原始协议类型 ("A0" 或 "A3")
		if (protocolType.contains("("))
		{
			QRegularExpression      rx("\\(([^)]+)\\)");
			QRegularExpressionMatch match = rx.match(protocolType);
			if (match.hasMatch())
			{
				protocolType = match.captured(1);
			}
		}

		for (int groupIndex = 0; groupIndex < protocolTabs->count(); ++groupIndex)
		{
			QTableWidget* table = qobject_cast<QTableWidget*>(protocolTabs->widget(groupIndex));
			if (!table)
				continue;

			int              row                  = 0;
			ParameterRecord* currentCommandRecord = nullptr;

			while (row < table->rowCount())
			{
				ParameterRecord record;
				record.protocolType = protocolStringToByte(protocolType);

				// 获取分组信息（从标签中提取）
				QString                 groupTitle = protocolTabs->tabText(groupIndex);
				QRegularExpression      groupRx("^(.*) (\\d+)$");
				QRegularExpressionMatch groupMatch = groupRx.match(groupTitle);
				if (groupMatch.hasMatch())
				{
					record.groupName = groupMatch.captured(1);
					record.groupId   = groupMatch.captured(2).toInt();
				}

				// 检查是否是合并单元格的起始行
				int commandNameRowSpan = table->rowSpan(row, 1);

				if (commandNameRowSpan > 1)
				{
					// 合并单元格的起始行
					if (table->item(row, 1) && !table->item(row, 1)->text().trimmed().isEmpty())
					{
						// 读取命令信息（将在下面的循环中用于继承）
						ParameterRecord commandRecord;
						if (table->item(row, 0))
							commandRecord.parameterName = table->item(row, 0)->text().trimmed();
						if (table->item(row, 1))
							commandRecord.commandId = table->item(row, 1)->text().toInt();
						if (table->item(row, 2))
							commandRecord.permission = table->item(row, 2)->text().trimmed();
						if (table->item(row, 4))
							commandRecord.dataType = table->item(row, 4)->text().trimmed();
						if (table->item(row, 8))
							commandRecord.unit = table->item(row, 8)->text().trimmed();
						if (table->item(row, 9))
							commandRecord.storageLocation = table->item(row, 9)->text().trimmed();
						if (table->item(row, 14))
							commandRecord.remark = table->item(row, 14)->text().trimmed();

						currentCommandRecord = new ParameterRecord(commandRecord);
					}

					// 处理合并单元格内的所有行
					for (int subRow = row; subRow < row + commandNameRowSpan; ++subRow)
					{
						// 复制基础命令信息
						ParameterRecord subRecord = record;
						if (currentCommandRecord)
						{
							subRecord.commandName     = currentCommandRecord->parameterName; // 重要：设置 commandName
							subRecord.commandId       = currentCommandRecord->commandId;
							subRecord.permission      = currentCommandRecord->permission;
							subRecord.dataType        = currentCommandRecord->dataType;
							subRecord.unit            = currentCommandRecord->unit;
							subRecord.storageLocation = currentCommandRecord->storageLocation;
							subRecord.remark          = currentCommandRecord->remark;

							// A3协议remark默认为"绘图"
							if (subRecord.protocolType == PROTOCOL_A3 && subRecord.remark.isEmpty())
							{
								subRecord.remark = QStringLiteral("绘图");
							}
						}

						// 读取参数信息
						if (table->item(subRow, 0))
							subRecord.parameterName = table->item(subRow, 0)->text().trimmed();
						if (table->item(subRow, 3))
							subRecord.readIndex = table->item(subRow, 3)->text().toInt();
						if (table->item(subRow, 5))
							subRecord.dataLength = table->item(subRow, 5)->text().toInt();
						if (table->item(subRow, 6))
							subRecord.description = table->item(subRow, 6)->text().trimmed();
						if (table->item(subRow, 7))
							subRecord.defaultValue = table->item(subRow, 7)->text().trimmed();
						if (table->item(subRow, 10))
							subRecord.maxValue = table->item(subRow, 10)->text().trimmed();
						if (table->item(subRow, 11))
							subRecord.minValue = table->item(subRow, 11)->text().trimmed();
						if (table->item(subRow, 12))
							subRecord.step = table->item(subRow, 12)->text().trimmed();
						if (table->item(subRow, 13))
						{
							QString enumText     = table->item(subRow, 13)->text().trimmed();
							subRecord.enumValues = enumText.split(QStringLiteral(" | "), Qt::SkipEmptyParts);
						}

						records.push_back(subRecord);
					}

					delete currentCommandRecord;
					currentCommandRecord = nullptr;
					row                  += commandNameRowSpan;
				}
				else
				{
					// 普通行
					if (table->item(row, 0) && !table->item(row, 0)->text().trimmed().isEmpty())
					{
						if (table->item(row, 0))
						{
							record.parameterName = table->item(row, 0)->text().trimmed();
							record.commandName   = record.parameterName; // 重要：设置 commandName
						}
						if (table->item(row, 1))
							record.commandId = table->item(row, 1)->text().toInt();
						if (table->item(row, 2))
							record.permission = table->item(row, 2)->text().trimmed();
						if (table->item(row, 3))
							record.readIndex = table->item(row, 3)->text().toInt();
						if (table->item(row, 4))
							record.dataType = table->item(row, 4)->text().trimmed();
						if (table->item(row, 5))
							record.dataLength = table->item(row, 5)->text().toInt();
						if (table->item(row, 6))
							record.description = table->item(row, 6)->text().trimmed();
						if (table->item(row, 7))
							record.defaultValue = table->item(row, 7)->text().trimmed();
						if (table->item(row, 8))
							record.unit = table->item(row, 8)->text().trimmed();
						if (table->item(row, 9))
							record.storageLocation = table->item(row, 9)->text().trimmed();
						if (table->item(row, 10))
							record.maxValue = table->item(row, 10)->text().trimmed();
						if (table->item(row, 11))
							record.minValue = table->item(row, 11)->text().trimmed();
						if (table->item(row, 12))
							record.step = table->item(row, 12)->text().trimmed();
						if (table->item(row, 13))
						{
							QString enumText  = table->item(row, 13)->text().trimmed();
							record.enumValues = enumText.split(QStringLiteral(" | "), Qt::SkipEmptyParts);
						}
						if (table->item(row, 14))
							record.remark = table->item(row, 14)->text().trimmed();
						else
							record.remark = QStringLiteral("绘图"); // A3协议默认"绘图"

						records.push_back(record);
					}
					row++;
				}
			}
		}
	}

	return records;
}

bool ProtocolConfigPage::checkForDuplicateSubCommands()
{
	// 直接遍历表格进行检测，这样可以准确显示问题所在的行号
	for (int protocolIndex = 0; protocolIndex < m_groupTabs->count(); ++protocolIndex)
	{
		QTabWidget* protocolTabs = qobject_cast<QTabWidget*>(m_groupTabs->widget(protocolIndex));
		if (!protocolTabs)
			continue;

		QString protocolType = m_groupTabs->tabText(protocolIndex);
		if (protocolType.contains("("))
		{
			QRegularExpression      rx("\\(([^)]+)\\)");
			QRegularExpressionMatch match = rx.match(protocolType);
			if (match.hasMatch())
			{
				protocolType = match.captured(1);
			}
		}

		for (int groupIndex = 0; groupIndex < protocolTabs->count(); ++groupIndex)
		{
			QTableWidget* table = qobject_cast<QTableWidget*>(protocolTabs->widget(groupIndex));
			if (!table)
				continue;

			QString                 groupTitle = protocolTabs->tabText(groupIndex);
			QRegularExpression      groupRx("^(.*) (\\d+)$");
			QRegularExpressionMatch groupMatch = groupRx.match(groupTitle);
			QString                 groupName  = groupTitle;
			int                     groupId    = 0;
			if (groupMatch.hasMatch())
			{
				groupName = groupMatch.captured(1);
				groupId   = groupMatch.captured(2).toInt();
			}

			QString        key = protocolType + QString::number(groupId);
			QMap<int, int> subCommandRows;

			int row = 0;
			while (row < table->rowCount())
			{
				// 检查是否是合并单元格的起始行
				int rowSpan = table->rowSpan(row, 1);
				if (rowSpan > 1)
				{
					// 只检测合并单元格的第一行，其余跳过
					QTableWidgetItem* item = table->item(row, 1);
					if (item && !item->text().trimmed().isEmpty())
					{
						bool ok         = false;
						int  subCommand = item->text().toInt(&ok);
						if (ok)
						{
							if (subCommandRows.contains(subCommand))
							{
								QMessageBox::warning(
									this,
									tr("子命令重复"),
									tr("检测到协议 %1 分组 %2 中第 %3 行和第 %4 行的子命令 %5 重复，请修改！")
									.arg(protocolType)
									.arg(groupName)
									.arg(subCommandRows[subCommand] + 1)
									.arg(row + 1)
									.arg(subCommand)
								);
								return false;
							}
							else
							{
								subCommandRows[subCommand] = row;
							}
						}
					}
					// 跳过合并的行
					row += rowSpan;
				}
				else
				{
					// 非合并行，正常检测
					QTableWidgetItem* item = table->item(row, 1);
					if (item && !item->text().trimmed().isEmpty())
					{
						bool ok         = false;
						int  subCommand = item->text().toInt(&ok);
						if (ok)
						{
							if (subCommandRows.contains(subCommand))
							{
								QMessageBox::warning(
									this,
									tr("子命令重复"),
									tr("检测到协议 %1 分组 %2 中第 %3 行和第 %4 行的子命令 %5 重复，请修改！")
									.arg(protocolType)
									.arg(groupName)
									.arg(subCommandRows[subCommand] + 1)
									.arg(row + 1)
									.arg(subCommand)
								);
								return false;
							}
							else
							{
								subCommandRows[subCommand] = row;
							}
						}
					}
					row++;
				}
			}
		}
	}

	return true;
}

bool ProtocolConfigPage::saveConfig(const QString& filePath)
{
	if (!checkForDuplicateSubCommands())
	{
		return false;
	}

	const QVector<ParameterRecord> records = collectRecordsFromTables();
	ParamConfigure  configure = ConfigService::instance().currentConfigure();
	configure.records = records;

	QString errorMessage;
	if (!ConfigService::instance().saveConfigure(filePath, configure, &errorMessage))
	{
		QMessageBox::critical(
			this,
			tr("保存失败"),
			tr("无法保存配置文件: %1").arg(errorMessage.isEmpty() ? filePath : errorMessage)
		);
		emit saveFailed(errorMessage.isEmpty() ? QStringLiteral("无法保存配置文件") : errorMessage);
		return false;
	}

	m_records = records;
	QMessageBox::information(
		this,
		tr("保存成功"),
		tr("配置文件已成功保存到: %1").arg(filePath)
	);
	emit saveSuccess();
	return true;
}

bool ProtocolConfigPage::loadConfig(const QString& filePath)
{
	ParamConfigure configure;
	QString                      errorMessage;
	if (!ConfigService::instance().loadConfigure(filePath, &configure, &errorMessage))
	{
		QMessageBox::critical(
			this,
			tr("加载失败"),
			tr("无法加载配置数据: %1").arg(errorMessage.isEmpty() ? filePath : errorMessage)
		);
		return false;
	}

	setRecords(configure.records);

	QMessageBox::information(
		this,
		tr("加载成功"),
		tr("配置文件已成功加载: %1").arg(filePath)
	);
	return true;
}

void ProtocolConfigPage::saveProtocolConfigure()
{
	QString filePath = QFileDialog::getSaveFileName(
		this,
		tr("保存配置"),
		ConfigService::instance().findDefaultConfigPath(),
		tr("JSON 文件 (*.json);;所有文件 (*.*)")
	);

	if (!filePath.isEmpty())
	{
		saveConfig(filePath);
	}
}

void ProtocolConfigPage::restoreProtocolConfigure()
{
	QString filePath = QFileDialog::getOpenFileName(
		this,
		tr("加载配置"),
		ConfigService::instance().findDefaultConfigPath(),
		tr("JSON 文件 (*.json);;所有文件 (*.*)")
	);

	if (!filePath.isEmpty())
	{
		loadConfig(filePath);
	}
}

QString ProtocolConfigPage::groupTabTitle(const QString& groupName, int groupId)
{
	return QStringLiteral("%1 %2").arg(groupName).arg(groupId);
}

void ProtocolConfigPage::on_btnAddMainOrder_clicked()
{
	AddMainOrderDialog dialog(this);
	if (dialog.exec() != QDialog::Accepted)
	{
		return;
	}

	QString mainOrderName = dialog.getMainOrderName();
	QString mainOrderNum  = dialog.getMainOrderNum();
	QString protocolType  = dialog.getProtocolType();

	bool ok         = false;
	int  newGroupId = mainOrderNum.toInt(&ok);
	if (!ok || newGroupId < 0)
	{
		QMessageBox::warning(this, QStringLiteral("警告"),
		                     QStringLiteral("主指令序号必须是大于等于0的整数"));
		return;
	}

	// 检查主指令名称和序号是否已存在
	bool found = false;
	for (int protocolIndex = 0; protocolIndex < m_groupTabs->count(); ++protocolIndex)
	{
		QString currentProtocolType = m_groupTabs->tabText(protocolIndex);
		if (currentProtocolType.contains("("))
		{
			QRegularExpression      rx("\\(([^)]+)\\)");
			QRegularExpressionMatch match = rx.match(currentProtocolType);
			if (match.hasMatch())
			{
				currentProtocolType = match.captured(1);
			}
		}

		if (currentProtocolType.compare(protocolType, Qt::CaseInsensitive) != 0)
		{
			continue;
		}

		QTabWidget* protocolTabs = qobject_cast<QTabWidget*>(m_groupTabs->widget(protocolIndex));
		if (!protocolTabs)
		{
			continue;
		}

		for (int groupIndex = 0; groupIndex < protocolTabs->count(); ++groupIndex)
		{
			QString                 groupTitle = protocolTabs->tabText(groupIndex);
			QRegularExpression      groupRx("^(.*) (\\d+)$");
			QRegularExpressionMatch groupMatch = groupRx.match(groupTitle);
			QString                 groupName  = groupTitle;
			int                     groupId    = 0;
			if (groupMatch.hasMatch())
			{
				groupName = groupMatch.captured(1);
				groupId   = groupMatch.captured(2).toInt();
			}

			if (groupName.compare(mainOrderName, Qt::CaseInsensitive) == 0 || groupId == newGroupId)
			{
				found = true;
				break;
			}
		}

		if (found)
		{
			break;
		}
	}

	if (found)
	{
		QMessageBox::warning(this, QStringLiteral("警告"),
		                     QStringLiteral("主指令名称或序号在协议 %1 中已存在，请使用其他值").arg(protocolType));
		return;
	}

	// 提示用户是否确认添加
	auto reply = QMessageBox::question(this, QStringLiteral("确认添加"),
	                                   QStringLiteral("确定要添加主指令 \"%1\" (%2) 到配置表中吗？").arg(mainOrderName).arg(
		                                   protocolType),
	                                   QMessageBox::Yes | QMessageBox::No);

	if (reply != QMessageBox::Yes)
	{
		return;
	}

	// 找到或创建对应的协议标签页
	int targetProtocolIndex = -1;
	for (int protocolIndex = 0; protocolIndex < m_groupTabs->count(); ++protocolIndex)
	{
		QString currentProtocolType = m_groupTabs->tabText(protocolIndex);
		if (currentProtocolType.contains("("))
		{
			QRegularExpression      rx("\\(([^)]+)\\)");
			QRegularExpressionMatch match = rx.match(currentProtocolType);
			if (match.hasMatch())
			{
				currentProtocolType = match.captured(1);
			}
		}

		if (currentProtocolType.compare(protocolType, Qt::CaseInsensitive) == 0)
		{
			targetProtocolIndex = protocolIndex;
			break;
		}
	}

	QTabWidget* protocolTabs = nullptr;
	if (targetProtocolIndex < 0)
	{
		// 创建新的协议标签页
		protocolTabs        = createProtocolTabs(this);
		QString displayName = protocolDisplayName(protocolStringToByte(protocolType));
		m_groupTabs->addTab(protocolTabs, displayName);
		targetProtocolIndex = m_groupTabs->count() - 1;
	}
	else
	{
		protocolTabs = qobject_cast<QTabWidget*>(m_groupTabs->widget(targetProtocolIndex));
	}

	if (!protocolTabs)
	{
		QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("无法创建或获取协议标签页"));
		return;
	}


	// 创建新的表格并添加到协议标签页
	QTableWidget* table = createTable(this);
	protocolTabs->addTab(table, groupTabTitle(mainOrderName, newGroupId));

	// 更新记录列表
	m_records = collectRecordsFromTables();

	QMessageBox::information(this, QStringLiteral("成功"),
	                         QStringLiteral("主指令 \"%1\" (%2) 已成功添加").arg(mainOrderName).arg(protocolType));
}

void ProtocolConfigPage::on_btnDeleteMainOrder_clicked()
{
	// 获取当前选中的协议标签和分组标签
	int currentProtocolIndex = m_groupTabs->currentIndex();
	if (currentProtocolIndex < 0 || currentProtocolIndex >= m_groupTabs->count())
	{
		QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("请先选择一个协议标签页"));
		return;
	}

	QTabWidget* protocolTabs = qobject_cast<QTabWidget*>(m_groupTabs->widget(currentProtocolIndex));
	if (!protocolTabs)
	{
		QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("无效的协议标签页"));
		return;
	}

	int currentGroupIndex = protocolTabs->currentIndex();
	if (currentGroupIndex < 0 || currentGroupIndex >= protocolTabs->count())
	{
		QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("请先选择一个主指令标签页"));
		return;
	}

	QString                 groupTitle = protocolTabs->tabText(currentGroupIndex);
	QRegularExpression      groupRx("^(.*) (\\d+)$");
	QRegularExpressionMatch groupMatch = groupRx.match(groupTitle);
	QString                 groupName  = groupTitle;
	if (groupMatch.hasMatch())
	{
		groupName = groupMatch.captured(1);
	}

	// 提示用户是否确认删除
	auto reply = QMessageBox::question(this, QStringLiteral("确认删除"),
	                                   QStringLiteral("确定要删除主指令 \"%1\" 及其所有配置指令吗？").arg(groupName),
	                                   QMessageBox::Yes | QMessageBox::No);

	if (reply != QMessageBox::Yes)
	{
		return;
	}

	// 从 QTabWidget 中删除该 Tab
	protocolTabs->removeTab(currentGroupIndex);

	// 如果该协议下没有分组了，也删除协议标签页
	if (protocolTabs->count() == 0)
	{
		m_groupTabs->removeTab(currentProtocolIndex);
	}

	// 更新记录列表
	m_records = collectRecordsFromTables();

	QMessageBox::information(this, QStringLiteral("成功"),
	                         QStringLiteral("主指令 \"%1\" 已成功删除").arg(groupName));
}

void ProtocolConfigPage::on_btnAddSubOrder_clicked()
{
	// 获取当前选中的协议标签和分组标签，确定协议类型和分组信息
	int currentProtocolIndex = m_groupTabs->currentIndex();
	if (currentProtocolIndex < 0 || currentProtocolIndex >= m_groupTabs->count())
	{
		QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("请先选择一个协议标签页"));
		return;
	}

	QTabWidget* protocolTabs = qobject_cast<QTabWidget*>(m_groupTabs->widget(currentProtocolIndex));
	if (!protocolTabs)
	{
		QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("无效的协议标签页"));
		return;
	}

	int currentGroupIndex = protocolTabs->currentIndex();
	if (currentGroupIndex < 0 || currentGroupIndex >= protocolTabs->count())
	{
		QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("请先选择一个分组标签页"));
		return;
	}

	// 从标签中提取协议类型和分组信息
	QString protocolTabText = m_groupTabs->tabText(currentProtocolIndex);
	QString protocolType    = protocolTabText;
	if (protocolType.contains("("))
	{
		QRegularExpression      rx("\\(([^)]+)\\)");
		QRegularExpressionMatch match = rx.match(protocolType);
		if (match.hasMatch())
		{
			protocolType = match.captured(1);
		}
	}

	QString                 groupTitle = protocolTabs->tabText(currentGroupIndex);
	QRegularExpression      groupRx("^(.*) (\\d+)$");
	QRegularExpressionMatch groupMatch = groupRx.match(groupTitle);
	QString                 groupName  = groupTitle;
	int                     groupId    = 0;
	if (groupMatch.hasMatch())
	{
		groupName = groupMatch.captured(1);
		groupId   = groupMatch.captured(2).toInt();
	}

	// 打开添加协议对话框
	AddProtocolDialog* addProtocolDialog = new AddProtocolDialog(this);
	addProtocolDialog->setContext(protocolType, groupId, groupName);
	addProtocolDialog->setAttribute(Qt::WA_DeleteOnClose);

	// 连接对话框接受信号
	connect(addProtocolDialog, &AddProtocolDialog::accepted, this,
	        [this, protocolTabs, currentGroupIndex, addProtocolDialog]()
	        {
		        QTableWidget* table = qobject_cast<QTableWidget*>(protocolTabs->widget(currentGroupIndex));
		        if (table)
		        {
			        // 获取对话框中配置的记录
			        QVector<ParameterRecord> newRecords = addProtocolDialog->getRecords();

			        if (newRecords.isEmpty())
			        {
				        return;
			        }

			        const int newCommandId = newRecords.first().commandId;
			        int       checkRow     = 0;
			        while (checkRow < table->rowCount())
			        {
				        const int rowSpan = std::max(table->rowSpan(checkRow, 1), 1);
				        if (QTableWidgetItem* item = table->item(checkRow, 1))
				        {
					        bool      ok                = false;
					        const int existingCommandId = item->text().trimmed().toInt(&ok);
					        if (ok && existingCommandId == newCommandId)
					        {
						        QMessageBox::warning(this, QStringLiteral("添加失败"), QStringLiteral("指令已经存在。添加失败"));
						        return;
					        }
				        }
				        checkRow += rowSpan;
			        }

			        // 添加新记录到表格
			        int startRow = table->rowCount();
			        table->setRowCount(startRow + newRecords.size());

			        for (int i = 0; i < newRecords.size(); ++i)
			        {
				        populateRow(table, startRow + i, newRecords.at(i));
			        }

			        // 重新合并单元格
			        int row = 0;
			        while (row < table->rowCount())
			        {
				        const QString currentCommand = table->item(row, 0) ? table->item(row, 0)->text() : "";
				        int           spanCount      = 1;
				        while (row + spanCount < table->rowCount() &&
					        (table->item(row + spanCount, 0) ? table->item(row + spanCount, 0)->text() : "") ==
					        currentCommand)
				        {
					        ++spanCount;
				        }

				        if (spanCount >= 2)
				        {
					        int colsArray[] = {0, 1, 2, 4, 8, 9, 14};
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

			        // 更新记录列表
			        m_records = collectRecordsFromTables();
		        }
	        });

	addProtocolDialog->exec();
}

void ProtocolConfigPage::on_btnClearAllOrder_clicked()
{
	// 提示用户是否确认清空所有配置信息
	auto reply = QMessageBox::question(this, QStringLiteral("确认清空"),
	                                   QStringLiteral("确定要清空所有配置信息吗？此操作不可恢复。"),
	                                   QMessageBox::Yes | QMessageBox::No);

	if (reply != QMessageBox::Yes)
	{
		return;
	}

	// 清空所有协议标签页和分组标签页
	m_groupTabs->clear();

	// 清空记录列表
	m_records.clear();

	QMessageBox::information(this, QStringLiteral("成功"),
	                         QStringLiteral("所有配置信息已成功清空"));
}

bool ProtocolConfigPage::eventFilter(QObject* obj, QEvent* event)
{
	// 检查是否是鼠标双击事件
	if (event->type() == QEvent::MouseButtonDblClick)
	{
		QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

		// 查找是哪个 tab bar 被点击了
		QTabBar*    clickedTabBar        = nullptr;
		QTabWidget* protocolTabs         = nullptr;
		int         currentProtocolIndex = -1;

		// 遍历所有协议标签页
		for (int protocolIndex = 0; protocolIndex < m_groupTabs->count(); ++protocolIndex)
		{
			QWidget*    widget = m_groupTabs->widget(protocolIndex);
			QTabWidget* tabs   = qobject_cast<QTabWidget*>(widget);
			if (tabs && tabs->tabBar() == obj)
			{
				clickedTabBar        = static_cast<QTabBar*>(obj);
				protocolTabs         = tabs;
				currentProtocolIndex = protocolIndex;
				break;
			}
		}

		if (clickedTabBar && protocolTabs)
		{
			// 确定被点击的 tab 索引
			int tabIndex = clickedTabBar->tabAt(mouseEvent->pos());
			if (tabIndex >= 0 && tabIndex < protocolTabs->count())
			{
				renameMainOrderTab(protocolTabs, tabIndex);
				return true; // 事件已处理
			}
		}
	}

	return QWidget::eventFilter(obj, event);
}

void ProtocolConfigPage::renameMainOrderTab(QTabWidget* protocolTabs, int tabIndex)
{
	// 获取当前 tab 的标题
	QString currentTitle = protocolTabs->tabText(tabIndex);

	// 解析当前标题，获取旧名称和序号
	QString                 oldName;
	int                     oldId = 0;
	QRegularExpression      groupRx("^(.*) (\\d+)$");
	QRegularExpressionMatch groupMatch = groupRx.match(currentTitle);
	if (groupMatch.hasMatch())
	{
		oldName = groupMatch.captured(1);
		oldId   = groupMatch.captured(2).toInt();
	}

	// 显示输入对话框让用户输入新名称
	bool    ok      = false;
	QString newName = QInputDialog::getText(this, QStringLiteral("重命名主指令"),
	                                        QStringLiteral("请输入新的主指令名称:"),
	                                        QLineEdit::Normal, oldName, &ok);

	if (ok && !newName.trimmed().isEmpty() && newName != oldName)
	{
		// 检查新名称是否已存在
		bool nameExists = false;
		for (int i = 0; i < protocolTabs->count(); ++i)
		{
			if (i == tabIndex)
				continue;

			QString                 otherTitle = protocolTabs->tabText(i);
			QString                 otherName;
			int                     otherId    = 0;
			QRegularExpressionMatch otherMatch = groupRx.match(otherTitle);
			if (otherMatch.hasMatch())
			{
				otherName = otherMatch.captured(1);
				otherId   = otherMatch.captured(2).toInt();
			}

			if (otherName == newName)
			{
				nameExists = true;
				break;
			}
		}

		if (nameExists)
		{
			QMessageBox::warning(this, QStringLiteral("警告"),
			                     QStringLiteral("该主指令名称已存在，请使用其他名称"));
			return;
		}

		// 重命名 tab
		protocolTabs->setTabText(tabIndex, groupTabTitle(newName, oldId));

		// 更新记录列表中的 groupName
		m_records = collectRecordsFromTables();

		QMessageBox::information(this, QStringLiteral("成功"),
		                         QStringLiteral("主指令名称已成功修改为 \"%1\"").arg(newName));
	}
}

void ProtocolConfigPage::on_btnDeleteSubOrder_clicked()
{
	// 获取当前选中的协议标签和分组标签
	int currentProtocolIndex = m_groupTabs->currentIndex();
	if (currentProtocolIndex < 0 || currentProtocolIndex >= m_groupTabs->count())
	{
		QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("请先选择一个协议标签页"));
		return;
	}

	QTabWidget* protocolTabs = qobject_cast<QTabWidget*>(m_groupTabs->widget(currentProtocolIndex));
	if (!protocolTabs)
	{
		QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("无效的协议标签页"));
		return;
	}

	int currentGroupIndex = protocolTabs->currentIndex();
	if (currentGroupIndex < 0 || currentGroupIndex >= protocolTabs->count())
	{
		QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("请先选择一个主指令标签页"));
		return;
	}

	QTableWidget* table = qobject_cast<QTableWidget*>(protocolTabs->widget(currentGroupIndex));
	if (!table)
	{
		QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("无效的表格"));
		return;
	}

	int currentRow = table->currentRow();
	if (currentRow < 0 || currentRow >= table->rowCount())
	{
		QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("请先选择一行要删除的子指令"));
		return;
	}

	// 检查是否是合并单元格的行
	int rowSpan = 1;
	// 我们可以通过检查第0列（参数名称）的合并情况来确定是否是合并单元格的行
	if (table->rowSpan(currentRow, 0) > 1)
	{
		rowSpan = table->rowSpan(currentRow, 0);
	}
	else
	{
		// 检查当前行是否是合并单元格的一部分
		for (int i = 0; i < currentRow; ++i)
		{
			if (table->rowSpan(i, 0) > 1 && (i + table->rowSpan(i, 0) > currentRow))
			{
				rowSpan    = table->rowSpan(i, 0);
				currentRow = i;
				break;
			}
		}
	}

	// 提示用户是否确认删除
	QString deleteMessage;
	if (rowSpan > 1)
	{
		deleteMessage = QStringLiteral("确定要删除这 %1 行合并的子指令吗？").arg(rowSpan);
	}
	else
	{
		deleteMessage = QStringLiteral("确定要删除这一行子指令吗？");
	}

	auto reply = QMessageBox::question(this, QStringLiteral("确认删除"),
	                                   deleteMessage,
	                                   QMessageBox::Yes | QMessageBox::No);

	if (reply != QMessageBox::Yes)
	{
		return;
	}

	// 删除选中的行（或合并的多行）
	for (int i = 0; i < rowSpan; ++i)
	{
		table->removeRow(currentRow);
	}

	// 更新记录列表
	m_records = collectRecordsFromTables();

	// 显示成功消息
	QString successMessage;
	if (rowSpan > 1)
	{
		successMessage = QStringLiteral("成功删除了 %1 行合并的子指令").arg(rowSpan);
	}
	else
	{
		successMessage = QStringLiteral("成功删除了一行子指令");
	}

	QMessageBox::information(this, QStringLiteral("成功"), successMessage);
}
