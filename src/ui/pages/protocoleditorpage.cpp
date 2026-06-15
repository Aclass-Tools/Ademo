#include "protocoleditorpage.h"
#include "ui_protocoleditorpage.h"
#include "models/binprotocolformat.h"
#include "ui/theme/thememanager.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QUrl>

ProtocolEditorPage::ProtocolEditorPage(QWidget *parent)
    : PlaceholderPageBase(parent)
    , ui(new Ui::ProtocolEditorPage)
{
    ui->setupUi(this);
    setStyleSheet(ThemeManager::contentPageStyle(ThemeManager::palette(ThemeKind::IndustrialBlue)));

    // 页面自持一个 ApiClient（保持现有页面"自包含"模式）。
    m_api = new ApiClient(this);

    // 按钮接线。
    connect(ui->openWebButton, &QPushButton::clicked, this, &ProtocolEditorPage::openWebEditor);
    connect(ui->refreshButton, &QPushButton::clicked, this, &ProtocolEditorPage::refreshProtocols);
    connect(ui->downloadButton, &QPushButton::clicked, this, &ProtocolEditorPage::downloadAndLoadCurrent);

    // ApiClient 信号。
    connect(m_api, &ApiClient::protocolBackendReady, this, &ProtocolEditorPage::onBackendReady);
    connect(m_api, &ApiClient::protocolsListed, this, &ProtocolEditorPage::onProtocolsListed);
    connect(m_api, &ApiClient::protocolBinDownloaded, this, &ProtocolEditorPage::onBinDownloaded);
    connect(m_api, &ApiClient::requestFailed, this, &ProtocolEditorPage::onRequestFailed);

    // 选中协议后启用下载按钮。
    connect(ui->protocolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        ui->downloadButton->setEnabled(index >= 0 && m_api->protocolBackendUrl().isValid());
    });

    ui->downloadButton->setEnabled(false);
    ui->backendStatus->setText(QStringLiteral("后端：未检测"));
    ui->summaryLabel->setText(QStringLiteral("提示：先点“打开 Web 编辑器”在浏览器里编辑，保存后回到这里下载 .bin。"));
}

ProtocolEditorPage::~ProtocolEditorPage()
{
    delete ui;
}

void ProtocolEditorPage::onPageActivated()
{
    // 切到本页时探活一次；已探活则只刷新列表。
    if (!m_backendChecked) {
        ui->backendStatus->setText(QStringLiteral("后端：检测中…"));
        m_api->checkProtocolBackend();
    } else {
        refreshProtocols();
    }
}

void ProtocolEditorPage::openWebEditor()
{
    // 用系统默认浏览器打开 Web 编辑器（无需 WebEngine）。
    const QUrl url = m_api->protocolBackendUrl();
    if (!url.isValid() || url.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("打开失败"), QStringLiteral("后端地址无效。"));
        return;
    }
    if (!QDesktopServices::openUrl(url)) {
        QMessageBox::warning(this, QStringLiteral("打开失败"),
            QStringLiteral("无法调用系统浏览器，请检查默认浏览器设置。"));
    }
}

void ProtocolEditorPage::refreshProtocols()
{
    if (!m_backendChecked) {
        // 还没探活，先探活（onBackendReady 里会自动拉列表）。
        m_api->checkProtocolBackend();
        return;
    }
    ui->refreshButton->setEnabled(false);
    m_api->listProtocols();
}

void ProtocolEditorPage::downloadAndLoadCurrent()
{
    const int idx = ui->protocolCombo->currentIndex();
    const int pid = ui->protocolCombo->currentData().toInt();
    if (idx < 0 || pid <= 0) {
        return;
    }
    // 落盘目录：applicationDirPath()/../data/protocols，与协议调试页默认目录一致。
    const QString dir = QCoreApplication::applicationDirPath()
        + QStringLiteral("/../data/protocols");
    QDir().mkpath(dir);
    QString name = m_protocolNames.value(pid, QStringLiteral("protocol_%1").arg(pid));
    // 去掉名字里不安全的字符，避免文件名问题。
    name.remove(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")));
    const QString savePath = dir + QStringLiteral("/") + name + QStringLiteral(".bin");

    ui->summaryLabel->setText(QStringLiteral("正在下载 %1 …").arg(savePath));
    ui->downloadButton->setEnabled(false);
    m_api->getProtocolBin(pid, savePath);
}

void ProtocolEditorPage::onBackendReady(bool ready)
{
    m_backendChecked = true;
    if (ready) {
        ui->backendStatus->setText(QStringLiteral("后端：已连接（%1）").arg(m_api->protocolBackendUrl().toString()));
        // 连上后自动拉一次列表。
        m_api->listProtocols();
    } else {
        ui->backendStatus->setText(QStringLiteral("后端：未连接（请先启动 backend\\run_backend.bat）"));
        ui->protocolCombo->clear();
        ui->downloadButton->setEnabled(false);
    }
}

void ProtocolEditorPage::onProtocolsListed(const QByteArray &payload)
{
    ui->refreshButton->setEnabled(true);
    // 解析 [{id,name,...}]，填充下拉框（显示 name，存 id）。
    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    const QJsonArray arr = doc.array();

    ui->protocolCombo->clear();
    m_protocolNames.clear();
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        const int id = o.value(QStringLiteral("id")).toInt();
        const QString name = o.value(QStringLiteral("name")).toString();
        if (id <= 0) {
            continue;
        }
        m_protocolNames.insert(id, name);
        ui->protocolCombo->addItem(name, id);
    }
    if (ui->protocolCombo->count() == 0) {
        ui->summaryLabel->setText(QStringLiteral("后端暂无协议。请到 Web 编辑器新建。"));
    } else {
        ui->summaryLabel->setText(QStringLiteral("共 %1 个协议，选中后点“下载并加载 .bin”。").arg(ui->protocolCombo->count()));
    }
    ui->downloadButton->setEnabled(ui->protocolCombo->count() > 0);
}

void ProtocolEditorPage::onBinDownloaded(const QString &localPath)
{
    ui->downloadButton->setEnabled(ui->protocolCombo->count() > 0);
    // 用 BinProtocolLoader 加载下载下来的 bin，作为下载成功的校验。
    if (!m_loader.load(localPath)) {
        ui->summaryLabel->setText(QStringLiteral("下载完成但加载失败：%1").arg(m_loader.lastError()));
        return;
    }
    showLoadedSummary(m_loader.protocol());
}

void ProtocolEditorPage::onRequestFailed(const QString &message)
{
    ui->refreshButton->setEnabled(true);
    ui->downloadButton->setEnabled(ui->protocolCombo->count() > 0);
    ui->summaryLabel->setText(QStringLiteral("请求失败：%1").arg(message));
}

void ProtocolEditorPage::showLoadedSummary(const BinProtocol &proto)
{
    // 字段数（含子字段），给用户一个直观确认。
    int topLevel = 0;
    int total = 0;
    for (const FieldDef &f : proto.fields) {
        ++topLevel;
        ++total;
        total += f.children.size();
    }
    ui->summaryLabel->setText(
        QStringLiteral("已加载：%1\n帧大小：%2 字节 | 顶层字段 %3 个（含子字段共 %4 个） | version %5")
            .arg(proto.sourcePath)
            .arg(proto.frameSize())
            .arg(topLevel)
            .arg(total)
            .arg(proto.version));
}
