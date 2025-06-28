#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{    
    QGuiApplication app(argc, argv);

    app.setWindowIcon(QIcon("MySongPlayer/assets/icons/app_icon.ico"));

    QQmlApplicationEngine engine;

    QObject::connect(
      &engine,
      &QQmlApplicationEngine::objectCreationFailed,
      &app,
      []() { QCoreApplication::exit(-1); },
      Qt::QueuedConnection);
    engine.loadFromModule("MySongPlayer", "Main");

    return app.exec();
}
