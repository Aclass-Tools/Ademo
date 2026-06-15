#include "terminalpage.h"
#include "ui_terminalpage.h"
#include "models/framecodec.h"
#include "services/connectionmanager.h"
#include "ui/theme/thememanager.h"

#include <QCheckBox>
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

    // 默认勾选：Hex 显示关、时间戳开、自动换行开。
    ui->recvTimestampCheck->setChecked(true);
    ui->recvAutoNewlineCheck->setChecked(true);

    // 信号接线。
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

    // 串口配置区已迁移到「设备连接」页：隐藏本页的配置/打开控件。
    // 仅保留收发区，避免与连接页重复管理同一物理连接。
    // （控件本身仍由 ui 文件创建，这里仅隐藏，不删除，保持 ui 文件稳定。）
    ui->openPortButton->setVisible(false);
    ui->refreshPortsButton->setVisible(false);
    ui->connStatusLabel->setText(QStringLiteral("● 待连接（请在设备连接页建立连接）"));

    updateConnStatus(false);
}

TerminalPage::~TerminalPage()
{
    delete ui;
}

void TerminalPage::setConnectionManager(services::ConnectionManager *mgr)
{
    m_conn = mgr;
    if (!m_conn) {
        return;
    }
    // 接收：订阅裸字节流。
    connect(m_conn, &services::ConnectionManager::rawBytesReceived,
            this, &TerminalPage::onDataReceived);
    // 连接状态。
    connect(m_conn, &services::ConnectionManager::connectionChanged,
            this, [this](bool connected, services::TransportMode) {
        updateConnStatus(connected);
    });
    // 错误。
    connect(m_conn, &services::ConnectionManager::errorOccurred, this, [this](const QString &msg) {
        ui->recvTextEdit->append(QStringLiteral("[错误] %1").arg(msg));
    });
    // 同步当前状态。
    updateConnStatus(m_conn->isConnected());
}

void TerminalPage::onPageActivated()
{
    // 连接由全局管理，进入页面时只刷新状态显示。
    if (m_conn) {
        updateConnStatus(m_conn->isConnected());
    }
}

void TerminalPage::onSendButtonClicked()
{
    if (!m_conn || !m_conn->isConnected()) {
        ui->recvTextEdit->append(QStringLiteral("[提示] 未连接，请在设备连接页建立连接。"));
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
    QString err;
    if (m_conn->sendRaw(payload, &err)) {
        m_txBytes += quint64(payload.size());
        ui->txCountLabel->setText(QStringLiteral("TX: %1").arg(m_txBytes));
    } else {
        ui->recvTextEdit->append(QStringLiteral("[发送失败] %1").arg(err));
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

void TerminalPage::updateConnStatus(bool connected)
{
    if (connected) {
        ui->connStatusLabel->setText(QStringLiteral("● 已连接"));
    } else {
        ui->connStatusLabel->setText(QStringLiteral("● 未连接"));
        if (m_timer.isActive()) {
            ui->timerToggleButton->setChecked(false);
        }
    }
}

void TerminalPage::onTimerToggled(bool checked)
{
    if (checked) {
        if (!m_conn || !m_conn->isConnected()) {
            ui->recvTextEdit->append(QStringLiteral("[提示] 未连接，无法定时发送。"));
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
