#ifndef PHOTOTWEET_H
#define PHOTOTWEET_H

#include <QObject>
#include "qtweetnetbase.h"
#include "Camera.h"

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

		void twitpicError(QTweetNetBase::ErrorCode errorCode, QString errorMsg);
		void twitpicJsonParseError(const QByteArray& json);
		void twitpicFinished(const TwitpicUploadStatus& status);

		void yfrogError(QTweetNetBase::ErrorCode errorCode, const YfrogUploadStatus& status);
		void yfrogFinished(const YfrogUploadStatus& status);

		void takePictureSuccess(const QString& filePath);
		void takePictureError(Camera::ErrorType errorType, int error);

		void takePhotoAndTweet();

	private:
		void uploadAndTweet(const QString& message, const QString& imagePath = QString());
		void printObject(const QVariant& object);
		void postMessage();
		void postMessageWithImageTwitpic(const QString& imagePath);
		void postMessageWithImageYfrog(const QString& imagePath);
		void showUsage();
		void doQuit();

		OAuthTwitter *m_oauthTwitter;
		QString m_yfrogApiKey;
		QString m_twitpicApiKey;
		QString m_message;

		QTweetConfiguration* m_tweetConfig;
		QTweetStatusUpdate* m_statusUpdate;
		TwitpicUpload* m_twitpic;
		YfrogUpload* m_yfrog;

		bool m_quit;
		bool m_idle;
		Camera* m_camera;
};

#endif // PHOTOTWEET_H
