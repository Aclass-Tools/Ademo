#include "ui/AddProtocolDialog.h"

#include <algorithm>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>

#include "models/ParameterRecord.h"
#include "services/ProtocolParser.h"
#include "ui/delegates/ComboBoxDelegate.h"

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

    static const int s_mergeColumns[]   = {0, 1, 2, 4, 8, 9, 14};
    static const int s_mergeColumnCount = sizeof(s_mergeColumns) / sizeof(s_mergeColumns[0]);
}

AddProtocolDialog::AddProtocolDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("添加协议"));
    setMinimumSize(1000, 600);
    initUI();
}

AddProtocolDialog::~AddProtocolDialog()
{
}

void AddProtocolDialog::setContext(const QString& protocolType, int groupId, const QString& groupName)
{
    m_protocolType = protocolType;
    m_groupId      = groupId;
    m_groupName    = groupName;
    m_statusLabel->setText(QStringLiteral("协议: %1, 组: %2 (%3)").arg(protocolType).arg(groupName).arg(groupId));

    if (m_tableWidget != nullptr)
    {
        if (protocolStringToByte(m_protocolType) == PROTOCOL_A3)
        {
            m_tableWidget->setItemDelegateForColumn(14, m_remarkDelegate);
            for (int row = 0; row < m_tableWidget->rowCount(); ++row)
            {
                QTableWidgetItem* remarkItem = m_tableWidget->item(row, 14);
                if (remarkItem && remarkItem->text().trimmed().isEmpty())
                {
                    remarkItem->setText(QStringLiteral("绘图"));
                }
            }
        }
        else
        {
            m_tableWidget->setItemDelegateForColumn(14, nullptr);
        }
    }
}

QVector<ParameterRecord> AddProtocolDialog::getRecords() const
{
    return m_records;
}

QStringList AddProtocolDialog::getPermissionOptions()
{
    return {QStringLiteral("R"), QStringLiteral("W"), QStringLiteral("RW")};
}

QStringList AddProtocolDialog::getDataTypeOptions()
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

void AddProtocolDialog::initUI()
{
    // 初始化 delegates
    m_permissionDelegate = new ComboBoxDelegate(this);
    m_permissionDelegate->setOptions(getPermissionOptions());

    m_dataTypeDelegate = new ComboBoxDelegate(this);
    m_dataTypeDelegate->setOptions(getDataTypeOptions());

    m_remarkDelegate = new ComboBoxDelegate(this);
    m_remarkDelegate->setOptions({QStringLiteral("绘图"), QStringLiteral("不绘图")});

    auto* mainLayout = new QVBoxLayout(this);

    // 顶部状态和行数输入区域
    auto* topLayout = new QHBoxLayout();

    m_statusLabel = new QLabel(QStringLiteral("请设置协议行数"));
    m_statusLabel->setStyleSheet(QStringLiteral("font-weight: bold; padding: 8px;"));
    topLayout->addWidget(m_statusLabel);

    topLayout->addStretch();

    topLayout->addWidget(new QLabel(QStringLiteral("指令行数:")));

    m_rowCountEdit = new QLineEdit(QStringLiteral("1"));
    m_rowCountEdit->setMaximumWidth(80);
    connect(m_rowCountEdit, &QLineEdit::textChanged, this, &AddProtocolDialog::onRowCountChanged);
    topLayout->addWidget(m_rowCountEdit);

    mainLayout->addLayout(topLayout);

    // 表格区域
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(kHeaders().size());
    m_tableWidget->setHorizontalHeaderLabels(kHeaders());
    m_tableWidget->setAlternatingRowColors(true);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_tableWidget->verticalHeader()->setVisible(true);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableWidget->horizontalHeader()->setSectionsMovable(true);
    m_tableWidget->setEditTriggers(
        QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
    m_tableWidget->setStyleSheet(QStringLiteral(
        "QTableWidget { border: none; background: white; gridline-color: #d9e2ec; }"
        "QHeaderView::section { background: #a8d3e3; color: #0f172a; padding: 6px; border: 1px solid #7faec0; }"));

    // 为权限列（第2列）设置 delegate
    m_tableWidget->setItemDelegateForColumn(2, m_permissionDelegate);
    // 为数据类型列（第4列）设置 delegate
    m_tableWidget->setItemDelegateForColumn(4, m_dataTypeDelegate);

    mainLayout->addWidget(m_tableWidget);

    // 底部按钮区域
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_cancelButton = new QPushButton(QStringLiteral("取消"));
    connect(m_cancelButton, &QPushButton::clicked, this, &AddProtocolDialog::onCancelButtonClicked);
    buttonLayout->addWidget(m_cancelButton);

    m_okButton = new QPushButton(QStringLiteral("确定"));
    m_okButton->setStyleSheet(
        QStringLiteral("background: #4CAF50; color: white; padding: 8px 16px; border: none; border-radius: 4px;"));
    connect(m_okButton, &QPushButton::clicked, this, &AddProtocolDialog::onOkButtonClicked);
    buttonLayout->addWidget(m_okButton);

    mainLayout->addLayout(buttonLayout);

    updateTable();
}

void AddProtocolDialog::setupTable()
{
    m_tableWidget->setRowCount(m_rowCount);

    for (int row = 0; row < m_rowCount; ++row)
    {
        for (int col = 0; col < kHeaders().size(); ++col)
        {
            auto* item = new QTableWidgetItem("");
            item->setFlags(item->flags() | Qt::ItemIsEditable);
            m_tableWidget->setItem(row, col, item);
        }
        // 为权限列设置默认值
        if (m_tableWidget->item(row, 2))
        {
            m_tableWidget->item(row, 2)->setText("R");
        }
        // 为数据类型列设置默认值
        if (m_tableWidget->item(row, 4))
        {
            m_tableWidget->item(row, 4)->setText("uint8_t");
        }
        if (protocolStringToByte(m_protocolType) == PROTOCOL_A3 && m_tableWidget->item(row, 14))
        {
            m_tableWidget->item(row, 14)->setText(QStringLiteral("绘图"));
        }
    }
}

void AddProtocolDialog::updateTable()
{
    setupTable();
    mergeCells();
}

void AddProtocolDialog::mergeCells()
{
    // 第一行作为合并的基准，所有行合并显示
    if (m_rowCount >= 2)
    {
        for (int i = 0; i < s_mergeColumnCount; ++i)
        {
            int col = s_mergeColumns[i];
            m_tableWidget->setSpan(0, col, m_rowCount, 1);

            // 清除下面行的内容
            for (int subRow = 1; subRow < m_rowCount; ++subRow)
            {
                if (m_tableWidget->item(subRow, 0) != nullptr)
                {
                    m_tableWidget->item(subRow, 0)->setText(QString());
                }
            }
        }
    }
}

void AddProtocolDialog::onRowCountChanged(const QString& text)
{
    bool ok    = false;
    int  count = text.toInt(&ok);
    if (ok && count >= 1 && count <= 100)
    {
        m_rowCount = count;
        updateTable();
    }
}

void AddProtocolDialog::onOkButtonClicked()
{
    if (!validateRecords())
    {
        return;
    }

    // 从表格读取记录
    m_records.clear();

    QString parameterName   = m_tableWidget->item(0, 0) ? m_tableWidget->item(0, 0)->text() : "";
    QString commandIdStr    = m_tableWidget->item(0, 1) ? m_tableWidget->item(0, 1)->text() : "";
    QString permission      = m_tableWidget->item(0, 2) ? m_tableWidget->item(0, 2)->text() : "R";
    QString dataType        = m_tableWidget->item(0, 4) ? m_tableWidget->item(0, 4)->text() : "";
    QString dataLengthStr   = m_tableWidget->item(0, 5) ? m_tableWidget->item(0, 5)->text() : "";
    QString unit            = m_tableWidget->item(0, 8) ? m_tableWidget->item(0, 8)->text() : "";
    QString storageLocation = m_tableWidget->item(0, 9) ? m_tableWidget->item(0, 9)->text() : "";

    bool ok        = false;
    int  commandId = commandIdStr.toInt(&ok);
    if (!ok)
    {
        QMessageBox::warning(this, QStringLiteral("输入错误"), QStringLiteral("子命令ID必须是数字"));
        return;
    }

    int dataLength = 1;
    if (!dataLengthStr.isEmpty())
    {
        dataLength = dataLengthStr.toInt(&ok);
        if (!ok || dataLength <= 0)
        {
            QMessageBox::warning(this, QStringLiteral("输入错误"), QStringLiteral("数据长度必须是正整数"));
            return;
        }
    }

    for (int row = 0; row < m_rowCount; ++row)
    {
        ParameterRecord record;
        record.protocolType    = protocolStringToByte(m_protocolType);
        record.groupId         = m_groupId;
        record.groupName       = m_groupName;
        record.parameterName   = parameterName;
        record.commandName     = parameterName;
        record.commandId       = commandId;
        record.permission      = permission;
        record.readIndex       = row + 1;
        record.dataType        = dataType;
        record.dataLength      = dataLength;
        record.unit            = unit;
        record.storageLocation = storageLocation;

        record.description  = m_tableWidget->item(row, 6) ? m_tableWidget->item(row, 6)->text() : "";
        record.defaultValue = m_tableWidget->item(row, 7) ? m_tableWidget->item(row, 7)->text() : "";
        record.maxValue     = m_tableWidget->item(row, 10) ? m_tableWidget->item(row, 10)->text() : "";
        record.minValue     = m_tableWidget->item(row, 11) ? m_tableWidget->item(row, 11)->text() : "";
        record.step         = m_tableWidget->item(row, 12) ? m_tableWidget->item(row, 12)->text() : "";

        QString enumStr = m_tableWidget->item(row, 13) ? m_tableWidget->item(row, 13)->text() : "";
        if (!enumStr.isEmpty())
        {
            record.enumValues = enumStr.split(" | ", Qt::SkipEmptyParts);
        }

        record.remark = m_tableWidget->item(row, 14) ? m_tableWidget->item(row, 14)->text() : "";

        m_records.push_back(record);
    }

    accept();
}

void AddProtocolDialog::onCancelButtonClicked()
{
    reject();
}

bool AddProtocolDialog::validateRecords()
{
    // 检查参数名称是否为空
    QString parameterName = m_tableWidget->item(0, 0) ? m_tableWidget->item(0, 0)->text() : "";
    if (parameterName.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("输入错误"), QStringLiteral("参数名称不能为空"));
        return false;
    }

    // 检查子命令ID是否为空
    QString commandIdStr = m_tableWidget->item(0, 1) ? m_tableWidget->item(0, 1)->text() : "";
    if (commandIdStr.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("输入错误"), QStringLiteral("子命令ID不能为空"));
        return false;
    }

    // 检查子命令ID是否有效
    bool ok        = false;
    int  commandId = commandIdStr.toInt(&ok);
    if (!ok || commandId <= 0 || commandId > 0xFF)
    {
        QMessageBox::warning(this, QStringLiteral("输入错误"), QStringLiteral("子命令ID必须是1-255的整数"));
        return false;
    }

    return true;
}
