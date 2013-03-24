#include <QCoreApplication>
#include "mainapp.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    MainApp m;
    QObject::connect(&m, SIGNAL(quit()), &app, SLOT(quit()));
    QMetaObject::invokeMethod(&m, "main", Qt::QueuedConnection);
    return app.exec();
}
