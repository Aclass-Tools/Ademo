#include "deviceupgradepage.h"
#include "ui_deviceupgrade.h"
#include "ui/theme/thememanager.h"

#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTextCursor>
#include <QTextEdit>

DeviceUpgradePage::DeviceUpgradePage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::DeviceUpgradePage)
{
    ui->setupUi(this);
    setStyleSheet(ThemeManager::contentPageStyle(ThemeManager::palette(ThemeKind::IndustrialBlue)));

    populateBaudRates();

    connect(ui->refreshPortsButton, &QPushButton::clicked, this, &DeviceUpgradePage::refreshPorts);
    connect(ui->connectButton, &QPushButton::clicked, this, &DeviceUpgradePage::connectOrDisconnect);
    connect(ui->browseButton, &QPushButton::clicked, this, &DeviceUpgradePage::browseFirmware);
    connect(ui->startButton, &QPushButton::clicked, this, &DeviceUpgradePage::startUpgrade);
    connect(ui->cancelButton, &QPushButton::clicked, this, &DeviceUpgradePage::cancelUpgrade);
    connect(&m_timer, &QTimer::timeout, this, &DeviceUpgradePage::sendNextChunk);

    connect(&m_serial, &SerialPortManager::portOpened, this, [this]() { updateConnStatus(true); });
    connect(&m_serial, &SerialPortManager::portClosed, this, [this]() { updateConnStatus(false); });
    connect(&m_serial, &SerialPortManager::errorOccurred, this, [this](const QString &msg) {
        log(QStringLiteral("[错误] %1").arg(msg));
    });

    refreshPorts();
    updateConnStatus(false);
}

DeviceUpgradePage::~DeviceUpgradePage()
{
    m_timer.stop();
    delete ui;
}

void DeviceUpgradePage::onPageActivated()
{
    refreshPorts();
}

void DeviceUpgradePage::refreshPorts()
{
    const QString prev = ui->portCombo->currentData().toString();
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

void DeviceUpgradePage::populateBaudRates()
{
    const QList<int> stdBauds = { 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600 };
    for (int b : stdBauds) {
        ui->baudCombo->addItem(QString::number(b), b);
    }
    ui->baudCombo->setCurrentIndex(stdBauds.indexOf(115200));
}

void DeviceUpgradePage::connectOrDisconnect()
{
    if (m_serial.isOpen()) {
        m_serial.closePort();
        return;
    }
    const QVariant data = ui->portCombo->currentData();
    m_serial.setPortName(data.isValid() ? data.toString() : QString());
    m_serial.setBaudRate(ui->baudCombo->currentData().toInt());
    m_serial.openPort();
}

void DeviceUpgradePage::browseFirmware()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择固件文件"),
        QString(),
        QStringLiteral("固件 (*.bin *.hex *.img);;所有文件 (*.*)"));
    if (path.isEmpty()) {
        return;
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        ui->fileInfoLabel->setText(QStringLiteral("读取失败。"));
        return;
    }
    m_firmware = f.readAll();
    f.close();
    const QFileInfo fi(path);
    ui->firmwarePathEdit->setText(path);
    // 用 hex 串展示前 8 字节作为简易指纹。
    const QByteArray head = m_firmware.left(8);
    QString headHex;
    for (int i = 0; i < head.size(); ++i) {
        headHex += QString::asprintf("%02X ", quint8(head.at(i)));
    }
    ui->fileInfoLabel->setText(QStringLiteral("大小：%1 字节　前 8 字节：%2")
        .arg(m_firmware.size())
        .arg(headHex.trimmed()));
    log(QStringLiteral("已加载固件：%1（%2 字节）").arg(fi.fileName()).arg(m_firmware.size()));
}

void DeviceUpgradePage::startUpgrade()
{
    if (m_running) {
        return;
    }
    if (m_firmware.isEmpty()) {
        log(QStringLiteral("[提示] 请先选择固件文件。"));
        return;
    }
    m_chunkSize = ui->chunkSizeSpin->value();
    m_sentOffset = 0;
    m_totalSent = 0;
    m_running = true;
    setControlsRunning(true);

    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(m_firmware.size());
    log(QStringLiteral("开始升级：共 %1 字节，分块 %2 字节，块间延时 %3 ms")
        .arg(m_firmware.size()).arg(m_chunkSize).arg(ui->intervalSpin->value()));

    if (!m_serial.isOpen()) {
        log(QStringLiteral("[提示] 串口未连接，将以“模拟”方式跑通流程（写返回 -1）。"));
    }
    // 启动逐块发送；间隔由 intervalSpin 决定，首块立即发送。
    const int interval = ui->intervalSpin->value();
    m_timer.start(qMax(interval, 1));
    // 立即触发第一块，避免等待一个间隔。
    sendNextChunk();
}

void DeviceUpgradePage::cancelUpgrade()
{
    if (!m_running) {
        return;
    }
    m_timer.stop();
    m_running = false;
    setControlsRunning(false);
    log(QStringLiteral("已取消升级：已发送 %1/%2 字节。").arg(m_totalSent).arg(m_firmware.size()));
    ui->progressStatusLabel->setText(QStringLiteral("已取消"));
}

void DeviceUpgradePage::sendNextChunk()
{
    if (!m_running) {
        return;
    }
    if (m_sentOffset >= m_firmware.size()) {
        m_timer.stop();
        m_running = false;
        setControlsRunning(false);
        log(QStringLiteral("升级完成：共发送 %1 字节。").arg(m_totalSent));
        ui->progressStatusLabel->setText(QStringLiteral("完成"));
        return;
    }

    const int remain = m_firmware.size() - m_sentOffset;
    const int thisChunk = qMin(m_chunkSize, remain);
    const QByteArray chunk = m_firmware.mid(m_sentOffset, thisChunk);

    const qint64 n = m_serial.sendData(chunk);
    const int blockIndex = m_sentOffset / m_chunkSize + 1;
    if (n > 0) {
        log(QStringLiteral("[块 %1] 发送 %2 字节 OK").arg(blockIndex).arg(n));
        m_totalSent += int(n);
    } else {
        log(QStringLiteral("[块 %1] 发送失败/模拟").arg(blockIndex));
    }

    m_sentOffset += thisChunk;
    ui->progressBar->setValue(m_sentOffset);
    const int pct = (m_firmware.size() > 0) ? int(quint64(m_sentOffset) * 100 / quint64(m_firmware.size())) : 0;
    ui->progressStatusLabel->setText(QStringLiteral("进度 %1%  (%2/%3)").arg(pct).arg(m_sentOffset).arg(m_firmware.size()));
}

void DeviceUpgradePage::updateConnStatus(bool open)
{
    if (open) {
        ui->connStatusLabel->setText(QStringLiteral("● 已连接"));
        ui->connectButton->setText(QStringLiteral("断开"));
    } else {
        ui->connStatusLabel->setText(QStringLiteral("● 未连接"));
        ui->connectButton->setText(QStringLiteral("连接"));
    }
}

void DeviceUpgradePage::log(const QString &msg)
{
    const QString line = QStringLiteral("[%1] %2")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz")))
        .arg(msg);
    QTextCursor c = ui->logTextEdit->textCursor();
    c.movePosition(QTextCursor::End);
    c.insertText(line + '\n');
    ui->logTextEdit->setTextCursor(c);
}

void DeviceUpgradePage::setControlsRunning(bool running)
{
    ui->startButton->setEnabled(!running);
    ui->cancelButton->setEnabled(running);
    ui->browseButton->setEnabled(!running);
    ui->portCombo->setEnabled(!running);
    ui->baudCombo->setEnabled(!running);
    ui->connectButton->setEnabled(!running);
    ui->chunkSizeSpin->setEnabled(!running);
}
