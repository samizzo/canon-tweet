#include <QJsonObject>
#include "twitpicUploadStatus.h"

TwitpicUploadStatus::TwitpicUploadStatus(const QJsonObject& jsonObject) :
m_width(0),
m_height(0)
{
	QJsonValue width = jsonObject.value("width");
	if (!width.isUndefined())
	{
		setWidth(width.toDouble());
	}
}

