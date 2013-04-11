#include "Config.h"
#include <QFile>
#include <QTextStream>
#include "../QsLog/QsLog.h"

Config::Config(const QString& file)
{
	QFile f(file);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QLOG_ERROR() << "Couldn't load config file " << file;
        return;
    }

    QTextStream reader(&f);
    while (!reader.atEnd())
    {
        QString line = reader.readLine().trimmed();
        if (line.length() > 0 && line.at(0) == '[' && line.endsWith(']'))
        {
            QString key = line.mid(1, line.length() - 2);
            QString val = reader.readLine().trimmed();
            if (val.length() > 0 && key.length() > 0)
            {
				SetValue(key, val);
            }
        }
    }
}

QString Config::GetValue(const QString& key, const QString& default /*= QString()*/)
{
	if (m_settings.contains(key))
	{
		return m_settings[key];
	}

	return default;
}

void Config::SetValue(const QString& key, const QString& value)
{
	m_settings[key] = value;
}

