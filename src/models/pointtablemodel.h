// 点位表模型（PointTableModel）
// --------------------------------------
// 协议点位数据的 QAbstractTableModel 实现。
//
// 采用 Model 的原因：
// - 视图（QTableView）与数据 schema 解耦。
// - 后续可接代理模型（排序/过滤）而无需重写界面。
// - 提供统一编辑拦截点，便于接乐观更新流程。

#pragma once

#include <QAbstractTableModel>
#include <QVector>

struct PointItem {
    // 后端资源 ID。
    int id = 0;
    // 乐观锁版本号。
    int version = 1;
    QString name;
    QString protocol;
    int registerAddress = 0;
};

class PointTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit PointTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    // 本地编辑写入（是否提交后端由上层决定）。
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    // 全量重置接口（当前数据规模下实现最简单稳妥）。
    void setItems(const QVector<PointItem> &items);
    const QVector<PointItem> &items() const;
    // 安全的行级读取接口，供外部命令处理使用。
    PointItem itemAt(int row) const;
    // 后端更新成功后用于提升 version。
    void updateVersionForRow(int row, int newVersion);

private:
    QVector<PointItem> m_items;
};
