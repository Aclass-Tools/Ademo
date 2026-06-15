#include "commandoperationpage.h"
#include "ui_commandoperationpage.h"
#include "models/framecodec.h"
#include "services/connectionmanager.h"
#include "ui/theme/thememanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>

// 字段类型枚举转字符串（文件内静态工具）。
namespace {
QString fieldTypeToString(FieldType t)
{
    switch (t) {
    case FieldType::UInt8: return QStringLiteral("UInt8");
    case FieldType::UInt16: return QStringLiteral("UInt16");
    case FieldType::UInt32: return QStringLiteral("UInt32");
    case FieldType::Int8: return QStringLiteral("Int8");
    case FieldType::Int16: return QStringLiteral("Int16");
    case FieldType::Int32: return QStringLiteral("Int32");
    case FieldType::Float32: return QStringLiteral("Float32");
    case FieldType::Float64: return QStringLiteral("Float64");
    case FieldType::ByteArray: return QStringLiteral("ByteArray");
    case FieldType::BitField: return QStringLiteral("BitField");
    case FieldType::String: return QStringLiteral("String");
    case FieldType::Struct: return QStringLiteral("Struct");
    }
    return QStringLiteral("Unknown");
}
} // namespace

CommandOperationPage::CommandOperationPage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::CommandOperationPage)
{
    ui->setupUi(this);
    setStyleSheet(ThemeManager::contentPageStyle(ThemeManager::palette(ThemeKind::IndustrialBlue)));

    // 工具栏：在标题右侧加"加载.bin""加载示例"，配合已有的"总召""总写"。
    // （ui 文件里只有 titleLabel/readAllButton/writeAllButton，这里动态插入加载按钮。）
    auto *loadBinBtn = new QPushButton(QStringLiteral("加载 .bin"), this);
    auto *sampleBtn = new QPushButton(QStringLiteral("加载示例"), this);
    ui->horizontalLayoutToolbar->insertWidget(ui->horizontalLayoutToolbar->count() - 2, sampleBtn);
    ui->horizontalLayoutToolbar->insertWidget(ui->horizontalLayoutToolbar->count() - 2, loadBinBtn);

    connect(loadBinBtn, &QPushButton::clicked, this, [this]() {
        const QString startDir = QCoreApplication::applicationDirPath() + QStringLiteral("/../data/protocols");
        const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择协议 .bin"),
            QDir(startDir).exists() ? startDir : QString(),
            QStringLiteral("协议描述 (*.bin);;所有文件 (*.*)"));
        if (!path.isEmpty()) {
            loadFromPath(path);
        }
    });
    connect(sampleBtn, &QPushButton::clicked, this, &CommandOperationPage::loadSample);
    connect(ui->readAllButton, &QPushButton::clicked, this, &CommandOperationPage::onReadAllClicked);
    connect(ui->writeAllButton, &QPushButton::clicked, this, &CommandOperationPage::onWriteAllClicked);
}

CommandOperationPage::~CommandOperationPage()
{
    delete ui;
}

void CommandOperationPage::setConnectionManager(services::ConnectionManager *mgr)
{
    m_conn = mgr;
    if (m_conn) {
        connect(m_conn, &services::ConnectionManager::frameReceived,
                this, &CommandOperationPage::onFrameReceived);
    }
}

void CommandOperationPage::onPageActivated()
{
    // 进入页面时若未加载，自动加载示例，便于立即使用。
    if (!m_loader.isLoaded()) {
        loadSample();
    }
}

void CommandOperationPage::loadFromPath(const QString &path)
{
    if (m_loader.load(path)) {
        rebuildTable();
    }
}

void CommandOperationPage::loadSample()
{
    m_loader.setProtocol(BinProtocolLoader::makeSample());
    rebuildTable();
}

void CommandOperationPage::rebuildTable()
{
    m_rowFields.clear();
    m_valueEdits.clear();

    // 单 tab 全部字段（后续可按类型/分组拆 tab）。
    QTableWidget *table = nullptr;
    if (ui->groupTabs->count() == 0) {
        table = new QTableWidget(this);
        table->setColumnCount(6);
        table->setHorizontalHeaderLabels(
            { QStringLiteral("字段"), QStringLiteral("类型"), QStringLiteral("偏移"),
              QStringLiteral("长度"), QStringLiteral("值"), QStringLiteral("操作") });
        table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
        ui->groupTabs->addTab(table, QStringLiteral("全部参数"));
    } else {
        table = qobject_cast<QTableWidget *>(ui->groupTabs->widget(0));
    }
    table->setRowCount(0);

    const QVector<FieldDef> &fields = m_loader.protocol().fields;
    int row = 0;
    for (const FieldDef &f : fields) {
        if (f.type == FieldType::Struct) {
            // Struct：展开子字段，每行一个。
            for (const FieldDef &c : f.children) {
                FieldDef shifted = c;
                shifted.byteOffset = f.byteOffset + c.byteOffset;
                m_rowFields.append(shifted);
                table->insertRow(row);
                table->setItem(row, 0, new QTableWidgetItem(QStringLiteral("%1.%2").arg(f.name, c.name)));
                table->setItem(row, 1, new QTableWidgetItem(fieldTypeToString(c.type)));
                table->setItem(row, 2, new QTableWidgetItem(QString::number(shifted.byteOffset)));
                table->setItem(row, 3, new QTableWidgetItem(QString::number(c.byteLength)));
                auto *edit = new QLineEdit(table);
                edit->setText(QStringLiteral("0"));
                table->setCellWidget(row, 4, edit);
                m_valueEdits.insert(row, edit);
                // 操作按钮。
                auto *w = new QWidget(table);
                auto *hl = new QHBoxLayout(w);
                hl->setContentsMargins(2, 0, 2, 0);
                auto *readBtn = new QPushButton(QStringLiteral("读"), w);
                auto *writeBtn = new QPushButton(QStringLiteral("写"), w);
                hl->addWidget(readBtn);
                hl->addWidget(writeBtn);
                table->setCellWidget(row, 5, w);
                const int capturedRow = row;
                connect(readBtn, &QPushButton::clicked, this, [this, capturedRow]() { sendRead(capturedRow); });
                connect(writeBtn, &QPushButton::clicked, this, [this, capturedRow]() { sendWrite(capturedRow); });
                ++row;
            }
        } else {
            m_rowFields.append(f);
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(f.name));
            table->setItem(row, 1, new QTableWidgetItem(fieldTypeToString(f.type)));
            table->setItem(row, 2, new QTableWidgetItem(QString::number(f.byteOffset)));
            table->setItem(row, 3, new QTableWidgetItem(QString::number(f.byteLength)));
            auto *edit = new QLineEdit(table);
            edit->setText(QStringLiteral("0"));
            table->setCellWidget(row, 4, edit);
            m_valueEdits.insert(row, edit);
            auto *w = new QWidget(table);
            auto *hl = new QHBoxLayout(w);
            hl->setContentsMargins(2, 0, 2, 0);
            auto *readBtn = new QPushButton(QStringLiteral("读"), w);
            auto *writeBtn = new QPushButton(QStringLiteral("写"), w);
            hl->addWidget(readBtn);
            hl->addWidget(writeBtn);
            table->setCellWidget(row, 5, w);
            const int capturedRow = row;
            connect(readBtn, &QPushButton::clicked, this, [this, capturedRow]() { sendRead(capturedRow); });
            connect(writeBtn, &QPushButton::clicked, this, [this, capturedRow]() { sendWrite(capturedRow); });
            ++row;
        }
    }
}

void CommandOperationPage::sendRead(int row)
{
    if (!m_conn || !m_conn->isConnected()) {
        return;
    }
    const FieldDef &f = m_rowFields.value(row);
    // mainCommand = 偏移高位，subCommand = 偏移低位，作为寻址该字段的约定。
    const QByteArray frame = services::ProtocolParser::buildReadFrame(
        services::kProtocolA0, m_sourceId, m_targetId,
        quint8((f.byteOffset >> 8) & 0xFF), quint8(f.byteOffset & 0xFF));
    m_conn->sendFrame(frame);
}

void CommandOperationPage::sendWrite(int row)
{
    if (!m_conn || !m_conn->isConnected()) {
        return;
    }
    const FieldDef &f = m_rowFields.value(row);
    QLineEdit *edit = m_valueEdits.value(row);
    const QString text = edit ? edit->text() : QString();
    const QByteArray payload = encodeFieldValue(f, text);
    const QByteArray frame = services::ProtocolParser::buildWriteFrame(
        services::kProtocolA0, m_sourceId, m_targetId,
        quint8((f.byteOffset >> 8) & 0xFF), quint8(f.byteOffset & 0xFF), payload);
    m_conn->sendFrame(frame);
}

void CommandOperationPage::onReadAllClicked()
{
    for (int row = 0; row < m_rowFields.size(); ++row) {
        sendRead(row);
    }
}

void CommandOperationPage::onWriteAllClicked()
{
    for (int row = 0; row < m_rowFields.size(); ++row) {
        sendWrite(row);
    }
}

void CommandOperationPage::onFrameReceived(const QByteArray &frame)
{
    if (!m_loader.isLoaded()) {
        return;
    }
    const services::ProtocolFrame pf = services::ProtocolParser::parseFrame(frame);
    if (!pf.isValid) {
        return;
    }
    // 用 mainCommand/subCommand 还原字段偏移，定位行并回填值。
    const quint32 offset = (quint32(pf.mainCommand) << 8) | pf.subCommand;
    QTableWidget *table = qobject_cast<QTableWidget *>(ui->groupTabs->widget(0));
    if (!table) {
        return;
    }
    for (int row = 0; row < m_rowFields.size(); ++row) {
        const FieldDef &f = m_rowFields[row];
        if (f.byteOffset == offset && f.byteLength > 0 && pf.payload.size() >= int(f.byteLength)) {
            const QString val = decodeFieldValue(f, pf.payload, 0);
            if (QTableWidgetItem *item = table->item(row, 4)) {
                item->setText(val);
            } else if (QLineEdit *edit = m_valueEdits.value(row)) {
                edit->setText(val);
            }
            break;
        }
    }
}

QByteArray CommandOperationPage::encodeFieldValue(const FieldDef &f, const QString &text) const
{
    // 复用 FrameCodec：构造一个仅含该字段的 values 表，再按字段表 encode。
    BinProtocol single;
    single.magic = QStringLiteral("APROTO01");
    FieldDef only = f;
    single.fields.append(only);
    QMap<QString, QVariant> values;
    values.insert(f.name, QVariant(text));
    return FrameCodec::encode(single, values);
}

QString CommandOperationPage::decodeFieldValue(const FieldDef &f, const QByteArray &payload, int offset) const
{
    BinProtocol single;
    single.magic = QStringLiteral("APROTO01");
    FieldDef only = f;
    only.byteOffset = offset;
    single.fields.append(only);
    const QMap<QString, QVariant> decoded = FrameCodec::decode(single, payload);
    return decoded.value(f.name).toString();
}
