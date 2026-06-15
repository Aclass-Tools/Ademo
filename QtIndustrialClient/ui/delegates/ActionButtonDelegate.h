#pragma once

#include <functional>

#include <QStyledItemDelegate>

#include "models/ParameterRecord.h"

class ActionButtonDelegate : public QStyledItemDelegate {
public:
    explicit ActionButtonDelegate(QObject* parent = nullptr);

    void setActionHandler(std::function<void(int, ParameterAction)> handler);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

private:
    static bool allowsRead(const QString& permission);
    static bool allowsWrite(const QString& permission);

    std::function<void(int, ParameterAction)> m_handler;
};
