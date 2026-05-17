#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <qqmlcontext.h>
#include <thread>
#include <chrono>
#include <iostream>
#include "core/systemSnapshot.hpp"
#include <QtCore/qstringliteral.h>




int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("helloText", "Hello From WSL");
    SystemSnapshot snap;
    std::cout << "Cpp version: " << __cplusplus << '\n';
    static_assert(__cplusplus >= 202002L, "C++20 minimum required");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    snap.update();
    snap.print();
    engine.load(QUrl(u"qrc:/qml/qml/main.qml"_qs));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
