/* Copyright 2010 Antonie Jovanoski
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Contact e-mail: Antonie Jovanoski <minimoog77_at_gmail.com>
 */

#include <QtDebug>
#include <QNetworkRequest>
#include <QNetworkReply>
#include "qtweetconfiguration.h"
#include "json/qjsondocument.h"
#include "json/qjsonobject.h"

/**
 *  Constructor
 */
QTweetConfiguration::QTweetConfiguration(QObject *parent) :
    QTweetNetBase(parent)
{
}

/**
 *  Constructor
 *  @param oauthTwitter OAuthTwitter object
 *  @param parent parent QObject
 */
QTweetConfiguration::QTweetConfiguration(OAuthTwitter *oauthTwitter, QObject *parent) :
        QTweetNetBase(oauthTwitter, parent)
{
}

/**
 *  Starts retrieving twitter configuration
 *  @remarks Emits configuration after finishing
 */
void QTweetConfiguration::get()
{
    QUrl url("http://api.twitter.com/1.1/help/configuration.json");

    QNetworkRequest req(url);

    if (isAuthenticationEnabled()) {
        QByteArray oauthHeader = oauthTwitter()->generateAuthorizationHeader(url, OAuth::GET);
        req.setRawHeader(AUTH_HEADER, oauthHeader);
    }

    QNetworkReply *reply = oauthTwitter()->networkAccessManager()->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(reply()));
}

void QTweetConfiguration::parseJsonFinished(const QJsonDocument &jsonDoc)
{
    if (jsonDoc.isObject()) {
        QJsonObject respJsonObject = jsonDoc.object();

        emit configuration(jsonDoc);
    }
}

