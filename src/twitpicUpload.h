#ifndef TWITPICUPLOAD_H
#define TWITPICUPLOAD_H

#include <QObject>
#include <QString>
#include "qtweetnetbase.h"

class OAuthTwitter;
class TwitpicUploadStatus;

/**
 *   Uploads to twitpic.
 */
class TwitpicUpload : public QObject
{
	Q_OBJECT

	public:
		TwitpicUpload(const QString& twitpicAppKey, OAuthTwitter *oauthTwitter, QObject *parent = 0);
		void upload(const QString& message, const QString& filename);

	signals:
		void error(QTweetNetBase::ErrorCode code, const QString& errorMsg);
		void jsonParseError(const QByteArray& json);
		void finished(const TwitpicUploadStatus& status);

	protected slots:
		void reply();

	private:
		OAuthTwitter* m_oauthTwitter;
		QString m_twitpicAppKey;
};

#endif // TWITPICUPLOAD_H
