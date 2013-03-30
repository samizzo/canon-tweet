#ifndef TWITPICUPLOADSTATUS_H
#define TWITPICUPLOADSTATUS_H

#include <QObject>
#include <QString>
#include <QJsonObject>

/**
 *   Status of a twitpic image upload.
 */
class TwitpicUploadStatus : public QObject
{
	Q_OBJECT

	public:
		TwitpicUploadStatus(const QJsonObject& jsonObject);

		QString getText();
		void setText(const QString& text);

		QString getImageUrl();
		void setImageUrl(const QString& imageUrl);

		int getWidth();
		void setWidth(int width);

		int getHeight();
		void setHeight(int height);

	private:
		int m_width;
		int m_height;
		QString m_text;
		QString m_imageUrl;
};

inline QString TwitpicUploadStatus::getText()
{
	return m_text;
}

inline void TwitpicUploadStatus::setText(const QString& text)
{
	m_text = text;
}

inline QString TwitpicUploadStatus::getImageUrl()
{
	return m_imageUrl;
}

inline void TwitpicUploadStatus::setImageUrl(const QString& imageUrl)
{
	m_imageUrl = imageUrl;
}

inline int TwitpicUploadStatus::getWidth()
{
	return m_width;
}

inline void TwitpicUploadStatus::setWidth(int width)
{
	m_width = width;
}

inline int TwitpicUploadStatus::getHeight()
{
	return m_height;
}

inline void TwitpicUploadStatus::setHeight(int height)
{
	m_height = height;
}

#endif // TWITPICUPLOADSTATUS_H
