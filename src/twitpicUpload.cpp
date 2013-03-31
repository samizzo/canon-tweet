#include <QtDebug>
#include <QFile>
#include <QNetworkRequest>
#include <QHttpMultipart>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "twitpicUpload.h"
#include "oauthtwitter.h"
#include "twitpicUploadStatus.h"

TwitpicUpload::TwitpicUpload(const QString& twitpicAppKey, OAuthTwitter* oauthTwitter, QObject* parent /*= 0*/) :
QObject(parent),
m_oauthTwitter(oauthTwitter),
m_twitpicAppKey(twitpicAppKey)
{
}

void TwitpicUpload::upload(const QString& filename)
{
	QString realm("http://api.twitter.com/");
	QString authProviderUrl("https://api.twitter.com/1.1/account/verify_credentials.json");
	QUrl url("http://api.twitpic.com/2/upload.json");

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
    keyPart.setBody(m_twitpicAppKey.toLatin1());
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

void TwitpicUpload::reply()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    if (reply)
	{
        QByteArray response = reply->readAll();

        if (reply->error() == QNetworkReply::NoError)
		{
			//QString limit = reply->rawHeader("X-RateLimit-Limit");
			//QString remaining = reply->rawHeader("X-RateLimit-Remaining");

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
            /*qDebug() << "Network error: " << reply->error();
            qDebug() << "Error string: " << reply->errorString();
            qDebug() << "Error response: " << response;*/

            // HTTP status code
            int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            /*
				"{
					"errors": [ {"code" : 401, "message" : "Could not authenticate you (header rejected by twitter)." } ]
				}"
			*/

			QString msg;
			QJsonDocument doc = QJsonDocument::fromJson(response);
			if (doc.isObject() && !doc.isNull())
			{
				QJsonValue errorsVal = doc.object().value("errors");
				if (!errorsVal.isNull() && errorsVal.isArray())
				{
					QJsonArray errors = errorsVal.toArray();
					if (errors.count() > 0)
					{
						QJsonValue error = errors.at(0);
						if (!error.isNull() && error.isObject())
						{
							QJsonObject obj = error.toObject();
							msg = obj.value("message").toString();
						}
					}
				}
			}

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
					emit error(static_cast<QTweetNetBase::ErrorCode>(httpStatus), msg);
					break;
				}

				default:
				{
					emit error(QTweetNetBase::UnknownError, msg);
				}
            }
        }

        reply->deleteLater();
    }
}
