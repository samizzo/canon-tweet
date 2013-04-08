#include <QCoreApplication>
#include <QDateTime>
#include "phototweet.h"

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
	(void)context;
	QDateTime now = QDateTime::currentDateTime();
	QDate nowDate = now.date();
	QTime nowTime = now.time();
	QByteArray localMsg = msg.toLocal8Bit();
    switch (type)
	{
		case QtDebugMsg:
		{
			printf("DEBUG: %02i/%02i/%4i %02i:%02i:%02i %s\n",
				nowDate.day(), nowDate.month(), nowDate.year(), nowTime.hour(), nowTime.minute(), nowTime.second(),
				localMsg.constData());
			break;
		}

		case QtWarningMsg:
		{
			printf("WARNING: %02i/%02i/%4i %02i:%02i:%02i %s\n",
				nowDate.day(), nowDate.month(), nowDate.year(), nowTime.hour(), nowTime.minute(), nowTime.second(),
				localMsg.constData());
			break;
		}

		case QtCriticalMsg:
		{
			printf("CRITICAL: %02i/%02i/%4i %02i:%02i:%02i %s\n",
				nowDate.day(), nowDate.month(), nowDate.year(), nowTime.hour(), nowTime.minute(), nowTime.second(),
				localMsg.constData());
			break;
		}

		case QtFatalMsg:
		{
			printf("FATAL: %02i/%02i/%4i %02i:%02i:%02i %s\n",
				nowDate.day(), nowDate.month(), nowDate.year(), nowTime.hour(), nowTime.minute(), nowTime.second(),
				localMsg.constData());
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
