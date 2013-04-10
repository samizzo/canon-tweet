#ifndef PHOTOTWEET_H
#define PHOTOTWEET_H

#include <QObject>
#include <QTime>
#include "qtweetnetbase.h"
#include "Camera.h"

class OAuthTwitter;
class QTweetStatus;
class YfrogUploadStatus;
class QTweetStatusUpdate;
class YfrogUpload;
class Camera;
class Config;

class PhotoTweet : public QObject
{
	Q_OBJECT

	public:
		PhotoTweet();
		~PhotoTweet();

	signals:
		void quit();

	public slots:
		void main();

	private slots:
		void postStatusFinished(const QTweetStatus& status);
		void postStatusError(QTweetNetBase::ErrorCode errorCode, QString errorMsg);

		void yfrogError(QTweetNetBase::ErrorCode errorCode, const YfrogUploadStatus& status);
		void yfrogFinished(const YfrogUploadStatus& status);

		void takePictureSuccess(const QString& filePath);
		void takePictureError(Camera::ErrorType errorType, int error);

		void takePhotoAndTweet();

	private:
		void uploadAndTweet(const QString& message, const QString& imagePath = QString());
		void postMessage();
		void showUsage();
		void doQuit();

		// Scales imagePath and returns a path to the newly scaled image.
		QString scaleImage(const QString& imagePath);

		OAuthTwitter *m_oauthTwitter;
		QString m_message;

		QTweetStatusUpdate* m_statusUpdate;
		YfrogUpload* m_yfrog;

		bool m_quit;
		bool m_idle;
		Camera* m_camera;
		Config* m_config;
		QTime m_uploadStartTime;
};

#endif // PHOTOTWEET_H
