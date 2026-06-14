#include "protocoldebugpage.h"
#include "ui_protocoldebugpage.h"
#include "models/binprotocolformat.h"
#include "models/framecodec.h"
#include "models/protocolfieldtablemodel.h"
#include "ui/theme/thememanager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
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
    // 容器垂直布局，避免空时高度塌陷。
    auto *hostLayout = new QVBoxLayout(ui->encodeInputsHost);
    hostLayout->setContentsMargins(0, 0, 0, 0);
    hostLayout->addLayout(form);
    hostLayout->addStretch(1);
    // 让外部能通过 objectName 取到 form。
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
    connect(ui->refreshPortsButton, &QPushButton::clicked, this, &ProtocolDebugPage::refreshPorts);
    connect(ui->sendViaSerialCheck, &QCheckBox::toggled, this, [this](bool checked) {
        ui->portCombo->setEnabled(checked);
        ui->refreshPortsButton->setEnabled(checked);
        if (checked) {
            refreshPorts();
        }
    });
    connect(&m_serial, &SerialPortManager::dataReceived, this, [this](const QByteArray &data) {
        // 自动把串口收到的数据填入解帧输入框并尝试解帧。
        ui->decodeHexEdit->setText(FrameCodec::toHexDisplay(data));
        onDecodeClicked();
    });

    ui->portCombo->setEnabled(false);
    ui->refreshPortsButton->setEnabled(false);
}

ProtocolDebugPage::~ProtocolDebugPage()
{
    delete ui;
}

void ProtocolDebugPage::onPageActivated()
{
    // 首次进入若未加载，提示用户加载示例。
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
    // 清空旧控件。
    while (form->rowCount() > 0) {
        form->removeRow(0);
    }
    m_inputs.clear();

    const QVector<FieldDef> &fields = m_loader.protocol().fields;
    for (const FieldDef &f : fields) {
        if (f.type == FieldType::Struct) {
            // 为每个子字段单独生成输入。
            for (const FieldDef &c : f.children) {
                const QString key = QStringLiteral("%1.%2").arg(f.name, c.name);
                auto *edit = new QLineEdit(ui->encodeInputsHost);
                edit->setText(QStringLiteral("0"));
                form->addRow(QStringLiteral("%1：%2").arg(f.name, c.name), edit);
                m_inputs.insert(key, edit);
            }
        } else {
            auto *edit = new QLineEdit(ui->encodeInputsHost);
            // 给出合理默认值，便于立即组包。
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
    QMap<QString, QVariant> values;
    for (auto it = m_inputs.constBegin(); it != m_inputs.constEnd(); ++it) {
        values.insert(it.key(), it.value()->text());
    }
    const QByteArray frame = FrameCodec::encode(m_loader.protocol(), values);
    const QString hex = FrameCodec::toHexDisplay(frame);
    ui->encodedHexEdit->setText(hex);

    // 经串口发送（可选）。
    if (ui->sendViaSerialCheck->isChecked()) {
        const QVariant portData = ui->portCombo->currentData();
        const QString portName = portData.isValid() ? portData.toString() : ui->portCombo->currentText();
        if (portName.isEmpty()) {
            ui->encodedHexEdit->setText(hex + QStringLiteral("   [未选择端口]"));
            return;
        }
        if (!m_serial.isOpen()) {
            m_serial.setPortName(portName);
            m_serial.setBaudRate(115200);
            m_serial.openPort();
        }
        const qint64 n = m_serial.sendData(frame);
        ui->encodedHexEdit->setText(hex + (n > 0
            ? QStringLiteral("   [已发送 %1 字节]").arg(n)
            : QStringLiteral("   [发送失败]")));
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
    const QMap<QString, QVariant> decoded = FrameCodec::decode(m_loader.protocol(), frame);

    // 收集字段名与类型映射（含 Struct 子字段）。
    struct RowInfo { QString field; QString typeStr; };
    QVector<RowInfo> rows;
    QHash<QString, QString> typeOf;
    for (const FieldDef &f : m_loader.protocol().fields) {
        if (f.type == FieldType::Struct) {
            for (const FieldDef &c : f.children) {
                const QString key = QStringLiteral("%1.%2").arg(f.name, c.name);
                rows.append({ key, ProtocolFieldTableModel::typeToString(c.type) });
                typeOf.insert(key, key);
            }
            // 父结构原始 hex 行。
            rows.append({ f.name, QStringLiteral("Struct(hex)") });
            typeOf.insert(f.name, f.name);
        } else {
            rows.append({ f.name, ProtocolFieldTableModel::typeToString(f.type) });
            typeOf.insert(f.name, f.name);
        }
    }

    ui->decodedTable->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        const QString key = rows[i].field;
        ui->decodedTable->setItem(i, 0, new QTableWidgetItem(key));
        ui->decodedTable->setItem(i, 1, new QTableWidgetItem(rows[i].typeStr));
        const QVariant v = decoded.value(key);
        ui->decodedTable->setItem(i, 2, new QTableWidgetItem(v.toString()));
    }
}

void ProtocolDebugPage::refreshPorts()
{
    const QString prev = ui->portCombo->currentText();
    ui->portCombo->blockSignals(true);
    ui->portCombo->clear();
    const QVariantList ports = SerialPortManager::availablePorts();
    if (ports.isEmpty()) {
        ui->portCombo->addItem(QStringLiteral("（无可用串口）"));
    } else {
        for (const QVariant &v : ports) {
            const QVariantMap m = v.toMap();
            const QString name = m.value(QStringLiteral("portName")).toString();
            ui->portCombo->addItem(name, name);
        }
    }
    const int idx = ui->portCombo->findData(prev);
    if (idx >= 0) {
        ui->portCombo->setCurrentIndex(idx);
    }
    ui->portCombo->blockSignals(false);
}
