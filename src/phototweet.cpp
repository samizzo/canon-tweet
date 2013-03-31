#include <QNetworkAccessManager>
#include <QtWidgets/QApplication>
#include <QStringList>
#include <QtDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include "phototweet.h"
#include "oauthtwitter.h"
#include "qtweetstatusupdate.h"
#include "qtweetstatusupdatewithmedia.h"
#include "qtweetstatus.h"
#include "qtweetconfiguration.h"
#include <QJsonDocument>
#include <QJsonObject>
#include "twitpicUpload.h"
#include "twitpicUploadStatus.h"
#include "yfrogUpload.h"
#include "yfrogUploadStatus.h"

void getInput(const char* msg, char* buf, int bufSize)
{
	if (msg)
	{
		printf("%s ", msg);
	}

	memset(buf, 0, bufSize);
	fgets(buf, bufSize-1, stdin);
	buf[strlen(buf)-1] = 0;
}

PhotoTweet::PhotoTweet() :
m_processing(false),
m_quit(true)
{
    m_oauthTwitter = new OAuthTwitter(this);
    m_oauthTwitter->setNetworkAccessManager(new QNetworkAccessManager(this));

    m_tweetConfig = new QTweetConfiguration(m_oauthTwitter, this);
    connect(m_tweetConfig, SIGNAL(configuration(QJsonDocument)), SLOT(getConfigurationFinished(QJsonDocument)));
    connect(m_tweetConfig, SIGNAL(error(QTweetNetBase::ErrorCode, QString)), SLOT(postStatusError(QTweetNetBase::ErrorCode, QString)));

	m_statusUpdate = new QTweetStatusUpdate(m_oauthTwitter, this);
    connect(m_statusUpdate, SIGNAL(postedStatus(QTweetStatus)), SLOT(postStatusFinished(QTweetStatus)));
    connect(m_statusUpdate, SIGNAL(error(QTweetNetBase::ErrorCode, QString)), SLOT(postStatusError(QTweetNetBase::ErrorCode, QString)));
    connect(m_statusUpdate, SIGNAL(finished(QByteArray, QNetworkReply)), SLOT(replyFinished(QByteArray, QNetworkReply)));

	m_twitpic = new TwitpicUpload(m_twitpicApiKey, m_oauthTwitter, this);
	connect(m_twitpic, SIGNAL(jsonParseError(QByteArray)), SLOT(twitpicJsonParseError(QByteArray)));
    connect(m_twitpic, SIGNAL(error(QTweetNetBase::ErrorCode, QString)), SLOT(twitpicError(QTweetNetBase::ErrorCode, QString)));
    connect(m_twitpic, SIGNAL(finished(TwitpicUploadStatus)), SLOT(twitpicFinished(TwitpicUploadStatus)));

	m_yfrog = new YfrogUpload(m_yfrogApiKey, m_oauthTwitter, this);
    connect(m_yfrog, SIGNAL(error(QTweetNetBase::ErrorCode, YfrogUploadStatus)), SLOT(yfrogError(QTweetNetBase::ErrorCode, YfrogUploadStatus)));
    connect(m_yfrog, SIGNAL(finished(YfrogUploadStatus)), SLOT(yfrogFinished(YfrogUploadStatus)));
}

bool PhotoTweet::loadConfig()
{
    QFile configFile("phototweet.cfg");
    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning("Couldn't load config file 'phototweet.cfg'");
        return false;
    }

    QTextStream reader(&configFile);
    while (!reader.atEnd())
    {
        QString line = reader.readLine().trimmed();
        if (line.length() > 0 && line.at(0) == '[' && line.endsWith(']'))
        {
            QString key = line.mid(1, line.length() - 2);
            QString val = reader.readLine().trimmed();
            if (val.length() > 0)
            {
                if (!key.compare("consumer_key"))
                {
                    m_oauthTwitter->setConsumerKey(val.toLatin1());
                }
                else if (!key.compare("consumer_secret"))
                {
                    m_oauthTwitter->setConsumerSecret(val.toLatin1());
                }
                else if (!key.compare("oauth_token"))
                {
                    m_oauthTwitter->setOAuthToken(val.toLatin1());
                }
                else if (!key.compare("oauth_token_secret"))
                {
                    m_oauthTwitter->setOAuthTokenSecret(val.toLatin1());
                }
				else if (!key.compare("twitpic_api_key"))
				{
					m_twitpicApiKey = val;
				}
				else if(!key.compare("yfrog_api_key"))
				{
					m_yfrogApiKey = val;
				}
            }
        }
    }

    return true;
}

void PhotoTweet::showUsage()
{
    printf("photoTweet.exe [args]\n\n");
    printf("where args are:\n\n");
    printf("-image <image path>         upload specified image\n");
    printf("-message <message text>     tweet specified status text\n");
    printf("-getconfig                  return media configuration settings\n");
	printf("-console                    run in console mode\n");
    printf("\n");
    printf("Note: a message must always be specified if tweeting.  Use quotes\n");
    printf("around the message text to specify multiple words.\n");
}

void PhotoTweet::runConsole()
{
	while (1)
	{
		QApplication::processEvents();
		while (m_processing)
		{
			QApplication::processEvents();
		}
		QApplication::processEvents();

		static const int MAX_BUF = 128;
		char buf[MAX_BUF];
		getInput(0, buf, MAX_BUF);
		if (!_stricmp(buf, "quit"))
		{
			return doQuit();
		}
		else if (!_stricmp(buf, "post"))
		{
			getInput("Enter message:", buf, MAX_BUF);
			run(QString(buf));
		}
		else if (!_stricmp(buf, "postimage"))
		{
			getInput("Enter message:", buf, MAX_BUF);
			QString msg(buf);
			getInput("Enter image path:", buf, MAX_BUF);
			QString img(buf);
			run(msg, img);
		}
		else if (!_stricmp(buf, "getconfig"))
		{
			getConfiguration();
		}
		else
		{
			printf("commands: quit, post, postimage, getconfig\n");
		}
	}
}

void PhotoTweet::main()
{
    QStringList& args = QApplication::arguments();
    bool error = false, console = false;

    QString message, imagePath;

    for (int i = 0; i < args.count(); i++)
    {
        if (!args.at(i).compare("-message"))
        {
            if (i + 1 < args.count())
            {
                message = args.at(i + 1);
            }
            else
            {
                printf("Missing argument to -message!\n");
                error = true;
                break;
            }
        }
        else if (!args.at(i).compare("-image"))
        {
            if (i + 1 < args.count())
            {
                imagePath = args.at(i + 1);
                if (!QFile::exists(imagePath))
                {
                    printf("Couldn't find file %s!\n", imagePath.toLatin1().constData());
                    error = true;
                }
            }
            else
            {
                printf("Missing argument to -image!\n");
                error = true;
                break;
            }
        }
        else if (!args.at(i).compare("-getconfig"))
        {
            getConfiguration();
            return;
        }
		else if (!args.at(i).compare("-console"))
		{
			console = true;
			m_quit = false;
		}
    }

	if (console)
	{
		return runConsole();
	}

    if (message.length() == 0)
    {
        showUsage();
        return doQuit();
    }

    if (error)
    {
        return doQuit();
    }

    run(message, imagePath);
}

void PhotoTweet::run(QString& message, QString& imagePath)
{
	m_processing = true;
    m_message = message;
    m_imagePath = imagePath;

    qDebug("Tweeting message: '%s'", m_message.toLatin1().constData());
    if (m_imagePath.length() > 0)
    {
        qDebug("Attaching image: '%s'", m_imagePath.toLatin1().constData());
		postMessageWithImageYfrog();
    }
    else
    {
        postMessage();
    }
}

void PhotoTweet::doQuit()
{
    emit quit();
}

void PhotoTweet::getConfiguration()
{
	m_processing = true;
    m_tweetConfig->get();
}

void PhotoTweet::printObject(const QVariant& object)
{
    if (object.type() == QVariant::Map)
    {
        QVariantMap m = object.toMap();
        QList<QString> keys = m.keys();
        for (int i = 0; i < keys.count(); i++)
        {
            QVariant value = m[keys[i]];
            QString key = keys[i];
            printf("%s=", key.toLatin1().constData());
            if (value.type() == QVariant::String || value.type() == QVariant::Int || value.type() == QVariant::Double)
            {
                printf("%s", value.toString().toLatin1().constData());
            }
            else
            {
                printf("(%s)", value.typeName());
            }
            if (i < keys.count() - 1)
            {
                printf(", ");
            }
        }
    }
}

void PhotoTweet::getConfigurationFinished(const QJsonDocument& json)
{
    QJsonObject response = json.object();
    QStringList& keys = response.keys();
    for (int i = 0; i < keys.count(); i++)
    {
        QString key = keys[i];
        QJsonValue value = response[key];
        printf("%s=", key.toLatin1().constData());

        if (value.isArray())
        {
            printf("<array>\n");
        }
        else if (value.isObject())
        {
            printf("\n");
            QVariant v = value.toVariant();
            QVariantMap m = v.toMap();
            QList<QString> k = m.keys();
            for (int i = 0; i < k.count(); i++)
            {
                printf("   %s={ ", k[i].toLatin1().constData());
                printObject(m[k[i]]);
                printf(" }\n");
            }
        }
        else
        {
            QVariant v = value.toVariant();
            QString s = v.toString();
            printf("%s\n", s.toLatin1().constData());
        }
    }

	m_processing = false;

	if (m_quit)
	{
		return doQuit();
	}
}

void PhotoTweet::postMessage()
{
	m_statusUpdate->post(m_message);
}

void PhotoTweet::postMessageWithImageTwitpic()
{
	if (QFile::exists(m_imagePath))
	{
		m_twitpic->upload(m_imagePath);
	}
	else
	{
		qWarning("Couldn't find file %s", m_imagePath.toLatin1().constData());
		qWarning("Aborting post..");
		m_processing = false;
	}
}

void PhotoTweet::postMessageWithImageYfrog()
{
	if (QFile::exists(m_imagePath))
	{
		m_yfrog->upload(m_imagePath);
	}
	else
	{
		qWarning("Couldn't find file %s", m_imagePath.toLatin1().constData());
		m_processing = false;
	}
}

void PhotoTweet::postStatusFinished(const QTweetStatus &status)
{
	qDebug("Posted status with id %llu", status.id());

	m_processing = false;

	if (m_quit)
	{
		doQuit();
	}
}

void PhotoTweet::postStatusError(QTweetNetBase::ErrorCode, QString errorMsg)
{
    if (errorMsg.length() > 0)
    {
        qWarning("Error posting message: %s", errorMsg.toLatin1().constData());
    }

	m_processing = false;

	if (m_quit)
	{
		doQuit();
	}
}

void PhotoTweet::replyFinished(const QByteArray&, const QNetworkReply& reply)
{
    QList<QByteArray> headers = reply.rawHeaderList();

    bool haveLimit = false, haveRemaining = false, haveReset = false;
    int limit, remaining;
    QDateTime reset;

    if (reply.hasRawHeader("X-MediaRateLimit-Limit"))
    {
        //qDebug("X-MediaRateLimit-Limit: %s", reply.rawHeader("X-MediaRateLimit-Limit").constData());
        limit = reply.rawHeader("X-MediaRateLimit-Limit").toInt();
        haveLimit = true;
    }

    if (reply.hasRawHeader("X-MediaRateLimit-Remaining"))
    {
        //qDebug("X-MediaRateLimit-Remaining: %s", reply.rawHeader("X-MediaRateLimit-Remaining").constData());
        remaining = reply.rawHeader("X-MediaRateLimit-Remaining").toInt();
        haveRemaining = true;
    }

    if (reply.hasRawHeader("X-MediaRateLimit-Reset"))
    {
        //qDebug("X-MediaRateLimit-Reset: %s", reply.rawHeader("X-MediaRateLimit-Reset").constData());
        reset.setTime_t(reply.rawHeader("X-MediaRateLimit-Reset").toInt());
        haveReset = true;
    }

    if (haveLimit && haveRemaining && haveReset)
    {
        printf("You have %i tweets with media remaining out of a total of %i.\n", remaining, limit);
        printf("This limit will reset at %s.\n", reset.toLocalTime().toString().toLatin1().constData());
    }

	m_processing = false;
}

void PhotoTweet::twitpicError(QTweetNetBase::ErrorCode, QString errorMsg)
{
	qWarning("Error posting image to twitpic: %s", errorMsg.toLatin1().constData());

	m_processing = false;

	if (m_quit)
	{
		doQuit();
	}
}

void PhotoTweet::twitpicJsonParseError(const QByteArray& json)
{
	qWarning("Error parsing json result while posting image to twitpic");
	qWarning("json: %s", json.constData());

	m_processing = false;

	if (m_quit)
	{
		doQuit();
	}
}

void PhotoTweet::twitpicFinished(const TwitpicUploadStatus& status)
{
	qDebug("Posted image to twitpic!");
	qDebug("Url is %s", status.getImageUrl().toLatin1().constData());
	if (m_message.length() > 0)
	{
		m_message += " ";
	}
	m_message += status.getImageUrl();
	qDebug("Posting link to twitter..");
	postMessage();
}

void PhotoTweet::yfrogError(QTweetNetBase::ErrorCode, const YfrogUploadStatus& status)
{
	qWarning("Error posting image to yfrog: %s", status.getStatusString().toLatin1().constData());

	m_processing = false;

	if (m_quit)
	{
		doQuit();
	}
}

void PhotoTweet::yfrogFinished(const YfrogUploadStatus& status)
{
	if (status.getStatus() != YfrogUploadStatus::Ok)
	{
		qWarning("Error posting image to yfrog: %s", status.getStatusString().toLatin1().constData());

		m_processing = false;

		if (m_quit)
		{
			doQuit();
		}
	}
	else
	{
		qDebug("Posted image to yfrog!");
		qDebug("Url is %s", status.getMediaUrl().toLatin1().constData());
		if (m_message.length() > 0)
		{
			m_message += " ";
		}

		m_message += status.getMediaUrl();
		qDebug("Posting link to twitter..");
		postMessage();
	}
}


