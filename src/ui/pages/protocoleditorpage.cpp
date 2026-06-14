#include "protocoleditorpage.h"
#include "ui_protocoleditorpage.h"
#include "models/binprotocolformat.h"
#include "models/protocolfieldtablemodel.h"
#include "ui/theme/thememanager.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableView>

namespace {
// 类型下拉项，与 ProtocolFieldTableModel::stringToType 对应。
const char *kTypeNames[] = {
    "UInt8", "UInt16", "UInt32", "Int8", "Int16", "Int32",
    "Float32", "Float64", "ByteArray", "BitField", "String", "Struct"
};
}

ProtocolEditorPage::ProtocolEditorPage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::ProtocolEditorPage)
{
    ui->setupUi(this);
    setStyleSheet(ThemeManager::contentPageStyle(ThemeManager::palette(ThemeKind::IndustrialBlue)));

    m_model = new ProtocolFieldTableModel(this);
    ui->fieldsTable->setModel(m_model);
    ui->fieldsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->fieldsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->fieldsTable->horizontalHeader()->setStretchLastSection(true);

    for (const char *t : kTypeNames) {
        ui->typeCombo->addItem(QString::fromLatin1(t));
    }

    // 工具栏。
    connect(ui->newButton, &QPushButton::clicked, this, &ProtocolEditorPage::newProtocol);
    connect(ui->openButton, &QPushButton::clicked, this, &ProtocolEditorPage::openFile);
    connect(ui->saveButton, &QPushButton::clicked, this, &ProtocolEditorPage::saveFile);
    connect(ui->saveAsButton, &QPushButton::clicked, this, &ProtocolEditorPage::saveAsFile);
    connect(ui->sampleButton, &QPushButton::clicked, this, &ProtocolEditorPage::loadSample);

    // 字段操作。
    connect(ui->addButton, &QPushButton::clicked, this, [this]() {
        FieldDef f;
        f.name = QStringLiteral("field%1").arg(m_model->rowCount() + 1);
        f.type = FieldType::UInt8;
        f.byteLength = 1;
        m_model->addField(f);
        refreshFrameSize();
        // 选中并滚动到新行。
        const int row = m_model->rowCount() - 1;
        ui->fieldsTable->selectRow(row);
        onSelectionChanged();
    });
    connect(ui->removeButton, &QPushButton::clicked, this, [this]() {
        const QModelIndex idx = ui->fieldsTable->currentIndex();
        if (idx.isValid()) {
            m_model->removeRows(idx.row(), 1);
            refreshFrameSize();
        }
    });
    connect(ui->upButton, &QPushButton::clicked, this, [this]() {
        const QModelIndex idx = ui->fieldsTable->currentIndex();
        if (idx.isValid()) {
            m_model->moveRow(idx.row(), true);
            refreshFrameSize();
        }
    });
    connect(ui->downButton, &QPushButton::clicked, this, [this]() {
        const QModelIndex idx = ui->fieldsTable->currentIndex();
        if (idx.isValid()) {
            m_model->moveRow(idx.row(), false);
            refreshFrameSize();
        }
    });

    // 选中行 → 镜像到详情面板。
    connect(ui->fieldsTable->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex &) { onSelectionChanged(); });

    // 应用详情面板回写表格。
    connect(ui->applyDetailButton, &QPushButton::clicked, this, &ProtocolEditorPage::applyDetailToCurrentRow);

    // 模型变化时刷新帧大小。
    connect(m_model, &ProtocolFieldTableModel::dataChanged, this, [this]() { refreshFrameSize(); });

    // 初始为空协议。
    newProtocol();
}

ProtocolEditorPage::~ProtocolEditorPage()
{
    delete ui;
}

void ProtocolEditorPage::newProtocol()
{
    BinProtocol proto;
    proto.magic = QStringLiteral("APROTO01");
    proto.version = 1;
    m_loader.setProtocol(proto);
    m_model->setFields(proto.fields);
    setFilePath(QString());
    refreshFrameSize();
}

void ProtocolEditorPage::openFile()
{
    const QString startDir = QCoreApplication::applicationDirPath() + QStringLiteral("/../data/protocols");
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("打开协议 .bin"),
        QDir(startDir).exists() ? startDir : QString(),
        QStringLiteral("协议描述 (*.bin);;所有文件 (*.*)"));
    if (path.isEmpty()) {
        return;
    }
    if (!m_loader.load(path)) {
        ui->filePathLabel->setText(QStringLiteral("打开失败：%1").arg(m_loader.lastError()));
        return;
    }
    m_model->setFields(m_loader.protocol().fields);
    setFilePath(path);
    refreshFrameSize();
}

void ProtocolEditorPage::saveFile()
{
    if (m_filePath.isEmpty()) {
        saveAsFile();
        return;
    }
    BinProtocol proto = m_loader.protocol();
    proto.fields = m_model->fields();
    m_loader.setProtocol(proto);
    if (m_loader.save(m_filePath)) {
        ui->filePathLabel->setText(m_filePath);
    } else {
        ui->filePathLabel->setText(QStringLiteral("保存失败。"));
    }
}

void ProtocolEditorPage::saveAsFile()
{
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("另存为 .bin"),
        QStringLiteral("protocol.bin"),
        QStringLiteral("协议描述 (*.bin)"));
    if (path.isEmpty()) {
        return;
    }
    BinProtocol proto = m_loader.protocol();
    proto.fields = m_model->fields();
    m_loader.setProtocol(proto);
    if (m_loader.save(path)) {
        setFilePath(path);
    } else {
        ui->filePathLabel->setText(QStringLiteral("保存失败。"));
    }
}

void ProtocolEditorPage::loadSample()
{
    BinProtocol proto = BinProtocolLoader::makeSample();
    m_loader.setProtocol(proto);
    m_model->setFields(proto.fields);
    setFilePath(QString());
    ui->filePathLabel->setText(QStringLiteral("（示例协议，未保存）"));
    refreshFrameSize();
}

void ProtocolEditorPage::applyDetailToCurrentRow()
{
    const QModelIndex idx = ui->fieldsTable->currentIndex();
    if (!idx.isValid()) {
        return;
    }
    const int row = idx.row();
    // 用 removeRows + addField 的方式回写到指定行，保持顺序。
    QVector<FieldDef> fields = m_model->fields();
    if (row < 0 || row >= fields.size()) {
        return;
    }
    FieldDef &f = fields[row];
    f.name = ui->nameEdit->text();
    bool ok = false;
    f.type = ProtocolFieldTableModel::stringToType(ui->typeCombo->currentText(), &ok);
    f.byteOffset = quint32(ui->offsetSpin->value());
    f.byteLength = quint16(ui->lengthSpin->value());
    f.bitOffset = quint8(ui->bitOffsetSpin->value());
    f.bitLength = quint8(ui->bitLengthSpin->value());
    f.comment = ui->commentEdit->text();
    m_model->setFields(fields);
    // 保持选中。
    ui->fieldsTable->selectRow(row);
    refreshFrameSize();
}

void ProtocolEditorPage::onSelectionChanged()
{
    const QModelIndex idx = ui->fieldsTable->currentIndex();
    if (!idx.isValid()) {
        return;
    }
    const FieldDef f = m_model->fieldAt(idx.row());
    ui->nameEdit->setText(f.name);
    ui->typeCombo->setCurrentText(ProtocolFieldTableModel::typeToString(f.type));
    ui->offsetSpin->setValue(int(f.byteOffset));
    ui->lengthSpin->setValue(int(f.byteLength));
    ui->bitOffsetSpin->setValue(int(f.bitOffset));
    ui->bitLengthSpin->setValue(int(f.bitLength));
    ui->commentEdit->setText(f.comment);
}

void ProtocolEditorPage::refreshFrameSize()
{
    BinProtocol proto = m_loader.protocol();
    proto.fields = m_model->fields();
    const quint32 size = proto.frameSize();
    ui->frameSizeLabel->setText(QStringLiteral("帧大小：%1 字节").arg(size));
}

void ProtocolEditorPage::setFilePath(const QString &path)
{
    m_filePath = path;
    ui->filePathLabel->setText(path.isEmpty() ? QStringLiteral("（未保存）") : path);
}
