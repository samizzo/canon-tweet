#include <QHttpMultipart>
#include "yfrogUpload.h"
#include "yfrogUploadStatus.h"

YfrogUpload::YfrogUpload(const QString& yfrogApiKey, OAuthTwitter* oauthTwitter, QObject* parent /*= 0*/) :
QObject(parent),
m_oauthTwitter(oauthTwitter),
m_yfrogApiKey(yfrogApiKey)
{
}

void YfrogUpload::upload(const QString& filename)
{
	QString realm("http://api.twitter.com/");
	QString authProviderUrl("https://api.twitter.com/1/account/verify_credentials.xml");
	QUrl url("http://yfrog.com/api/xauth_upload");

    QHttpMultiPart* mp = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // Add the image data.
    QFile file(filename);
	file.open(QIODevice::ReadOnly);
    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"media\"; filename=\"" + file.fileName() + "\"");
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    imagePart.setBody(file.readAll());
    mp->append(imagePart);

	// key
	QHttpPart keyPart;
    keyPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"key\"");
    keyPart.setBody(m_yfrogApiKey.toLatin1());
    mp->append(keyPart);

	QByteArray oauthHeader = m_oauthTwitter->generateAuthorizationHeader(authProviderUrl, OAuth::GET, realm);
    QNetworkRequest req(url);
	req.setRawHeader("X-Auth-Service-Provider", authProviderUrl.toLatin1());
    req.setRawHeader("X-Verify-Credentials-Authorization", oauthHeader);

	// Post it!
    QNetworkReply* reply = m_oauthTwitter->networkAccessManager()->post(req, mp);
    mp->setParent(reply);
	connect(reply, SIGNAL(finished()), this, SLOT(reply()));
}

void YfrogUpload::reply()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    if (reply)
	{
        QByteArray response = reply->readAll();

        if (reply->error() == QNetworkReply::NoError)
		{
			QDomDocument doc;
			doc.setContent(response);
			YfrogUploadStatus status(doc);
			emit finished(status);
		}
		else
		{
            // HTTP status code
            int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

			QDomDocument doc;
			doc.setContent(response);
			YfrogUploadStatus status(doc);

            switch (httpStatus)
			{
				case QTweetNetBase::NotModified:
				case QTweetNetBase::BadRequest:
				case QTweetNetBase::Unauthorized:
				case QTweetNetBase::Forbidden:
				case QTweetNetBase::NotFound:
				case QTweetNetBase::NotAcceptable:
				case QTweetNetBase::EnhanceYourCalm:
				case QTweetNetBase::InternalServerError:
				case QTweetNetBase::BadGateway:
				case QTweetNetBase::ServiceUnavailable:
				{
					emit error(static_cast<QTweetNetBase::ErrorCode>(httpStatus), status);
					break;
				}

				default:
				{
					emit error(QTweetNetBase::UnknownError, status);
				}
            }
        }

        reply->deleteLater();
    }
}
