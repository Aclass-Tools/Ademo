// 协议编辑页面（ProtocolEditorPage）
// ------------------------------------
// 协议编辑已迁移到 Web：本页面是 Qt 与 Web 后端之间的"桥接页"。
//
// 职责：
// - 打开系统浏览器进入 Web 编辑器（QDesktopServices::openUrl，无需 WebEngine）。
// - 从后端拉取协议列表，供用户选择。
// - 下载选中协议的 .bin 到本地 data/protocols/，并用 BinProtocolLoader 加载，
//   加载结果（帧大小/字段数）作为下载成功的确认展示。
//
// 说明：
// - 本页面不再做字段级的可视化编辑（已交给网页）。
// - BinProtocolLoader 保留，仅用于"下载后加载校验"，不再用于本地编辑回写。
// - 后端地址默认 http://127.0.0.1:5000，与 ApiClient::protocolBackendUrl() 一致。

#pragma once

#include "placeholderpagebase.h"
#include "models/binprotocolloader.h"
#include "network/apiclient.h"

#include <QHash>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class ProtocolEditorPage;
}
QT_END_NAMESPACE

class ProtocolEditorPage : public PlaceholderPageBase
{
    Q_OBJECT

public:
    explicit ProtocolEditorPage(QWidget *parent = nullptr);
    ~ProtocolEditorPage();

protected:
    // 切到本页时自动探活后端并刷新协议列表。
    void onPageActivated() override;

private:
    void openWebEditor();
    void refreshProtocols();
    void downloadAndLoadCurrent();

    // ApiClient 回调。
    void onBackendReady(bool ready);
    void onProtocolsListed(const QByteArray &payload);
    void onBinDownloaded(const QString &localPath);
    void onRequestFailed(const QString &message);

    // 把已加载协议的关键信息回显到状态区。
    void showLoadedSummary(const BinProtocol &proto);

private:
    Ui::ProtocolEditorPage *ui;
    ApiClient *m_api = nullptr;
    BinProtocolLoader m_loader;
    // protocolId -> name，便于下载时取文件名。
    QHash<int, QString> m_protocolNames;
    // 避免每次切到本页都探活：首活后置位。
    bool m_backendChecked = false;
};