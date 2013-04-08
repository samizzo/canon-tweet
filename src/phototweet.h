#ifndef PHOTOTWEET_H
#define PHOTOTWEET_H

#include <QObject>
#include "qtweetnetbase.h"

class TwitpicUploadStatus;
class OAuthTwitter;
class QTweetStatus;
class YfrogUploadStatus;
class QTweetConfiguration;
class QTweetStatusUpdate;
class TwitpicUpload;
class YfrogUpload;
class Camera;

class PhotoTweet : public QObject
{
	Q_OBJECT

	public:
		PhotoTweet();
		~PhotoTweet();

		bool loadConfig();

	signals:
		void quit();

	public slots:
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

		void takePhoto();

	private:
		void uploadAndTweet(QString& message, QString& imagePath = QString());
		void printObject(const QVariant& object);
		void getConfiguration();
		void postMessage();
		void postMessageWithImageTwitpic();
		void postMessageWithImageYfrog();
		void showUsage();
		void doQuit();
		void takePhotoAndTweet();

		OAuthTwitter *m_oauthTwitter;
		QString m_yfrogApiKey;
		QString m_twitpicApiKey;
		QString m_message;
		QString m_imagePath;

		QTweetConfiguration* m_tweetConfig;
		QTweetStatusUpdate* m_statusUpdate;
		TwitpicUpload* m_twitpic;
		YfrogUpload* m_yfrog;

		bool m_processing;
		bool m_quit;

		Camera* m_camera;
};

#endif // PHOTOTWEET_H
