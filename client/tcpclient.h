#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonObject>
#include <QStringList>

class MessageModel;

class TcpClient : public QObject {
    Q_OBJECT
public:
    explicit TcpClient(QObject* parent = nullptr);
    Q_INVOKABLE void connectToHost(const QString& host, quint16 port);
    Q_INVOKABLE void disconnectFromHost();
    Q_INVOKABLE void sendJson(const QJsonObject& obj);
    void setModel(MessageModel* m) { model = m; }

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString&);

    // New signals for UI
    void loginSucceeded(const QString& username);
    void loginFailed(const QString& reason);
    void registerSucceeded();
    void registerFailed(const QString& reason);

    void onlineUsersUpdated(const QStringList& users);

    // Emitted when a message frame is received (also model is updated)
    void messageReceived(const QString& from, const QString& text, qint64 ts);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError);

    void sendHeartbeat();

private:
    void processFrame(const QByteArray& framePayload);
    QTcpSocket socket_;
    QByteArray receiveBuffer_;
    MessageModel* model = nullptr;
    QTimer heartbeatTimer_;
};
