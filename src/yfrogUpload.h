#ifndef YFROGUPLOAD_H
#define YFROGUPLOAD_H

#include <QObject>
#include <QString>
#include "oauth.h"
#include "oauthtwitter.h"
#include "yfrogUploadStatus.h"
#include "qtweetnetbase.h"

class OAuthTwitter;

/**
 *   Uploads to yfrog.
 */
class YfrogUpload : public QObject
{
	Q_OBJECT

	public:
		YfrogUpload(const QString& yfrogApiKey, OAuthTwitter *oauthTwitter, QObject *parent = 0);
		void upload(const QString& message, const QString& filename);

	signals:
		void error(QTweetNetBase::ErrorCode code, const YfrogUploadStatus& status);
		void finished(const YfrogUploadStatus& status);

	protected slots:
		void reply();

	private:
		OAuthTwitter* m_oauthTwitter;
		QString m_yfrogApiKey;
};

#endif // YFROGUPLOAD_H
