#include "messagemodel.h"
#include <QDebug>

MessageModel::MessageModel(QObject* parent) : QAbstractListModel(parent) {}

int MessageModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return items_.size();
}

QVariant MessageModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return {};
    const ChatItem& it = items_.at(index.row());
    switch (role) {
    case SenderRole: return it.sender;
    case TextRole: return it.text;
    case TimeRole: return it.time.toString(Qt::ISODate);
    }
    return {};
}

QHash<int, QByteArray> MessageModel::roleNames() const {
    QHash<int, QByteArray> r;
    r[SenderRole] = "sender";
    r[TextRole] = "text";
    r[TimeRole] = "time";
    return r;
}

void MessageModel::addMessage(const QString& sender, const QString& text, const QDateTime& time) {
    qDebug() << "[MessageModel] addMessage sender=" << sender << "text=" << text << "time=" << time.toString();
    beginInsertRows(QModelIndex(), items_.size(), items_.size());
    items_.append({sender, text, time});
    endInsertRows();
    qDebug() << "[MessageModel] new rowCount=" << items_.size();
}