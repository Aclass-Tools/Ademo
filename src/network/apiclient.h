// 接口客户端（ApiClient）
// -------------------------
// 面向 UI 层的轻量 HTTP 网关。
//
// 设计目标：
// - 将请求/响应的传输细节与界面控件解耦。
// - 通过语义化信号对上层输出异步结果，便于测试与替换。
// - 统一基于 baseUrl() 构建接口路径。
//
// 说明：
// - 当前负载协议保持最小化，后续可平滑扩展。
// - 409 冲突通过专用信号抛出，配合乐观锁流程。

#pragma once

#include <QObject>
#include <QUrl>
#include <functional>

class QNetworkAccessManager;
class QNetworkReply;

class ApiClient : public QObject
{
    Q_OBJECT

public:
    explicit ApiClient(QObject *parent = nullptr);

    // 运行时可配置的服务根地址（如 http://127.0.0.1:8000）。
    // 为本地部署与远程部署切换预留能力。
    void setBaseUrl(const QUrl &baseUrl);
    QUrl baseUrl() const;

    // 基础健康/版本探针。
    void getVersion();
    // 获取点位集合原始 JSON。
    // 解析策略由调用层决定，避免网络层与业务 schema 强耦合。
    void getPoints();
    // 示例更新接口，携带版本号用于乐观锁。
    // 后续可演进为更完整 DTO/命令模型。
    void updatePoint(int id, int version, const QString &name, const QString &protocol, int registerAddress);

signals:
    // /api/version 解析后发出。
    void versionReceived(const QString &version);
    // 返回原始响应，便于调用层灵活处理。
    void pointsReceived(const QByteArray &payload);
    void pointUpdated(int id);
    // 409 冲突专用信号。
    void conflictDetected(int id);
    void requestFailed(const QString &message);

private:
    // GET 类请求的统一收尾路径。
    // 仅当网络层无错误时才执行 onSuccess。
    void handleReply(QNetworkReply *reply, const std::function<void (QNetworkReply *)> &onSuccess);

private:
    QUrl m_baseUrl;
    // 由本类持有。单实例足够覆盖当前应用请求规模。
    QNetworkAccessManager *m_manager;
};
