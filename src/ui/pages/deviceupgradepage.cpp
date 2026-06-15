#include "deviceupgradepage.h"
#include "ui_deviceupgrade.h"
#include "services/connectionmanager.h"
#include "services/protocolparser.h"
#include "ui/theme/thememanager.h"

#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
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

    connect(ui->browseButton, &QPushButton::clicked, this, &DeviceUpgradePage::browseFirmware);
    connect(ui->startButton, &QPushButton::clicked, this, &DeviceUpgradePage::startUpgrade);
    connect(ui->cancelButton, &QPushButton::clicked, this, &DeviceUpgradePage::cancelUpgrade);
    connect(&m_timer, &QTimer::timeout, this, &DeviceUpgradePage::sendNextChunk);

    // 串口配置区已迁移到「设备连接」页：隐藏本页的端口/波特率/连接控件。
    ui->refreshPortsButton->setVisible(false);
    ui->connectButton->setVisible(false);
    ui->portCombo->setVisible(false);
    ui->baudCombo->setVisible(false);
    ui->connStatusLabel->setText(QStringLiteral("● 待连接（请在设备连接页建立连接）"));

    updateConnStatus(false);
}

DeviceUpgradePage::~DeviceUpgradePage()
{
    m_timer.stop();
    delete ui;
}

void DeviceUpgradePage::setConnectionManager(services::ConnectionManager *mgr)
{
    m_conn = mgr;
    if (m_conn) {
        connect(m_conn, &services::ConnectionManager::connectionChanged,
                this, [this](bool connected, services::TransportMode) {
            updateConnStatus(connected);
        });
        connect(m_conn, &services::ConnectionManager::errorOccurred, this, [this](const QString &msg) {
            log(QStringLiteral("[错误] %1").arg(msg));
        });
        updateConnStatus(m_conn->isConnected());
    }
}

void DeviceUpgradePage::onPageActivated()
{
    if (m_conn) {
        updateConnStatus(m_conn->isConnected());
    }
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
    m_blockIndex = 0;
    m_running = true;
    setControlsRunning(true);

    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(m_firmware.size());
    log(QStringLiteral("开始升级：共 %1 字节，分块 %2 字节，块间延时 %3 ms")
        .arg(m_firmware.size()).arg(m_chunkSize).arg(ui->intervalSpin->value()));

    if (!m_conn || !m_conn->isConnected()) {
        log(QStringLiteral("[提示] 未连接，将以“模拟”方式跑通流程（发送失败）。"));
    }
    const int interval = ui->intervalSpin->value();
    m_timer.start(qMax(interval, 1));
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
    ++m_blockIndex;

    // 每块固件用 A0 写帧封装：mainCommand/subCommand 携带块号（mod 65536）。
    const quint16 blockIdx16 = quint16(m_blockIndex & 0xFFFF);
    QString err;
    bool ok = false;
    if (m_conn && m_conn->isConnected()) {
        const QByteArray frame = services::ProtocolParser::buildWriteFrame(
            services::kProtocolA0, m_sourceId, m_targetId,
            quint8((blockIdx16 >> 8) & 0xFF), quint8(blockIdx16 & 0xFF), chunk);
        ok = m_conn->sendFrame(frame, &err);
    }
    if (ok) {
        log(QStringLiteral("[块 %1] 发送 %2 字节 OK").arg(m_blockIndex).arg(thisChunk));
        m_totalSent += thisChunk;
    } else {
        log(QStringLiteral("[块 %1] 发送失败/模拟：%2").arg(m_blockIndex).arg(err));
    }

    m_sentOffset += thisChunk;
    ui->progressBar->setValue(m_sentOffset);
    const int pct = (m_firmware.size() > 0) ? int(quint64(m_sentOffset) * 100 / quint64(m_firmware.size())) : 0;
    ui->progressStatusLabel->setText(QStringLiteral("进度 %1%  (%2/%3)").arg(pct).arg(m_sentOffset).arg(m_firmware.size()));
}

void DeviceUpgradePage::updateConnStatus(bool connected)
{
    ui->connStatusLabel->setText(connected ? QStringLiteral("● 已连接") : QStringLiteral("● 未连接"));
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
    ui->chunkSizeSpin->setEnabled(!running);
}
