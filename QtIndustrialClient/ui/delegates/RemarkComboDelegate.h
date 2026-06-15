#pragma once

#include <QStyledItemDelegate>
#include <QComboBox>
#include <QLineEdit>

/**
 * @class RemarkComboDelegate
 * @brief 备注列委托类，根据协议类型切换编辑方式
 *
 * A3协议: 使用QComboBox，选项为"绘图"/"不绘图"，默认"绘图"
 * A0协议: 使用QLineEdit，自由文本输入
 *
 * 协议类型通过 item data (Qt::UserRole + 100) 传递
 */
class RemarkComboDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit RemarkComboDelegate(QObject* parent = nullptr);
    ~RemarkComboDelegate() override;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    static constexpr int ProtocolTypeRole = Qt::UserRole + 100;

private:
    // 从index获取协议类型，默认A0
    static quint8 getProtocolType(const QModelIndex& index);
};
