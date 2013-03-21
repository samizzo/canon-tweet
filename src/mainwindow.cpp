#include <QNetworkAccessManager>
#include <QApplication>
#include <QStringList>
#include "mainwindow.h"
#include "oauthtwitter.h"
#include "qtweetstatusupdate.h"
#include "qtweetstatus.h"

MainWindow::MainWindow()
{
    m_authorized = false;

    m_oauthTwitter = new OAuthTwitter(this);
    m_oauthTwitter->setNetworkAccessManager(new QNetworkAccessManager(this));
}

void MainWindow::showUsage()
{
    printf("photoTweet.exe -message <message text> [-image <image path>]\n");
    printf("tweets <message text> to the account, optionally attaching the");
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
                printf("Missing argument to -message!");
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
                printf("Missing argument to -imagePath!");
                error = true;
                break;
            }
        }
    }

    if (message.length() == 0 || error)
    {
        showUsage();
        return quit();
    }

    // TODO: Some other form of auth?

    printf("tweeting message: '%s'", message.toAscii().constData());
    if (imagePath.length() > 0)
    {
        printf("attaching image: '%s'", imagePath.toAscii().constData());
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
        printf("Posted status with id %llu", status.id());
        statusUpdate->deleteLater();
    }
    return quit();
}

void MainWindow::postStatusError(QTweetNetBase::ErrorCode errorCode, QString errorMsg)
{
    return quit();
}
