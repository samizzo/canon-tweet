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

		const QString& getText() const;
		void setText(const QString& text);

		const QString& getImageUrl() const;
		void setImageUrl(const QString& imageUrl);

		int getWidth() const;
		void setWidth(int width);

		int getHeight() const;
		void setHeight(int height);

	private:
		int m_width;
		int m_height;
		QString m_text;
		QString m_imageUrl;
};

inline const QString& TwitpicUploadStatus::getText() const
{
	return m_text;
}

inline void TwitpicUploadStatus::setText(const QString& text)
{
	m_text = text;
}

inline const QString& TwitpicUploadStatus::getImageUrl() const
{
	return m_imageUrl;
}

inline void TwitpicUploadStatus::setImageUrl(const QString& imageUrl)
{
	m_imageUrl = imageUrl;
}

inline int TwitpicUploadStatus::getWidth() const
{
	return m_width;
}

inline void TwitpicUploadStatus::setWidth(int width)
{
	m_width = width;
}

inline int TwitpicUploadStatus::getHeight() const
{
	return m_height;
}

inline void TwitpicUploadStatus::setHeight(int height)
{
	m_height = height;
}

#endif // TWITPICUPLOADSTATUS_H
