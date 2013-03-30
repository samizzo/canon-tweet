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

PhotoTweet::PhotoTweet() :
m_doPost(false),
m_photoSizeLimit(0)
{
    m_oauthTwitter = new OAuthTwitter(this);
    m_oauthTwitter->setNetworkAccessManager(new QNetworkAccessManager(this));
}

bool PhotoTweet::loadConfig()
{
    QFile configFile("phototweet.cfg");
    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        printf("Couldn't load config file phototweet.cfg\n");
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
    printf("-getConfig                  return media configuration settings\n");
    printf("\n");
    printf("Note: a message must always be specified if tweeting.  Use quotes\n");
    printf("around the message text to specify multiple words.\n");
}

void PhotoTweet::main()
{
    QStringList& args = QApplication::arguments();
    bool error = false;

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
        else if (!args.at(i).compare("-getConfig"))
        {
            getConfiguration();
            return;
        }
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
    m_message = message;
    m_imagePath = imagePath;

    printf("Tweeting message: '%s'\n", m_message.toLatin1().constData());
    if (m_imagePath.length() > 0)
    {
        printf("Attaching image: '%s'\n", m_imagePath.toLatin1().constData());
		postMessageWithImage();
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
    QTweetConfiguration* config = new QTweetConfiguration(m_oauthTwitter, this);
    connect(config, SIGNAL(configuration(QJsonDocument)), SLOT(getConfigurationFinished(QJsonDocument)));
    connect(config, SIGNAL(error(QTweetNetBase::ErrorCode, QString)), SLOT(postStatusError(QTweetNetBase::ErrorCode, QString)));
    config->get();
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
        if (m_doPost)
        {
            if (value.isDouble() && !key.compare("photo_size_limit"))
            {
                m_photoSizeLimit = (int)value.toDouble();
                break;
            }
        }
        else
        {
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
    }

	return doQuit();
}

void PhotoTweet::postMessage()
{
    QTweetStatusUpdate *statusUpdate = new QTweetStatusUpdate(m_oauthTwitter, this);
    connect(statusUpdate, SIGNAL(postedStatus(QTweetStatus)), SLOT(postStatusFinished(QTweetStatus)));
    connect(statusUpdate, SIGNAL(error(QTweetNetBase::ErrorCode, QString)), SLOT(postStatusError(QTweetNetBase::ErrorCode, QString)));
    connect(statusUpdate, SIGNAL(finished(QByteArray, QNetworkReply)), SLOT(replyFinished(QByteArray, QNetworkReply)));
    statusUpdate->post(m_message);
}

void PhotoTweet::postMessageWithImage()
{
	TwitpicUpload* upload = new TwitpicUpload(m_twitpicApiKey, m_oauthTwitter, this);
	connect(upload, SIGNAL(jsonParseError(QByteArray)), SLOT(twitpicJsonParseError(QByteArray)));
    connect(upload, SIGNAL(error(QTweetNetBase::ErrorCode, QString)), SLOT(twitpicError(QTweetNetBase::ErrorCode, QString)));
    connect(upload, SIGNAL(finished(TwitpicUploadStatus)), SLOT(twitpicFinished(TwitpicUploadStatus)));
	upload->upload(m_message, m_imagePath);
}

void PhotoTweet::postStatusFinished(const QTweetStatus &status)
{
    QTweetStatusUpdate *statusUpdate = qobject_cast<QTweetStatusUpdate*>(sender());
    if (statusUpdate)
    {
        printf("Posted status with id %llu\n", status.id());
        statusUpdate->deleteLater();
    }
    return doQuit();
}

void PhotoTweet::postStatusError(QTweetNetBase::ErrorCode, QString errorMsg)
{
    if (errorMsg.length() > 0)
    {
        printf("Error posting message: %s\n", errorMsg.toLatin1().constData());
    }
    return doQuit();
}

void PhotoTweet::replyFinished(const QByteArray&, const QNetworkReply& reply)
{
    QList<QByteArray> headers = reply.rawHeaderList();

    bool haveLimit = false, haveRemaining = false, haveReset = false;
    int limit, remaining;
    QDateTime reset;

    if (reply.hasRawHeader("X-MediaRateLimit-Limit"))
    {
        //printf("X-MediaRateLimit-Limit: %s\n", reply.rawHeader("X-MediaRateLimit-Limit").constData());
        limit = reply.rawHeader("X-MediaRateLimit-Limit").toInt();
        haveLimit = true;
    }

    if (reply.hasRawHeader("X-MediaRateLimit-Remaining"))
    {
        //printf("X-MediaRateLimit-Remaining: %s\n", reply.rawHeader("X-MediaRateLimit-Remaining").constData());
        remaining = reply.rawHeader("X-MediaRateLimit-Remaining").toInt();
        haveRemaining = true;
    }

    if (reply.hasRawHeader("X-MediaRateLimit-Reset"))
    {
        //printf("X-MediaRateLimit-Reset: %s\n", reply.rawHeader("X-MediaRateLimit-Reset").constData());
        reset.setTime_t(reply.rawHeader("X-MediaRateLimit-Reset").toInt());
        haveReset = true;
    }

    if (haveLimit && haveRemaining && haveReset)
    {
        printf("\nYou have %i tweets with media remaining out of a total of %i.\n", remaining, limit);
        printf("This limit will reset at %s.\n", reset.toLocalTime().toString().toLatin1().constData());
    }
}

void PhotoTweet::twitpicError(QTweetNetBase::ErrorCode errorCode, QString errorMsg)
{
	printf("Error posting image to twitpic:\n%i %s\n", errorCode, errorMsg.toLatin1().constData());
	doQuit();
}

void PhotoTweet::twitpicJsonParseError(const QByteArray& json)
{
	printf("Error parsing json result while posting image to twitpic\n");
	printf("json: %s\n", json.constData());
	doQuit();
}

void PhotoTweet::twitpicFinished(const TwitpicUploadStatus& status)
{
	printf("Posted image to twitpic!  Url is %s\n", status.getImageUrl().toLatin1().constData());
	doQuit();
}

