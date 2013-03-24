#include <QCoreApplication>
#include "phototweet.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    PhotoTweet pt;
    if (!pt.loadConfig())
    {
        return 1;
    }

    QObject::connect(&pt, SIGNAL(quit()), &app, SLOT(quit()));
    QMetaObject::invokeMethod(&pt, "main", Qt::QueuedConnection);
    return app.exec();
}
