#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

#include "models/ParameterRecord.h"

class ComboBoxDelegate;

class AddProtocolDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddProtocolDialog(QWidget* parent = nullptr);
    ~AddProtocolDialog();

    void setContext(const QString& protocolType, int groupId, const QString& groupName);
    QVector<ParameterRecord> getRecords() const;

private:
    void initUI();
    void setupTable();
    void updateTable();
    void mergeCells();
    bool validateRecords();

    void onRowCountChanged(const QString& text);
    void onOkButtonClicked();
    void onCancelButtonClicked();

    static QStringList getPermissionOptions();
    static QStringList getDataTypeOptions();

    QString m_protocolType;
    int m_groupId = 0;
    QString m_groupName;

    QTableWidget* m_tableWidget = nullptr;
    QLineEdit* m_rowCountEdit = nullptr;
    QPushButton* m_okButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    QLabel* m_statusLabel = nullptr;

    ComboBoxDelegate* m_permissionDelegate = nullptr;
    ComboBoxDelegate* m_dataTypeDelegate = nullptr;
    ComboBoxDelegate* m_remarkDelegate = nullptr;

    int m_rowCount = 1;

    QVector<ParameterRecord> m_records;
};
