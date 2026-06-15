#pragma once

#include <QAbstractTableModel>
#include <QVector>

#include "models/ParameterRecord.h"

class ParameterTableModel : public QAbstractTableModel {
public:
    enum Column {
        ParameterNameColumn = 0,
        SubCommandColumn,
        PermissionColumn,
        ReadIndexColumn,
        DataTypeColumn,
        DataLengthColumn,
        DescriptionColumn,
        DefaultValueColumn,
        UnitColumn,
        StorageColumn,
        MaxValueColumn,
        MinValueColumn,
        StepColumn,
        EnumColumn,
        RemarkColumn,
        ActionColumn,
        ColumnCount
    };

    explicit ParameterTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setRecords(QVector<ParameterRecord> records);
    const ParameterRecord& recordAt(int row) const;

private:
    QVector<ParameterRecord> m_records;
};
