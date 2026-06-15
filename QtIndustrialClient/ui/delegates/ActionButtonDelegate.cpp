#include "ui/delegates/ActionButtonDelegate.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionButton>
#include <utility>

ActionButtonDelegate::ActionButtonDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void ActionButtonDelegate::setActionHandler(std::function<void(int, ParameterAction)> handler)
{
    m_handler = std::move(handler);
}

void ActionButtonDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString permission = index.data(Qt::DisplayRole).toString();
    const bool canRead = allowsRead(permission);
    const bool canWrite = allowsWrite(permission);

    painter->save();
    painter->fillRect(option.rect, option.palette.base());

    QRect readRect = option.rect.adjusted(8, 6, -option.rect.width() / 2, -6);
    QRect writeRect = option.rect.adjusted(option.rect.width() / 2, 6, -8, -6);
    if (!canRead) {
        writeRect = option.rect.adjusted(8, 6, -8, -6);
    }

    if (canRead) {
        QStyleOptionButton button;
        button.rect = readRect;
        button.text = QStringLiteral("读取");
        QApplication::style()->drawControl(QStyle::CE_PushButton, &button, painter);
    }

    if (canWrite) {
        QStyleOptionButton button;
        button.rect = writeRect;
        button.text = QStringLiteral("写入");
        QApplication::style()->drawControl(QStyle::CE_PushButton, &button, painter);
    }

    painter->restore();
}

bool ActionButtonDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    Q_UNUSED(model);

    if (event->type() != QEvent::MouseButtonRelease || !m_handler) {
        return false;
    }

    const auto* mouseEvent = static_cast<QMouseEvent*>(event);
    const QString permission = index.data(Qt::DisplayRole).toString();
    const bool canRead = allowsRead(permission);
    const bool canWrite = allowsWrite(permission);

    QRect readRect = option.rect.adjusted(8, 6, -option.rect.width() / 2, -6);
    QRect writeRect = option.rect.adjusted(option.rect.width() / 2, 6, -8, -6);
    if (!canRead) {
        writeRect = option.rect.adjusted(8, 6, -8, -6);
    }

    if (canRead && readRect.contains(mouseEvent->pos())) {
        m_handler(index.row(), ParameterAction::Read);
        return true;
    }

    if (canWrite && writeRect.contains(mouseEvent->pos())) {
        m_handler(index.row(), ParameterAction::Write);
        return true;
    }

    return false;
}

bool ActionButtonDelegate::allowsRead(const QString& permission)
{
    return permission.contains(QLatin1Char('R'));
}

bool ActionButtonDelegate::allowsWrite(const QString& permission)
{
    return permission.contains(QLatin1Char('W'));
}
