#include <QNetworkAccessManager>
#include <QApplication>
#include <QStringList>
#include <QtDebug>
#include "mainwindow.h"
#include "oauthtwitter.h"
#include "qtweetstatusupdate.h"
#include "qtweetstatus.h"
#include "qtweetconfiguration.h"
#include "json/qjsondocument.h"
#include "json/qjsonobject.h"

MainWindow::MainWindow()
{
    m_authorized = false;

    m_oauthTwitter = new OAuthTwitter(this);
    m_oauthTwitter->setNetworkAccessManager(new QNetworkAccessManager(this));
}

void MainWindow::showUsage()
{
    printf("photoTweet.exe -message <message text> [-image <image path>]\n");
    printf("tweets <message text> to the account, optionally attaching the\n");
    printf("image specified by <image path> to the tweet.");
}

void MainWindow::run()
{
    // TODO: Read these from a file.
    m_authorized = true;
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
        else if (!args.at(i).compare("-imagePath"))
        {
            if (i + 1 < args.count())
            {
                imagePath = args.at(i + 1);
            }
            else
            {
                printf("Missing argument to -imagePath!\n");
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

    if (message.length() == 0 || error)
    {
        showUsage();
        return quit();
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

void MainWindow::quit()
{
    emit finished();
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
}

void MainWindow::postMessage(const QString& message)
{
    QTweetStatusUpdate *statusUpdate = new QTweetStatusUpdate(m_oauthTwitter, this);
    connect(statusUpdate, SIGNAL(postedStatus(QTweetStatus)), SLOT(postStatusFinished(QTweetStatus)));
    connect(statusUpdate, SIGNAL(error(QTweetNetBase::ErrorCode, QString)), SLOT(postStatusError(QTweetNetBase::ErrorCode, QString)));
    statusUpdate->post(message);
}

void MainWindow::postMessageWithImage(const QString& message, const QString& imagePath)
{
    QTweetStatusUpdate *statusUpdate = new QTweetStatusUpdate(m_oauthTwitter, this);
    connect(statusUpdate, SIGNAL(postedStatus(QTweetStatus)), SLOT(postStatusFinished(QTweetStatus)));
    connect(statusUpdate, SIGNAL(error(QTweetNetBase::ErrorCode, QString)), SLOT(postStatusError(QTweetNetBase::ErrorCode, QString)));
    statusUpdate->post(message);
}

void MainWindow::postStatusFinished(const QTweetStatus &status)
{
    QTweetStatusUpdate *statusUpdate = qobject_cast<QTweetStatusUpdate*>(sender());
    if (statusUpdate)
    {
        printf("Posted status with id %llu\n", status.id());
        statusUpdate->deleteLater();
    }
    return quit();
}

void MainWindow::postStatusError(QTweetNetBase::ErrorCode errorCode, QString errorMsg)
{
    return quit();
}
