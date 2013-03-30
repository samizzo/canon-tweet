#include <QJsonObject>
#include "twitpicUploadStatus.h"

TwitpicUploadStatus::TwitpicUploadStatus(const QJsonObject& jsonObject) :
m_width(0),
m_height(0)
{
	QJsonValue width = jsonObject.value("width");
	if (!width.isUndefined())
	{
		int val = (int)width.toDouble();
		if (width.isString())
		{
			QString s = width.toString();
			val = s.toInt();
		}

		setWidth(val);
	}

	QJsonValue height = jsonObject.value("height");
	if (!height.isUndefined())
	{
		int val = (int)height.toDouble();
		if (height.isString())
		{
			QString s = height.toString();
			val = s.toInt();
		}

		setHeight(val);
	}

	QJsonValue text = jsonObject.value("text");
	if (!text.isUndefined())
	{
		setText(text.toString());
	}

	QJsonValue url = jsonObject.value("url");
	if (!text.isUndefined())
	{
		setImageUrl(url.toString());
	}
}

