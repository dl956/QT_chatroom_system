#include "tcpclient.h"
#include "messagemodel.h"
#include <QAbstractSocket>
#include <QJsonDocument>
#include <QDateTime>
#include <QDataStream>
#include <QJsonArray>
#include <QStringList>
#include <QDebug>

static QString gCurrentUser; // Used for message deduplication (keep QML currentUser and C++ synchronized)

TcpClient::TcpClient(QObject* parent) : QObject(parent) {
    connect(&socket_, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(&socket_, &QTcpSocket::connected, this, &TcpClient::onConnected);
    connect(&socket_, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);
    connect(&socket_, &QTcpSocket::errorOccurred, this, &TcpClient::onErrorOccurred);

    heartbeatTimer_.setInterval(10000);
    connect(&heartbeatTimer_, &QTimer::timeout, this, &TcpClient::sendHeartbeat);
}

void TcpClient::connectToHost(const QString& host, quint16 port) {
    if (socket_.state() == QAbstractSocket::ConnectedState) socket_.disconnectFromHost();
    socket_.connectToHost(host, port);
}

void TcpClient::disconnectFromHost() {
    if (socket_.state() == QAbstractSocket::ConnectedState) {
        socket_.disconnectFromHost();
    }
}

void TcpClient::onConnected() {
    heartbeatTimer_.start();
    emit connected();
}

void TcpClient::onDisconnected() {
    heartbeatTimer_.stop();
    emit disconnected();
    gCurrentUser.clear();
}

void TcpClient::onErrorOccurred(QAbstractSocket::SocketError socketError) {
    Q_UNUSED(socketError);
    emit errorOccurred(socket_.errorString());
}

void TcpClient::sendJson(const QJsonObject& obj) {
    QString messageType = obj.value("type").toString();
    if (messageType == "message") {
        QString text = obj.value("text").toString();
        if (model) model->addMessage("me", text, QDateTime::currentDateTime());
    } else if (messageType == "login") {
        gCurrentUser = obj.value("username").toString();
    } else if (messageType == "logout") {
        gCurrentUser.clear();
    }

    QJsonDocument jsonDocument(obj);
    QByteArray payload = jsonDocument.toJson(QJsonDocument::Compact);
    QByteArray frame;
    QDataStream dataStream(&frame, QIODevice::WriteOnly);
    dataStream.setByteOrder(QDataStream::BigEndian);
    dataStream << static_cast<quint32>(payload.size());
    frame.append(payload);
    socket_.write(frame);
}

void TcpClient::onReadyRead() {
    QByteArray chunk = socket_.readAll();
    receiveBuffer_.append(chunk);
    while (receiveBuffer_.size() >= 4) {
        QDataStream dataStream(receiveBuffer_);
        dataStream.setByteOrder(QDataStream::BigEndian);
        quint32 frameLength = 0;
        dataStream >> frameLength;
        if (receiveBuffer_.size() < 4 + static_cast<int>(frameLength)) break;
        QByteArray framePayload = receiveBuffer_.mid(4, frameLength);
        receiveBuffer_.remove(0, 4 + frameLength);
        processFrame(framePayload);
    }
}

void TcpClient::processFrame(const QByteArray& payload) {
    QJsonDocument jsonDocument = QJsonDocument::fromJson(payload);
    if (!jsonDocument.isObject()) return;
    QJsonObject obj = jsonDocument.object();
    QString type = obj.value("type").toString();

    if (type == "message" || type == "private") {
        QString from = obj.value("from").toString();
        QString text = obj.value("text").toString();
        qint64 ts = obj.value("ts").toVariant().toLongLong();
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(ts ? ts : QDateTime::currentMSecsSinceEpoch());
        // Only show one message: when receiving an echo from server, don'messageType add again if it's sent by self
        if (from != gCurrentUser && model) model->addMessage(from, text, dt);
        emit messageReceived(from, text, dt.toMSecsSinceEpoch());
    } else if (type == "login_result" || type == "register_result") {
        bool ok = obj.value("ok").toBool();
        QString reason = obj.value("reason").toString();
        QString username = obj.value("username").toString();
        if (type == "login_result") {
            if (ok) {
                emit loginSucceeded(gCurrentUser.isEmpty() ? username : gCurrentUser);
            } else {
                emit loginFailed(reason);
            }
            // 不再向model添加login_result消息
        } else {
            if (ok) emit registerSucceeded();
            else emit registerFailed(reason);
            // 不再向model添加register_result消息
        }
    } else if (type == "pong") {
        // Ignore
    } else if (obj.contains("users") && obj.value("users").isArray()) {
        QJsonArray userArray = obj.value("users").toArray();
        QStringList usernamesList;
        for (const QJsonValue& v : userArray) usernamesList << v.toString();
        emit onlineUsersUpdated(usernamesList);
    }
}

void TcpClient::sendHeartbeat() {
    QJsonObject j;
    j["type"] = "heartbeat";
    sendJson(j);
}
