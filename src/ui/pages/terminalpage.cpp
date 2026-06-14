#include "terminalpage.h"
#include "ui_terminalpage.h"
#include "models/framecodec.h"
#include "ui/theme/thememanager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTextCursor>
#include <QTextEdit>

TerminalPage::TerminalPage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::TerminalPage)
{
    ui->setupUi(this);
    // 有实际内容的页面使用 contentPageStyle，而非占位页样式。
    setStyleSheet(ThemeManager::contentPageStyle(ThemeManager::palette(ThemeKind::IndustrialBlue)));

    // 初始化各下拉框候选项。
    ui->dataBitsCombo->addItems({ QStringLiteral("8"), QStringLiteral("7"), QStringLiteral("6"), QStringLiteral("5") });
    ui->stopBitsCombo->addItems({ QStringLiteral("1"), QStringLiteral("1.5"), QStringLiteral("2") });
    ui->parityCombo->addItems({ QStringLiteral("无"), QStringLiteral("偶"), QStringLiteral("奇"), QStringLiteral("空号"), QStringLiteral("标记") });
    ui->flowCombo->addItems({ QStringLiteral("无"), QStringLiteral("RTS/CTS"), QStringLiteral("XON/XOFF"), QStringLiteral("全部") });
    ui->dataBitsCombo->setCurrentIndex(0);
    ui->stopBitsCombo->setCurrentIndex(0);
    ui->parityCombo->setCurrentIndex(0);
    ui->flowCombo->setCurrentIndex(0);
    populateBaudRates();

    refreshPorts();

    // 默认勾选：Hex 显示关、时间戳开、自动换行开。
    ui->recvTimestampCheck->setChecked(true);
    ui->recvAutoNewlineCheck->setChecked(true);

    // 信号接线。
    connect(ui->refreshPortsButton, &QPushButton::clicked, this, &TerminalPage::refreshPorts);
    connect(ui->openPortButton, &QPushButton::clicked, this, &TerminalPage::openOrClosePort);
    connect(ui->sendButton, &QPushButton::clicked, this, &TerminalPage::onSendButtonClicked);
    connect(ui->recvClearButton, &QPushButton::clicked, ui->recvTextEdit, &QTextEdit::clear);
    connect(ui->resetCountButton, &QPushButton::clicked, this, [this]() {
        m_rxBytes = 0;
        m_txBytes = 0;
        ui->rxCountLabel->setText(QStringLiteral("RX: 0"));
        ui->txCountLabel->setText(QStringLiteral("TX: 0"));
    });
    connect(ui->timerToggleButton, &QPushButton::toggled, this, &TerminalPage::onTimerToggled);
    connect(&m_timer, &QTimer::timeout, this, &TerminalPage::onSendButtonClicked);

    // SerialPortManager 事件面。
    connect(&m_serial, &SerialPortManager::dataReceived, this, &TerminalPage::onDataReceived);
    connect(&m_serial, &SerialPortManager::portOpened, this, [this]() { updateConnStatus(true); });
    connect(&m_serial, &SerialPortManager::portClosed, this, [this]() { updateConnStatus(false); });
    connect(&m_serial, &SerialPortManager::errorOccurred, this, [this](const QString &msg) {
        ui->recvTextEdit->append(QStringLiteral("[错误] %1").arg(msg));
    });

    updateConnStatus(false);
}

TerminalPage::~TerminalPage()
{
    delete ui;
}

void TerminalPage::onPageActivated()
{
    // 每次切回本页刷新端口，便于热插拔识别。
    refreshPorts();
}

void TerminalPage::refreshPorts()
{
    const QString prev = ui->portNameCombo->currentText();
    ui->portNameCombo->blockSignals(true);
    ui->portNameCombo->clear();
    const QVariantList ports = SerialPortManager::availablePorts();
    if (ports.isEmpty()) {
        ui->portNameCombo->addItem(QStringLiteral("（无可用串口）"));
    } else {
        for (const QVariant &v : ports) {
            const QVariantMap m = v.toMap();
            const QString name = m.value(QStringLiteral("portName")).toString();
            const QString desc = m.value(QStringLiteral("description")).toString();
            ui->portNameCombo->addItem(desc.isEmpty() ? name : QStringLiteral("%1 - %2").arg(name, desc), name);
        }
    }
    // 恢复之前选择。
    const int idx = ui->portNameCombo->findData(prev);
    if (idx >= 0) {
        ui->portNameCombo->setCurrentIndex(idx);
    }
    ui->portNameCombo->blockSignals(false);
}

void TerminalPage::populateBaudRates()
{
    const QList<qint32> stdBauds = { 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600 };
    for (qint32 b : stdBauds) {
        ui->baudCombo->addItem(QString::number(b), b);
    }
    ui->baudCombo->setCurrentIndex(stdBauds.indexOf(115200));
}

void TerminalPage::applyConfigToSerial()
{
    // 取端口名称：优先 userData，避免带描述文字。
    const QVariant data = ui->portNameCombo->currentData();
    m_serial.setPortName(data.isValid() ? data.toString() : ui->portNameCombo->currentText());
    m_serial.setBaudRate(ui->baudCombo->currentData().toInt());
    m_serial.setDataBits(ui->dataBitsCombo->currentText().toInt());
    m_serial.setParity(ui->parityCombo->currentIndex());
    m_serial.setStopBits(ui->stopBitsCombo->currentIndex());
    m_serial.setFlowControl(ui->flowCombo->currentIndex());
}

void TerminalPage::openOrClosePort()
{
    if (m_serial.isOpen()) {
        m_serial.closePort();
        return;
    }
    applyConfigToSerial();
    m_serial.openPort();
}

void TerminalPage::onSendButtonClicked()
{
    if (!m_serial.isOpen()) {
        return;
    }
    QByteArray payload;
    if (ui->sendHexCheck->isChecked()) {
        payload = FrameCodec::fromHexDisplay(ui->sendLineEdit->text());
        if (payload.isEmpty() && !ui->sendLineEdit->text().trimmed().isEmpty()) {
            ui->recvTextEdit->append(QStringLiteral("[提示] Hex 格式无效，未发送。"));
            return;
        }
    } else {
        payload = ui->sendLineEdit->text().toUtf8();
    }
    if (ui->sendNewlineCheck->isChecked()) {
        payload.append("\r\n");
    }
    if (payload.isEmpty()) {
        return;
    }
    const qint64 n = m_serial.sendData(payload);
    if (n > 0) {
        m_txBytes += quint64(n);
        ui->txCountLabel->setText(QStringLiteral("TX: %1").arg(m_txBytes));
    }
}

void TerminalPage::onDataReceived(const QByteArray &data)
{
    m_rxBytes += quint64(data.size());
    ui->rxCountLabel->setText(QStringLiteral("RX: %1").arg(m_rxBytes));
    appendRecv(data);
}

void TerminalPage::appendRecv(const QByteArray &data)
{
    // 把本次到达的字节按当前显示模式渲染追加。
    QString chunk;
    if (ui->recvHexCheck->isChecked()) {
        chunk = FrameCodec::toHexDisplay(data);
    } else {
        chunk = QString::fromUtf8(data);
    }

    // 时间戳：本块起始加一个时间前缀。
    if (ui->recvTimestampCheck->isChecked()) {
        const QString ts = QDateTime::currentDateTime().toString(QStringLiteral("[HH:mm:ss.zzz] "));
        chunk.prepend(ts);
    }
    // 自动换行：每块独立成行。
    if (ui->recvAutoNewlineCheck->isChecked()) {
        chunk.append('\n');
    }

    QTextCursor c = ui->recvTextEdit->textCursor();
    c.movePosition(QTextCursor::End);
    c.insertText(chunk);
    ui->recvTextEdit->setTextCursor(c);
}

void TerminalPage::updateConnStatus(bool open)
{
    if (open) {
        ui->connStatusLabel->setText(QStringLiteral("● 已连接"));
        ui->openPortButton->setText(QStringLiteral("关闭串口"));
    } else {
        ui->connStatusLabel->setText(QStringLiteral("● 未连接"));
        ui->openPortButton->setText(QStringLiteral("打开串口"));
        if (m_timer.isActive()) {
            ui->timerToggleButton->setChecked(false);
        }
    }
}

void TerminalPage::onTimerToggled(bool checked)
{
    if (checked) {
        if (!m_serial.isOpen()) {
            ui->recvTextEdit->append(QStringLiteral("[提示] 请先打开串口。"));
            ui->timerToggleButton->setChecked(false);
            return;
        }
        m_timer.start(ui->timerIntervalSpin->value());
        ui->timerToggleButton->setText(QStringLiteral("停止定时"));
    } else {
        m_timer.stop();
        ui->timerToggleButton->setText(QStringLiteral("启动定时"));
    }
}
