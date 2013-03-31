#ifndef PHOTOTWEET_H
#define PHOTOTWEET_H

#include <QObject>
#include "qtweetnetbase.h"

class TwitpicUploadStatus;
class OAuthTwitter;
class QTweetStatus;
class YfrogUploadStatus;

class PhotoTweet : public QObject
{
    Q_OBJECT

public:
    explicit PhotoTweet();

    bool loadConfig();

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

	void twitpicError(QTweetNetBase::ErrorCode errorCode, QString errorMsg);
	void twitpicJsonParseError(const QByteArray& json);
	void twitpicFinished(const TwitpicUploadStatus& status);

	void yfrogError(QTweetNetBase::ErrorCode errorCode, const YfrogUploadStatus& status);
	void yfrogFinished(const YfrogUploadStatus& status);

private:
    void printObject(const QVariant& object);
    void getConfiguration();
    void postMessage();
	void postMessageWithImageTwitpic();
	void postMessageWithImageYfrog();
    void showUsage();
    void doQuit();

    OAuthTwitter *m_oauthTwitter;

	QString m_yfrogApiKey;
	QString m_twitpicApiKey;
    QString m_message;
    QString m_imagePath;
    bool m_doPost;
    int m_photoSizeLimit;
};

#endif // PHOTOTWEET_H
