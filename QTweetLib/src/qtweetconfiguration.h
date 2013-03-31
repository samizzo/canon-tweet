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

#endif // QTWEETCONFIGURATION_H
