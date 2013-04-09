#ifndef YFROGUPLOADSTATUS_H
#define YFROGUPLOADSTATUS_H

#include <QObject>
#include <QString>
#include <QtXml>

/**
 *   Status of a yfrog image upload.
 */
class YfrogUploadStatus : public QObject
{
	Q_OBJECT

	public:
		enum Status
		{
			Ok,
			AuthFailed,
			MediaNotFound,
			UnsupportedMediaType,
			MediaTooBig,
			MissingErrorTag,
			BadXmlResponse,
			MissingXmlResponse,
			UnknownErrorCode,
			UnknownError,
		};

		YfrogUploadStatus(const QDomDocument& xml, int httpStatus = 0, QString httpStatusString = QString());

		const QString& getMediaId() const;
		void setMediaId(const QString& mediaId);

		const QString& getMediaUrl() const;
		void setMediaUrl(const QString& mediaUrl);

		Status getStatus() const;
		void setStatus(Status status);

		QString getStatusString() const;
		int getErrorCode() const;
		int getHttpStatus() const;
		QString getHttpStatusString() const;

	private:
		QString m_mediaId;
		QString m_mediaUrl;
		Status m_status;
		int m_errorCode;
		int m_httpStatus;
		QString m_httpStatusString;
};

inline const QString& YfrogUploadStatus::getMediaId() const
{
	return m_mediaId;
}

inline void YfrogUploadStatus::setMediaId(const QString& mediaId)
{
	m_mediaId = mediaId;
}

inline const QString& YfrogUploadStatus::getMediaUrl() const
{
	return m_mediaUrl;
}

inline void YfrogUploadStatus::setMediaUrl(const QString& mediaUrl)
{
	m_mediaUrl = mediaUrl;
}

inline YfrogUploadStatus::Status YfrogUploadStatus::getStatus() const
{
	return m_status;
}

inline void YfrogUploadStatus::setStatus(Status status)
{
	m_status = status;
}

inline int YfrogUploadStatus::getErrorCode() const
{
	return m_errorCode;
}

inline int YfrogUploadStatus::getHttpStatus() const
{
	return m_httpStatus;
}

inline QString YfrogUploadStatus::getHttpStatusString() const
{
	return m_httpStatusString;
}

inline QString YfrogUploadStatus::getStatusString() const
{
	switch (m_status)
	{
		case YfrogUploadStatus::Ok:
		{
			return "Yfrog says ok";
		}

		case YfrogUploadStatus::AuthFailed:
		{
			return "Yfrog says authorisation with twitter failed";
		}

		case YfrogUploadStatus::MediaNotFound:
		{
			return "Yfrog says media not found";
		}

		case YfrogUploadStatus::UnsupportedMediaType:
		{
			return "Yfrog says unsupported media type";
		}

		case YfrogUploadStatus::MediaTooBig:
		{
			return "Yfrog says media too big";
		}

		case YfrogUploadStatus::MissingErrorTag:
		{
			return "Missing error tag in response from Yfrog";
		}

		case YfrogUploadStatus::BadXmlResponse:
		{
			return "Bad XML response from Yfrog";
		}

		case YfrogUploadStatus::MissingXmlResponse:
		{
			return "Missing XML response from Yfrog";
		}

		case YfrogUploadStatus::UnknownErrorCode:
		{
			return "Unknown error code from Yfrog";
		}

		default:
		{
			return "Unknown error";
		}
	}
}

#endif // YFROGUPLOADSTATUS_H
