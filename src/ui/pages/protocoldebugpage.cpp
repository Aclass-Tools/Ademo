#include "protocoldebugpage.h"
#include "ui_protocoldebugpage.h"
#include "models/binprotocolformat.h"
#include "models/framecodec.h"
#include "models/protocolfieldtablemodel.h"
#include "services/connectionmanager.h"
#include "ui/theme/thememanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableView>
#include <QTableWidgetItem>
#include <QVBoxLayout>

ProtocolDebugPage::ProtocolDebugPage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::ProtocolDebugPage)
{
    ui->setupUi(this);
    setStyleSheet(ThemeManager::contentPageStyle(ThemeManager::palette(ThemeKind::IndustrialBlue)));

    m_model = new ProtocolFieldTableModel(this);
    ui->fieldsTable->setModel(m_model);
    ui->fieldsTable->horizontalHeader()->setStretchLastSection(true);
    ui->fieldsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->decodedTable->horizontalHeader()->setStretchLastSection(true);

    // 在 encodeInputsHost 上建立 QFormLayout，供动态字段输入使用。
    auto *form = new QFormLayout(ui->encodeInputsHost);
    form->setLabelAlignment(Qt::AlignRight);
    auto *hostLayout = new QVBoxLayout(ui->encodeInputsHost);
    hostLayout->setContentsMargins(0, 0, 0, 0);
    hostLayout->addLayout(form);
    hostLayout->addStretch(1);
    form->setObjectName(QStringLiteral("encodeForm"));

    connect(ui->loadBinButton, &QPushButton::clicked, this, [this]() {
        const QString startDir = QCoreApplication::applicationDirPath() + QStringLiteral("/../data/protocols");
        const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择协议 .bin"),
            QDir(startDir).exists() ? startDir : QString(),
            QStringLiteral("协议描述 (*.bin);;所有文件 (*.*)"));
        if (!path.isEmpty()) {
            loadFromPath(path);
        }
    });
    connect(ui->loadSampleButton, &QPushButton::clicked, this, &ProtocolDebugPage::loadSample);
    connect(ui->encodeButton, &QPushButton::clicked, this, &ProtocolDebugPage::onEncodeClicked);
    connect(ui->decodeButton, &QPushButton::clicked, this, &ProtocolDebugPage::onDecodeClicked);
    connect(ui->decodeClearButton, &QPushButton::clicked, ui->decodedTable, &QTableWidget::clearContents);
    connect(ui->decodeClearButton, &QPushButton::clicked, ui->decodeHexEdit, &QLineEdit::clear);

    // 串口配置区已迁移到「设备连接」页：隐藏本页相关控件。
    ui->sendViaSerialCheck->setVisible(false);
    ui->refreshPortsButton->setVisible(false);
    ui->portCombo->setVisible(false);
}

ProtocolDebugPage::~ProtocolDebugPage()
{
    delete ui;
}

void ProtocolDebugPage::setConnectionManager(services::ConnectionManager *mgr)
{
    m_conn = mgr;
    if (m_conn) {
        // 收到完整帧 → 去封装 → 解 payload → 刷新解帧表。
        connect(m_conn, &services::ConnectionManager::frameReceived,
                this, &ProtocolDebugPage::onFrameReceived);
    }
}

void ProtocolDebugPage::onPageActivated()
{
    if (!m_loader.isLoaded()) {
        ui->filePathLabel->setText(QStringLiteral("（未加载，点击“加载示例”快速开始）"));
    }
}

void ProtocolDebugPage::loadFromPath(const QString &path)
{
    if (!m_loader.load(path)) {
        ui->filePathLabel->setText(QStringLiteral("加载失败：%1").arg(m_loader.lastError()));
        return;
    }
    m_model->setFields(m_loader.protocol().fields);
    ui->filePathLabel->setText(path);
    rebuildEncodeInputs();
}

void ProtocolDebugPage::loadSample()
{
    BinProtocol proto = BinProtocolLoader::makeSample();
    m_loader.setProtocol(proto);
    m_model->setFields(proto.fields);
    ui->filePathLabel->setText(QStringLiteral("（示例协议）"));
    rebuildEncodeInputs();
}

void ProtocolDebugPage::rebuildEncodeInputs()
{
    auto *form = ui->encodeInputsHost->findChild<QFormLayout *>(QStringLiteral("encodeForm"));
    if (!form) {
        return;
    }
    while (form->rowCount() > 0) {
        form->removeRow(0);
    }
    m_inputs.clear();

    const QVector<FieldDef> &fields = m_loader.protocol().fields;
    for (const FieldDef &f : fields) {
        if (f.type == FieldType::Struct) {
            for (const FieldDef &c : f.children) {
                const QString key = QStringLiteral("%1.%2").arg(f.name, c.name);
                auto *edit = new QLineEdit(ui->encodeInputsHost);
                edit->setText(QStringLiteral("0"));
                form->addRow(QStringLiteral("%1：%2").arg(f.name, c.name), edit);
                m_inputs.insert(key, edit);
            }
        } else {
            auto *edit = new QLineEdit(ui->encodeInputsHost);
            edit->setText(f.type == FieldType::String ? QStringLiteral("hello")
                : (f.type == FieldType::ByteArray ? QStringLiteral("00 11 22 33 44 55")
                : QStringLiteral("0")));
            form->addRow(f.name, edit);
            m_inputs.insert(f.name, edit);
        }
    }
}

void ProtocolDebugPage::onEncodeClicked()
{
    if (!m_loader.isLoaded()) {
        ui->encodedHexEdit->setText(QStringLiteral("请先加载协议。"));
        return;
    }
    // 1) 字段表层：字段值 → payload。
    QMap<QString, QVariant> values;
    for (auto it = m_inputs.constBegin(); it != m_inputs.constEnd(); ++it) {
        values.insert(it.key(), it.value()->text());
    }
    const QByteArray payload = FrameCodec::encode(m_loader.protocol(), values);

    // 2) 传输封装层：按协议自带的 transport 选择封装。
    const TransportProtocol tp = m_loader.protocol().transport;
    if (tp == TransportProtocol::Modbus) {
        // Modbus 协议不走 ProtocolParser；提示用指令操作页或 Modbus 专用接口。
        ui->encodedHexEdit->setText(
            FrameCodec::toHexDisplay(payload) + QStringLiteral("   [Modbus 协议：payload 见上，请经指令操作页读写寄存器]"));
        return;
    }
    const quint8 protoByte = (tp == TransportProtocol::A3) ? services::kProtocolA3 : services::kProtocolA0;
    const QByteArray frame = services::ProtocolParser::buildWriteFrame(
        protoByte, m_sourceId, m_targetId,
        /*mainCommand*/ 0x00, /*subCommand*/ 0x00, payload);
    const QString hex = FrameCodec::toHexDisplay(frame);
    ui->encodedHexEdit->setText(hex);

    // 3) 传输层：经全局连接发送（未连接则仅显示组包结果，便于离线调试）。
    if (m_conn && m_conn->isConnected()) {
        QString err;
        if (m_conn->sendFrame(frame, &err)) {
            ui->encodedHexEdit->setText(hex + QStringLiteral("   [已发送 %1 字节]").arg(frame.size()));
        } else {
            ui->encodedHexEdit->setText(hex + QStringLiteral("   [发送失败] %1").arg(err));
        }
    } else {
        ui->encodedHexEdit->setText(hex + QStringLiteral("   [未连接，仅组包]"));
    }
}

void ProtocolDebugPage::onDecodeClicked()
{
    if (!m_loader.isLoaded()) {
        return;
    }
    const QByteArray frame = FrameCodec::fromHexDisplay(ui->decodeHexEdit->text());
    if (frame.isEmpty()) {
        ui->decodedTable->setRowCount(0);
        return;
    }
    // 手动解帧：尝试先按 A0/A3 去封装，取 payload 再解字段；失败则当裸 payload 直接解。
    QByteArray payload = frame;
    if (frame.size() >= 4) {
        const services::ProtocolFrame pf = services::ProtocolParser::parseFrame(frame);
        if (pf.isValid) {
            payload = pf.payload;
        }
    }
    const QMap<QString, QVariant> decoded = FrameCodec::decode(m_loader.protocol(), payload);

    // 收集字段名与类型映射（含 Struct 子字段）。
    struct RowInfo { QString field; QString typeStr; };
    QVector<RowInfo> rows;
    for (const FieldDef &f : m_loader.protocol().fields) {
        if (f.type == FieldType::Struct) {
            for (const FieldDef &c : f.children) {
                const QString key = QStringLiteral("%1.%2").arg(f.name, c.name);
                rows.append({ key, ProtocolFieldTableModel::typeToString(c.type) });
            }
            rows.append({ f.name, QStringLiteral("Struct(hex)") });
        } else {
            rows.append({ f.name, ProtocolFieldTableModel::typeToString(f.type) });
        }
    }

    ui->decodedTable->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        const QString key = rows[i].field;
        ui->decodedTable->setItem(i, 0, new QTableWidgetItem(key));
        ui->decodedTable->setItem(i, 1, new QTableWidgetItem(rows[i].typeStr));
        ui->decodedTable->setItem(i, 2, new QTableWidgetItem(decoded.value(key).toString()));
    }
}

void ProtocolDebugPage::onFrameReceived(const QByteArray &frame)
{
    if (!m_loader.isLoaded()) {
        return;
    }
    // 把收到的完整帧 hex 显示到解帧输入框（便于追溯），并解析。
    ui->decodeHexEdit->setText(FrameCodec::toHexDisplay(frame));
    onDecodeClicked();
}
