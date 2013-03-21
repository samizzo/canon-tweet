#include <QCoreApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    MainWindow m;
    QObject::connect(&m, SIGNAL(finished()), &app, SLOT(quit()));
    QMetaObject::invokeMethod(&m, "run", Qt::QueuedConnection);
    return app.exec();
}
