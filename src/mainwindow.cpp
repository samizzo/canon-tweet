#include <QNetworkAccessManager>
#include <QApplication>
#include <QStringList>
#include <QtDebug>
#include <QFile>
#include "mainwindow.h"
#include "oauthtwitter.h"
#include "qtweetstatusupdate.h"
#include "qtweetstatus.h"
#include "qtweetconfiguration.h"
#include "json/qjsondocument.h"
#include "json/qjsonobject.h"

MainWindow::MainWindow()
{
    m_oauthTwitter = new OAuthTwitter(this);
    m_oauthTwitter->setNetworkAccessManager(new QNetworkAccessManager(this));
}

void MainWindow::showUsage()
{
    printf("photoTweet.exe [args]\n\n");
    printf("where args are:\n\n");
    printf("-image <image path>         upload specified image\n");
    printf("-message <message text>     tweet specified status text\n");
    printf("-getConfig                  return the twitter configuration\n");
    printf("\n");
    printf("Note: a message must always be specified\n");
}

void MainWindow::run()
{
    // TODO: Read these from a file.
    m_oauthTwitter->setConsumerKey("8Y0v3tSsxiTVE8EPK93bKg");
    m_oauthTwitter->setConsumerSecret("38THDrK3hoFWNVYXMhS953KAqt1MgiYgxfxRvR0ROFQ");
    m_oauthTwitter->setOAuthToken("1264390094-1umU5vWcNDlFobbDAlwdJu9aRa7cW7xPGubE7wa");
    m_oauthTwitter->setOAuthTokenSecret("8ZOr4y8NMVwAWeHnNloYCSgZ3rhrLIbhn2DF7msrvwM");

    QString message;
    QString imagePath;

    QStringList& args = QApplication::arguments();
    bool error = false;

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
                    printf("Couldn't find file %s!\n", imagePath.toAscii().constData());
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

    // TODO: Some other form of auth?

    printf("tweeting message: '%s'\n", message.toAscii().constData());
    if (imagePath.length() > 0)
    {
        printf("attaching image: '%s'\n", imagePath.toAscii().constData());
        postMessageWithImage(message, imagePath);
    }
    else
    {
        postMessage(message);
    }
}

void MainWindow::doQuit()
{
    emit quit();
}

void MainWindow::getConfiguration()
{
    QTweetConfiguration* config = new QTweetConfiguration(m_oauthTwitter, this);
    connect(config, SIGNAL(configuration(QJsonDocument)), SLOT(getConfigurationFinished(QJsonDocument)));
    connect(config, SIGNAL(error(QTweetNetBase::ErrorCode, QString)), SLOT(postStatusError(QTweetNetBase::ErrorCode, QString)));
    config->get();
}

void MainWindow::printObject(const QVariant& object)
{
    if (object.type() == QVariant::Map)
    {
        QVariantMap m = object.toMap();
        QList<QString> keys = m.keys();
        for (int i = 0; i < keys.count(); i++)
        {
            QVariant value = m[keys[i]];
            QString key = keys[i];
            printf("%s=", key.toAscii().constData());
            if (value.type() == QVariant::String || value.type() == QVariant::Int || value.type() == QVariant::Double)
            {
                printf("%s", value.toString().toAscii().constData());
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

void MainWindow::getConfigurationFinished(const QJsonDocument& json)
{
    QJsonObject response = json.object();
    QStringList& keys = response.keys();
    for (int i = 0; i < keys.count(); i++)
    {
        QJsonValue value = response[keys[i]];
        printf("%s=", keys[i].toAscii().constData());
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
                printf("   %s={ ", k[i].toAscii().constData());
                printObject(m[k[i]]);
                printf(" }\n");
            }
        }
        else
        {
            QVariant v = value.toVariant();
            QString s = v.toString();
            printf("%s\n", s.toAscii().constData());
        }
    }
    return doQuit();
}

void MainWindow::postMessage(const QString& message)
{
    QTweetStatusUpdate *statusUpdate = new QTweetStatusUpdate(m_oauthTwitter, this);
    connect(statusUpdate, SIGNAL(postedStatus(QTweetStatus)), SLOT(postStatusFinished(QTweetStatus)));
    connect(statusUpdate, SIGNAL(error(QTweetNetBase::ErrorCode, QString)), SLOT(postStatusError(QTweetNetBase::ErrorCode, QString)));
    connect(statusUpdate, SIGNAL(finished(QByteArray, QNetworkReply)), SLOT(replyFinished(QByteArray, QNetworkReply)));
    statusUpdate->post(message);
}

void MainWindow::postMessageWithImage(const QString& message, const QString& imagePath)
{
    QTweetStatusUpdate *statusUpdate = new QTweetStatusUpdate(m_oauthTwitter, this);
    connect(statusUpdate, SIGNAL(postedStatus(QTweetStatus)), SLOT(postStatusFinished(QTweetStatus)));
    connect(statusUpdate, SIGNAL(error(QTweetNetBase::ErrorCode, QString)), SLOT(postStatusError(QTweetNetBase::ErrorCode, QString)));
    connect(statusUpdate, SIGNAL(finished(QByteArray, QNetworkReply)), SLOT(replyFinished(QByteArray, QNetworkReply)));
    statusUpdate->post(message, imagePath, 0, QTweetGeoCoord(-37.83148, 144.9646), QString(), true);
}

void MainWindow::postStatusFinished(const QTweetStatus &status)
{
    QTweetStatusUpdate *statusUpdate = qobject_cast<QTweetStatusUpdate*>(sender());
    if (statusUpdate)
    {
        printf("Posted status with id %llu\n", status.id());
        statusUpdate->deleteLater();
    }
    return doQuit();
}

void MainWindow::postStatusError(QTweetNetBase::ErrorCode, QString errorMsg)
{
    if (errorMsg.length() > 0)
    {
        printf("Error posting message: %s\n", errorMsg.toAscii().constData());
    }
    return doQuit();
}

void MainWindow::replyFinished(const QByteArray&, const QNetworkReply& reply)
{
    QList<QByteArray> headers = reply.rawHeaderList();
    if (reply.hasRawHeader("X-MediaRateLimit-Limit"))
    {
        printf("X-MediaRateLimit-Limit: %s\n", reply.rawHeader("X-MediaRateLimit-Limit").constData());
    }

    if (reply.hasRawHeader("X-MediaRateLimit-Remaining"))
    {
        printf("X-MediaRateLimit-Remaining: %s\n", reply.rawHeader("X-MediaRateLimit-Remaining").constData());
    }

    if (reply.hasRawHeader("X-MediaRateLimit-Reset"))
    {
        printf("X-MediaRateLimit-Reset: %s\n", reply.rawHeader("X-MediaRateLimit-Reset").constData());
    }
}

