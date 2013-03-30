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

#ifndef QTWEETCONVERT_H
#define QTWEETCONVERT_H

#include <QList>

class QTweetStatus;
class QTweetUser;
class QTweetDMStatus;
class QTweetPlace;
class QTweetEntityUrl;
class QTweetEntityHashtag;
class QTweetEntityUserMentions;
class QTweetEntityMedia;

class QJsonArray;
class QJsonObject;

/**
 *  Contains static converting functions
 */
class QTweetConvert
{
public:
    static QList<QTweetStatus> jsonArrayToStatusList(const QJsonArray& jsonArray);
    static QTweetStatus jsonObjectToStatus(const QJsonObject& jsonObject);
    static QTweetUser jsonObjectToUser(const QJsonObject& jsonObject);
    static QList<QTweetUser> jsonArrayToUserInfoList(const QJsonArray& jsonArray);
    static QTweetPlace jsonObjectToPlace(const QJsonObject& var);
    static QTweetPlace jsonObjectToPlaceRecursive(const QJsonObject& jsonObject);
    static QList<QTweetPlace> jsonObjectToPlaceList(const QJsonObject& jsonObject);
    static QTweetEntityUrl jsonObjectToEntityUrl(const QJsonObject& jsonObject);
    static QTweetEntityHashtag jsonObjectToEntityHashtag(const QJsonObject &jsonObject);
    static QTweetEntityUserMentions jsonObjectToEntityUserMentions(const QJsonObject& jsonObject);
    static QTweetEntityMedia jsonObjectToEntityMedia(const QJsonObject& jsonObject);
};

#endif // QTWEETCONVERT_H
