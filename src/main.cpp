#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include "Config.h"
#include "phototweet.h"
#include "../QsLog/QsLog.h"
#include "../QsLog/QsLogDest.h"

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
			QLOG_DEBUG() << localMsg.constData();
			break;
		}

		case QtWarningMsg:
		{
			QLOG_WARN() << localMsg.constData();
			break;
		}

		case QtCriticalMsg:
		{
			QLOG_ERROR() << localMsg.constData();
			break;
		}

		case QtFatalMsg:
		{
			QLOG_FATAL() << localMsg.constData();
			break;
		}
    }
}

int main(int argc, char *argv[])
{
	qInstallMessageHandler(messageHandler);

    QCoreApplication app(argc, argv);
	QsLogging::Logger& logger = QsLogging::Logger::instance();
	logger.setLoggingLevel(QsLogging::TraceLevel);
	logger.setTimestampFormat("yyyy-MM-dd hh:mm:ss");

	// Find a free log file name.
	QString logFile(QDir(app.applicationDirPath()).filePath("phototweet"));
	int logNumber = 0;
	while (1)
	{
		QString name = logFile + QString::number(logNumber) + ".log";
		if (!QFile::exists(name))
		{
			logFile = name;
			break;
		}

		logNumber++;
	}

	QsLogging::DestinationPtr fileDestination(QsLogging::DestinationFactory::MakeFileDestination(logFile));
	logger.addDestination(fileDestination.get());

	QsLogging::DestinationPtr debugDestination(QsLogging::DestinationFactory::MakeDebugOutputDestination(true));
	logger.addDestination(debugDestination.get());

	if (!QFile::exists("phototweet.cfg"))
	{
		QLOG_FATAL() << "Couldn't find config file 'phototweet.cfg'!";
		return 1;
	}

	Config config("phototweet.cfg");
	QString loggingLevel = config.GetValue("logging_level");
	logger.setLoggingLevel(loggingLevel);
	QLOG_INFO() << "Phototweet is starting..";

    PhotoTweet pt(&config);
    QObject::connect(&pt, SIGNAL(quit()), &app, SLOT(quit()));
    QMetaObject::invokeMethod(&pt, "main", Qt::QueuedConnection);
    return app.exec();
}
