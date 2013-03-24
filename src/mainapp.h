#ifndef MAINAPP_H
#define MAINAPP_H

#include <QObject>
#include "qtweetnetbase.h"

class OAuthTwitter;
class QTweetStatus;

class MainApp : public QObject
{
    Q_OBJECT

public:
    explicit MainApp();

signals:
    void quit();

public slots:
    void run();

private slots:
    void getConfigurationFinished(const QJsonDocument& json);
    void postStatusFinished(const QTweetStatus& status);
    void postStatusError(QTweetNetBase::ErrorCode errorCode, QString errorMsg);
    void replyFinished(const QByteArray& response, const QNetworkReply& reply);

private:
    void printObject(const QVariant& object);
    void getConfiguration();
    void postMessage(const QString& message);
    void postMessageWithImage(const QString& message, const QString& imagePath);
    void showUsage();
    void doQuit();

    OAuthTwitter *m_oauthTwitter;
};

#endif // MAINAPP_H
