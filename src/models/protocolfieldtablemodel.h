// 协议字段表模型（ProtocolFieldTableModel）
// -----------------------------------------
// 将 BinProtocol.fields 渲染为可编辑表格。
//
// 设计参考 PointTableModel：
// - QAbstractTableModel，视图与 schema 解耦。
// - 列改用 enum Column 而非裸数字，便于维护。
// - 提供增删/全量重置接口，供协议编辑页使用。

#pragma once

#include "binprotocolformat.h"

#include <QAbstractTableModel>
#include <QVector>

class ProtocolFieldTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        NameCol = 0,     // 字段名（可编辑）
        TypeCol,         // 类型（可编辑，字符串形式）
        OffsetCol,       // 字节偏移
        LengthCol,       // 字节长度（可编辑）
        BitOffsetCol,    // 位偏移（可编辑）
        BitLengthCol,    // 位长（可编辑）
        CommentCol,      // 备注（可编辑）
        ColCount
    };

    explicit ProtocolFieldTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    // 全量重置（加载协议后调用）。
    void setFields(const QVector<FieldDef> &fields);
    const QVector<FieldDef> &fields() const { return m_fields; }
    FieldDef fieldAt(int row) const;

    // 编辑操作（协议编辑页用）。
    void addField(const FieldDef &field);
    void removeRows(int row, int count);
    void moveRow(int row, bool up);

    // 类型枚举 ↔ 显示字符串互转（公开工具，供其它页面复用）。
    static QString typeToString(FieldType t);
    static FieldType stringToType(const QString &s, bool *ok = nullptr);

private:
    QVector<FieldDef> m_fields;
};
