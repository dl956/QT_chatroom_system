#pragma once
#include <QAbstractListModel>
#include <QDateTime>

struct ChatItem {
    QString sender;
    QString text;
    QDateTime time;
};

class MessageModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { SenderRole = Qt::UserRole + 1, TextRole, TimeRole };
    explicit MessageModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void addMessage(const QString& sender, const QString& text, const QDateTime& time);

private:
    QList<ChatItem> chat_items_;
};
