#include <QCoreApplication>
#include "phototweet.h"

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
	QByteArray localMsg = msg.toLocal8Bit();
    switch (type)
	{
		case QtDebugMsg:
		{
			printf("DEBUG: %s (%s:%u)\n", localMsg.constData(), context.file, context.line);
			break;
		}

		case QtWarningMsg:
		{
			printf("WARNING: %s (%s:%u)\n", localMsg.constData(), context.file, context.line);
			break;
		}

		case QtCriticalMsg:
		{
			printf("CRITICAL: %s (%s:%u)\n", localMsg.constData(), context.file, context.line);
			break;
		}

		case QtFatalMsg:
		{
			printf("FATAL: %s (%s:%u)\n", localMsg.constData(), context.file, context.line);
			abort();
		}
    }
}

int main(int argc, char *argv[])
{
	qInstallMessageHandler(messageHandler);
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
