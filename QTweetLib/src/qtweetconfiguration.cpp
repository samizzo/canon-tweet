#include <QtDebug>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include "qtweetconfiguration.h"

QTweetConfiguration::QTweetConfiguration(QObject *parent) :
    QTweetNetBase(parent)
{
}

QTweetConfiguration::QTweetConfiguration(OAuthTwitter *oauthTwitter, QObject *parent) :
        QTweetNetBase(oauthTwitter, parent)
{
}

void QTweetConfiguration::get()
{
    QUrl url("http://api.twitter.com/1.1/help/configuration.json");

    QNetworkRequest req(url);

    if (isAuthenticationEnabled())
	{
        QByteArray oauthHeader = oauthTwitter()->generateAuthorizationHeader(url, OAuth::GET);
        req.setRawHeader(AUTH_HEADER, oauthHeader);
    }

    QNetworkReply *reply = oauthTwitter()->networkAccessManager()->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(reply()));
}

void QTweetConfiguration::parseJsonFinished(const QJsonDocument &jsonDoc)
{
    if (jsonDoc.isObject())
	{
        QJsonObject respJsonObject = jsonDoc.object();
        emit configuration(jsonDoc);
    }
}

