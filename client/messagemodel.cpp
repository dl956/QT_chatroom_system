#include "messagemodel.h"
#include <QDebug>

MessageModel::MessageModel(QObject* parent) : QAbstractListModel(parent) {}

int MessageModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return chat_items_.size();
}

QVariant MessageModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return {};
    const ChatItem& chatItem = chat_items_.at(index.row());
    switch (role) {
    case SenderRole: return chatItem.sender;
    case TextRole: return chatItem.text;
    case TimeRole: return chatItem.time.toString(Qt::ISODate);
    }
    return {};
}

QHash<int, QByteArray> MessageModel::roleNames() const {
    QHash<int, QByteArray> roleNameHash;
    roleNameHash[SenderRole] = "sender";
    roleNameHash[TextRole] = "text";
    roleNameHash[TimeRole] = "time";
    return roleNameHash;
}

void MessageModel::addMessage(const QString& sender, const QString& text, const QDateTime& time) {
    qDebug() << "[MessageModel] addMessage sender=" << sender << "text=" << text << "time=" << time.toString();
    beginInsertRows(QModelIndex(), chat_items_.size(), chat_items_.size());
    chat_items_.append({sender, text, time});
    endInsertRows();
    qDebug() << "[MessageModel] new rowCount=" << chat_items_.size();
}
