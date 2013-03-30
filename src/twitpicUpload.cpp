#include <QtDebug>
#include <QFile>
#include <QNetworkRequest>
#include <QHttpMultipart>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include "twitpicUpload.h"
#include "oauthtwitter.h"
#include "twitpicUploadStatus.h"

TwitpicUpload::TwitpicUpload(const QString& twitpicAppKey, OAuthTwitter* oauthTwitter, QObject* parent /*= 0*/) :
QObject(parent),
m_oauthTwitter(oauthTwitter),
m_twitpicAppKey(twitpicAppKey)
{
}

void TwitpicUpload::upload(const QString& message, const QString& filename)
{
	if (!QFile::exists(filename))
	{
		return;
	}

	QString authProviderUrl("https://api.twitter.com/1.1/account/verify_credentials.json");
	QUrl url("http://api.twitpic.com/2/upload.json");

	/*
		Custom headers:
		X-Auth-Service-Provider: https://api.twitter.com/1.1/account/verify_credentials.json
		X-Verify-Credentials-Authorization: OAuth realm="http://api.twitter.com/", oauth_consumer_key="8Y0v3tSsxiTVE8EPK93bKg", oauth_signature_method="HMAC-SHA1", oauth_token="1264390094-1umU5vWcNDlFobbDAlwdJu9aRa7cW7xPGubE7wa", oauth_timestamp="1364619957", oauth_nonce="6293246d4863adde18694c1366c61e46", oauth_version="1.0", oauth_signature="6V5ik801sMdM5yN2bNEb%2FUTciJw%3D"

		Multipart query params:
		key=597e4f4eb3aa39e68c6493ed683c7145
		media=@public_html/flui.png
		message="" (can be empty)

		url: http://api.twitpic.com/2/upload.json
	*/

    QHttpMultiPart* mp = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // Add the image data.
    QFile file(filename);
    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"media[]\"; filename=\"" + filename + "\"");
    imagePart.setBody(file.readAll());
    mp->append(imagePart);

	// key
	QHttpPart keyPart;
    keyPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"key\"");
    keyPart.setBody(m_twitpicAppKey.toLatin1());
    mp->append(keyPart);

	// message
	QHttpPart msgPart;
	msgPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"message\"");
    msgPart.setBody(message.toLatin1());
	mp->append(msgPart);

	QByteArray oauthHeader = m_oauthTwitter->generateAuthorizationHeader(url, OAuth::POST);
    QNetworkRequest req(url);
	req.setRawHeader("X-Auth-Service-Provider", authProviderUrl.toLatin1());
    req.setRawHeader("X-Verify-Credentials-Authorization", oauthHeader);

	// Post it!
    QNetworkReply* reply = m_oauthTwitter->networkAccessManager()->post(req, mp);
    mp->setParent(reply);
	connect(reply, SIGNAL(finished()), this, SLOT(reply()));
}

void TwitpicUpload::reply()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    if (reply)
	{
        QByteArray response = reply->readAll();

        if (reply->error() == QNetworkReply::NoError)
		{
			QJsonParseError jsonError;
			QJsonDocument json = QJsonDocument::fromJson(response, &jsonError);
			if (jsonError.error == QJsonParseError::NoError && json.isObject() && !json.isNull())
			{
				QJsonObject object = json.object();
				TwitpicUploadStatus status(object);
				emit finished(status);
			}
			else
			{
				emit jsonParseError(response);
			}
		}
		else
		{
            qDebug() << "Network error: " << reply->error();
            qDebug() << "Error string: " << reply->errorString();
            qDebug() << "Error response: " << response;

            // HTTP status code
            int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            // TODO: Parse json response

			QString errorMsg;

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
					emit error(static_cast<QTweetNetBase::ErrorCode>(httpStatus), errorMsg);
					break;
				}

				default:
				{
					emit error(QTweetNetBase::UnknownError, errorMsg);
				}
            }
        }

        reply->deleteLater();
    }
}
