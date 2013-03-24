#ifndef PHOTOTWEET_H
#define PHOTOTWEET_H

#include <QObject>
#include "qtweetnetbase.h"

class OAuthTwitter;
class QTweetStatus;

class PhotoTweet : public QObject
{
    Q_OBJECT

public:
    explicit PhotoTweet();

signals:
    void quit();

public slots:
    void run(QString& message, QString& imagePath);
    void main();

private slots:
    void getConfigurationFinished(const QJsonDocument& json);
    void postStatusFinished(const QTweetStatus& status);
    void postStatusError(QTweetNetBase::ErrorCode errorCode, QString errorMsg);
    void replyFinished(const QByteArray& response, const QNetworkReply& reply);

private:
    void printObject(const QVariant& object);
    void getConfiguration();
    void postMessage();
    void postMessageWithImage();
    void showUsage();
    void doQuit();

    OAuthTwitter *m_oauthTwitter;

    QString m_message;
    QString m_imagePath;
    bool m_doPost;
    int m_photoSizeLimit;
};

#endif // PHOTOTWEET_H
