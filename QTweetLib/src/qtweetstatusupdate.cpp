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
#include <QHttpMultiPart>
#include <QStringList>
#include <QFile>
#include <QUrl>
#include "qtweetstatusupdate.h"
#include "qtweetstatus.h"
#include "qtweetgeocoord.h"
#include "qtweetconvert.h"
#include "json/qjsondocument.h"
#include "json/qjsonobject.h"

QTweetStatusUpdate::QTweetStatusUpdate(QObject *parent) :
    QTweetNetBase(parent)
{
}

QTweetStatusUpdate::QTweetStatusUpdate(OAuthTwitter *oauthTwitter, QObject *parent) :
        QTweetNetBase(oauthTwitter, parent)
{
}

/**
 *   Posts a tweet
 *   @param status text of the status update
 *   @param inReplyToStatus ID of a existing tweet is in reply to
 *   @param latLong latitude and longitude
 *   @param placeid a place in the world (use reverse geocoding)
 *   @param displayCoordinates whether or not to put a exact coordinates a tweet has been sent from
 */
void QTweetStatusUpdate::post(const QString &status,
                              const QString& filename,
                              qint64 inReplyToStatus,
                              const QTweetGeoCoord& latLong,
                              const QString &placeid,
                              bool displayCoordinates,
                              bool trimUser,
                              bool includeEntities)
{
    if (!isAuthenticationEnabled()) {
        qCritical("Needs authentication to be enabled");
        return;
    }

    QString urlString("https://api.twitter.com/1.1/statuses/");
    if (filename.length() == 0)
    {
        urlString += "update.json";
    }
    else
    {
        urlString += "update_with_media.json";
    }

    QByteArray oauthHeader;

    QUrl urlBase = QUrl(urlString);
    QUrl urlQuery(urlString);
    urlQuery.addEncodedQueryItem("status", QUrl::toPercentEncoding(status));

    if (inReplyToStatus != 0)
        urlQuery.addQueryItem("in_reply_to_status_id", QString::number(inReplyToStatus));

    if (latLong.isValid()) {
        urlQuery.addQueryItem("lat", QString::number(latLong.latitude()));
        urlQuery.addQueryItem("long", QString::number(latLong.longitude()));
    }

    if (!placeid.isEmpty())
        urlQuery.addQueryItem("place_id", placeid);

    if (displayCoordinates)
        urlQuery.addQueryItem("display_coordinates", "true");

    if (trimUser)
        urlQuery.addQueryItem("trim_user", "true");

    if (includeEntities)
        urlQuery.addQueryItem("include_entities", "true");

    if (filename.length() == 0)
    {
        // Not uploading a file, so we are using urlencoded args.
        // We therefore need to generate the OAuth signature based on
        // the entire query.
        oauthHeader = oauthTwitter()->generateAuthorizationHeader(urlQuery, OAuth::POST);
    }
    else
    {
        // We are uploading a file, so we generate the OAuth signature
        // based only on the base url.
        oauthHeader = oauthTwitter()->generateAuthorizationHeader(urlBase, OAuth::POST);
    }

    QNetworkRequest req(urlBase);
    req.setRawHeader(AUTH_HEADER, oauthHeader);

    //build status post array
    QByteArray statusPost = urlQuery.toEncoded(QUrl::RemoveScheme | QUrl::RemoveAuthority | QUrl::RemovePath);
    //remove '?'
    statusPost.remove(0, 1);

    QNetworkReply* reply = 0;

    if (filename.length() == 0)
    {
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        reply = oauthTwitter()->networkAccessManager()->post(req, statusPost);
    }
    else
    {
        QHttpMultiPart* mp = new QHttpMultiPart(QHttpMultiPart::FormDataType);

        // Add the image data.
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly))
        {
            QHttpPart imagePart;
            imagePart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
            imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"media[]\"; filename=\"" + filename + "\"");
            imagePart.setBody(file.readAll());
            mp->append(imagePart);
        }

        // Each param is a part of the multipart.
        QList<QByteArray> params = statusPost.split('&');
        for (int i = 0; i < params.count(); i++)
        {
            QString p(params[i]);
            QStringList pairs(p.split('='));

            if (pairs.count() == 2)
            {
                QHttpPart part;
                part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"" + QUrl::fromPercentEncoding(pairs[0].toAscii()) + "\"");
                part.setBody(QUrl::fromPercentEncoding(pairs[1].toAscii()).toAscii());
                mp->append(part);
            }
        }

        reply = oauthTwitter()->networkAccessManager()->post(req, mp);
        mp->setParent(reply);
    }

    connect(reply, SIGNAL(finished()), this, SLOT(reply()));
}

void QTweetStatusUpdate::parseJsonFinished(const QJsonDocument &jsonDoc)
{
    if (jsonDoc.isObject()) {
        QTweetStatus status = QTweetConvert::jsonObjectToStatus(jsonDoc.object());

        emit postedStatus(status);
    }
}

