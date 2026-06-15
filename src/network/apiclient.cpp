#include "apiclient.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

ApiClient::ApiClient(QObject *parent)
    : QObject(parent)
    // 默认本地后端地址，可通过 setBaseUrl() 覆盖。
    , m_baseUrl(QStringLiteral("http://127.0.0.1:8000"))
    // 协议编辑后端（Flask）默认地址，独立端口。
    , m_protocolBackendUrl(QStringLiteral("http://127.0.0.1:5000"))
    // 生命周期绑定到 ApiClient。
    , m_manager(new QNetworkAccessManager(this))
{
}

void ApiClient::setBaseUrl(const QUrl &baseUrl)
{
    m_baseUrl = baseUrl;
}

QUrl ApiClient::baseUrl() const
{
    return m_baseUrl;
}

void ApiClient::getVersion()
{
    // 路径约定来自后端接口设计。
    const QUrl url = m_baseUrl.resolved(QUrl(QStringLiteral("/api/version")));
    QNetworkReply *reply = m_manager->get(QNetworkRequest(url));
    handleReply(reply, [this](QNetworkReply *okReply) {
        // 防御式解析：字段缺失时返回空字符串。
        const QJsonDocument doc = QJsonDocument::fromJson(okReply->readAll());
        const QString version = doc.object().value(QStringLiteral("version")).toString();
        emit versionReceived(version);
    });
}

void ApiClient::getPoints()
{
    const QUrl url = m_baseUrl.resolved(QUrl(QStringLiteral("/api/v1/points")));
    QNetworkReply *reply = m_manager->get(QNetworkRequest(url));
    handleReply(reply, [this](QNetworkReply *okReply) {
        // schema 解析由上层决定。
        emit pointsReceived(okReply->readAll());
    });
}

void ApiClient::updatePoint(int id, int version, const QString &name, const QString &protocol, int registerAddress)
{
    // REST 资源更新路径。
    const QUrl url = m_baseUrl.resolved(QUrl(QStringLiteral("/api/v1/points/%1").arg(id)));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    // 最小更新体，包含乐观锁 version。
    QJsonObject body;
    body.insert(QStringLiteral("version"), version);
    body.insert(QStringLiteral("name"), name);
    body.insert(QStringLiteral("protocol"), protocol);
    body.insert(QStringLiteral("register_address"), registerAddress);

    QNetworkReply *reply = m_manager->put(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, id]() {
        const int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() == QNetworkReply::NoError) {
            emit pointUpdated(id);
            reply->deleteLater();
            return;
        }
        // 冲突分支单独输出，便于 UI 做对账/回滚。
        if (code == 409) {
            emit conflictDetected(id);
            reply->deleteLater();
            return;
        }
        // 通用传输/协议失败。
        emit requestFailed(reply->errorString());
        reply->deleteLater();
    });
}

void ApiClient::setProtocolBackendUrl(const QUrl &url)
{
    m_protocolBackendUrl = url;
}

QUrl ApiClient::protocolBackendUrl() const
{
    return m_protocolBackendUrl;
}

void ApiClient::checkProtocolBackend()
{
    // GET /api/health，2xx 视为可用。
    const QUrl url = m_protocolBackendUrl.resolved(QUrl(QStringLiteral("/api/health")));
    QNetworkReply *reply = m_manager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const bool ok = (reply->error() == QNetworkReply::NoError);
        if (!ok) {
            emit requestFailed(reply->errorString());
        }
        emit protocolBackendReady(ok);
        reply->deleteLater();
    });
}

void ApiClient::listProtocols()
{
    const QUrl url = m_protocolBackendUrl.resolved(QUrl(QStringLiteral("/api/protocols")));
    QNetworkReply *reply = m_manager->get(QNetworkRequest(url));
    handleReply(reply, [this](QNetworkReply *okReply) {
        emit protocolsListed(okReply->readAll());
    });
}

void ApiClient::getProtocolBin(int protocolId, const QString &savePath)
{
    // 注意：bin 是二进制，不能走 JSON 解析路径，需直接落盘。
    const QUrl url = m_protocolBackendUrl.resolved(
        QUrl(QStringLiteral("/api/protocols/%1/bin").arg(protocolId)));
    QNetworkReply *reply = m_manager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, savePath]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(reply->errorString());
            reply->deleteLater();
            return;
        }
        // 把整个响应体写入目标文件。
        QFile file(savePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            emit requestFailed(QStringLiteral("无法写入文件：%1").arg(savePath));
            reply->deleteLater();
            return;
        }
        file.write(reply->readAll());
        file.close();
        emit protocolBinDownloaded(savePath);
        reply->deleteLater();
    });
}

void ApiClient::handleReply(QNetworkReply *reply, const std::function<void (QNetworkReply *)> &onSuccess)
{
    connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess]() {
        if (reply->error() == QNetworkReply::NoError) {
            onSuccess(reply);
        } else {
            emit requestFailed(reply->errorString());
        }
        // 必须释放 reply，防止内存泄漏。
        reply->deleteLater();
    });
}
