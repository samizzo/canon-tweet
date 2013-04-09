#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <QHash>

class Config : public QObject
{
	Q_OBJECT

	public:
		Config(const QString& file);

		QString GetValue(const QString& key, const QString& default = QString());
		void SetValue(const QString& key, const QString& value);

	private:
		QHash<QString, QString> m_settings;
};

#endif // CONFIG_H
