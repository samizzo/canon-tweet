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

#ifndef QTWEETCONFIGURATION_H
#define QTWEETCONFIGURATION_H

#include "qtweetnetbase.h"

/**
 *   Returns the current twitter configuration settings.
 */
class QTWEETLIBSHARED_EXPORT QTweetConfiguration : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetConfiguration(QObject *parent = 0);
    QTweetConfiguration(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void get();

signals:
    /** Emits configuration info
     *  @param config Json configuration document
     */
    void configuration(const QJsonDocument &jsonDoc);

protected slots:
    void parseJsonFinished(const QJsonDocument &jsonDoc);
};

#endif // QTWEETCONFIGURATION_
