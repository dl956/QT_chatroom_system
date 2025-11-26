#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "tcpclient.h"
#include "messagemodel.h"

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    TcpClient tcp;
    MessageModel model;
    tcp.setModel(&model);

    engine.rootContext()->setContextProperty("tcpClient", &tcp);
    engine.rootContext()->setContextProperty("messageModel", &model);

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty()) return -1;
    return app.exec();
}
