#include <QNetworkAccessManager>
#include <QtWidgets/QApplication>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMultiHash>
#include <QImage>
#include <QFileInfo>
#include "phototweet.h"
#include "oauthtwitter.h"
#include "qtweetstatusupdate.h"
#include "qtweetstatus.h"
#include "yfrogUpload.h"
#include "yfrogUploadStatus.h"
#include "Config.h"

PhotoTweet::PhotoTweet() :
m_quit(true),
m_idle(true)
{
    m_oauthTwitter = new OAuthTwitter(this);
    m_oauthTwitter->setNetworkAccessManager(new QNetworkAccessManager(this));

	m_config = new Config("phototweet.cfg");

	QString val = m_config->GetValue("consumer_key");
	m_oauthTwitter->setConsumerKey(val.toLatin1());

	val = m_config->GetValue("consumer_secret");
	m_oauthTwitter->setConsumerSecret(val.toLatin1());

	val = m_config->GetValue("oauth_token");
	m_oauthTwitter->setOAuthToken(val.toLatin1());

	val = m_config->GetValue("oauth_token_secret");
	m_oauthTwitter->setOAuthTokenSecret(val.toLatin1());

	QString yfrogApiKey = m_config->GetValue("yfrog_api_key");

	// Setup the camera system.
	QString imageDir = QCoreApplication::applicationDirPath() + "/images";
	m_camera = new Camera(imageDir);

	connect(m_camera, SIGNAL(OnTakePictureSuccess(QString)), SLOT(takePictureSuccess(QString)));
	connect(m_camera, SIGNAL(OnTakePictureError(Camera::ErrorType, int)), SLOT(takePictureError(Camera::ErrorType, int)));

	int error = m_camera->Startup();
	if (error != EDS_ERR_OK)
	{
		qCritical("Couldn't start the camera system!");
		qCritical("%s", Camera::GetErrorMessage(Camera::ErrorType_Normal, error).toLatin1().constData());
	}

	// Setup status update access.
	m_statusUpdate = new QTweetStatusUpdate(m_oauthTwitter, this);
    connect(m_statusUpdate, SIGNAL(postedStatus(QTweetStatus)), SLOT(postStatusFinished(QTweetStatus)));
    connect(m_statusUpdate, SIGNAL(error(QTweetNetBase::ErrorCode, QString)), SLOT(postStatusError(QTweetNetBase::ErrorCode, QString)));

	// Setup yfrog uploads.
	m_yfrog = new YfrogUpload(yfrogApiKey, m_oauthTwitter, this);
    connect(m_yfrog, SIGNAL(error(QTweetNetBase::ErrorCode, YfrogUploadStatus)), SLOT(yfrogError(QTweetNetBase::ErrorCode, YfrogUploadStatus)));
    connect(m_yfrog, SIGNAL(finished(YfrogUploadStatus)), SLOT(yfrogFinished(YfrogUploadStatus)));
}

PhotoTweet::~PhotoTweet()
{
	m_camera->Shutdown();
}

void PhotoTweet::main()
{
    QStringList& args = QApplication::arguments();
    bool error = false, continuous = false;
	float shotTime = 2.0f;

    QString message, imagePath;

    for (int i = 0; i < args.count(); i++)
    {
        if (!args.at(i).compare("-message"))
        {
            if (i + 1 < args.count())
            {
                message = args.at(i + 1);
            }
            else
            {
                printf("Missing argument to -message!\n");
                error = true;
                break;
            }
        }
        else if (!args.at(i).compare("-image"))
        {
            if (i + 1 < args.count())
            {
                imagePath = args.at(i + 1);
                if (!QFile::exists(imagePath))
                {
                    printf("Couldn't find file %s!\n", imagePath.toLatin1().constData());
                    error = true;
                }
            }
            else
            {
                printf("Missing argument to -image!\n");
                error = true;
                break;
            }
        }
		else if (!args.at(i).compare("-continuous"))
		{
			if (i + 1 < args.count())
			{
				shotTime = args.at(i + 1).toFloat();
				continuous = true;
				m_quit = false;
			}
			else
			{
				printf("Missing argument to -continuous!\n");
				error = true;
				break;
			}
		}
		else if (!args.at(i).compare("-takePhotoAndTweet"))
		{
			return takePhotoAndTweet();
		}
    }

	if (continuous)
	{
		// Run in continuous mode, taking a picture and uploading every xx seconds.
		QTimer* timer = new QTimer(this);
		connect(timer, SIGNAL(timeout()), this, SLOT(takePhotoAndTweet()));
		timer->start(shotTime * 1000);
		return;
	}

    if (message.length() == 0)
    {
        showUsage();
        return doQuit();
    }

    if (error)
    {
        return doQuit();
    }

    uploadAndTweet(message, imagePath);
}

void PhotoTweet::showUsage()
{
    printf("photoTweet.exe [args]\n\n");
    printf("where args are:\n\n");
    printf("-image <image path>         upload specified image\n");
    printf("-message <message text>     tweet specified status text\n");
	printf("-continuous <time>          take photo and tweet every <time> seconds\n");
	printf("-takePhotoAndTweet          take a photo and tweet it\n");
    printf("\n");
    printf("Note: a message must always be specified if tweeting.  Use quotes\n");
    printf("around the message text to specify multiple words.\n");
}

void PhotoTweet::takePhotoAndTweet()
{
	m_message = m_config->GetValue("message");

	// A photo will only be taken if we aren't currently taking a photo
	// and/or uploading and tweeting.
	if (m_idle)
	{
		m_idle = false;
		qDebug("Taking a photo..");
		m_camera->TakePicture();
	}
	else
	{
		qDebug("Photo in progress..");
	}
}

QString PhotoTweet::scaleImage(const QString& imagePath)
{
	QImage image(imagePath);
	if (image.isNull())
	{
		return imagePath;
	}

	int quality = m_config->GetValue("jpeg_quality", "-1").toInt();
	int width = m_config->GetValue("scaled_width", "1024").toInt();
	if ((quality < 0 && quality != -1) || quality > 100)
	{
		// -1 by default if a bogus value has been specified.
		quality = -1;
	}

	// Clamp width to between 128 and 4096.
	width = min(max(128, width), 4096);

	QTime timer;
	timer.start();

	QImage scaledImage = image.scaledToWidth(width, Qt::SmoothTransformation);
	QFileInfo fi(imagePath);
	QString newName = fi.path() + "/" + fi.baseName() + "_small." + fi.suffix();
	scaledImage.save(newName, 0, quality);

	int elapsed = timer.elapsed();
	qDebug("Image scaling took %.2f seconds", elapsed / 1000.0f);

	return newName;
}

void PhotoTweet::takePictureSuccess(const QString& filePath)
{
	// Picture was taken successfully, so first scale it, then upload and tweet.
	QString scaledImage = scaleImage(filePath);
	uploadAndTweet(m_message, scaledImage);
}

void PhotoTweet::takePictureError(Camera::ErrorType errorType, int error)
{
	qCritical("Couldn't take a picture!");
	qCritical("%s", Camera::GetErrorMessage(errorType, error).toLatin1().constData());
	m_idle = true;
}

void PhotoTweet::uploadAndTweet(const QString& message, const QString& imagePath)
{
	m_uploadStartTime.start();
    m_message = message;

	if (m_message.length() > 0)
	{
		qDebug("Tweeting message: '%s'", m_message.toLatin1().constData());
	}

    if (imagePath.length() > 0 && QFile::exists(imagePath))
    {
		// Upload the image.  The message will be tweeted after the upload
		// completes.
		qDebug("Uploading image: '%s'", imagePath.toLatin1().constData());
		m_yfrog->upload(imagePath);
    }
    else
    {
		if (imagePath.length() > 0 && !QFile::exists(imagePath))
		{
			qWarning("Image '%s' upload was requested but the image couldn't be found!",
				imagePath.toLatin1().constData());
		}

		// No image specified so just tweet a message.
        postMessage();
    }
}

void PhotoTweet::doQuit()
{
    emit quit();
}

void PhotoTweet::postMessage()
{
	m_statusUpdate->post(m_message);
}

void PhotoTweet::postStatusFinished(const QTweetStatus &status)
{
	qDebug("Posted status with id %llu", status.id());

	if (m_quit)
	{
		doQuit();
	}

	int elapsed = m_uploadStartTime.elapsed();
	qDebug("Uploads took %.2f seconds", elapsed / 1000.0f);

	m_idle = true;
}

void PhotoTweet::postStatusError(QTweetNetBase::ErrorCode, QString errorMsg)
{
    if (errorMsg.length() > 0)
    {
        qWarning("Error posting message: %s", errorMsg.toLatin1().constData());
    }

	if (m_quit)
	{
		doQuit();
	}

	m_idle = true;
}

void PhotoTweet::yfrogError(QTweetNetBase::ErrorCode code, const YfrogUploadStatus& status)
{
	qWarning("Error posting image to yfrog: %s (%i)", status.getHttpStatusString(), code);

	if (m_quit)
	{
		doQuit();
	}

	m_idle = true;
}

void PhotoTweet::yfrogFinished(const YfrogUploadStatus& status)
{
	if (status.getStatus() != YfrogUploadStatus::Ok)
	{
		qWarning("Error posting image to yfrog: %s (%i)", status.getStatusString().toLatin1().constData(), status.getErrorCode());

		if (m_quit)
		{
			doQuit();
		}

		m_idle = true;
	}
	else
	{
		// Image was posted successfully, so now tweet the url.
		qDebug("Posted image to yfrog!");
		qDebug("Url is %s", status.getMediaUrl().toLatin1().constData());

		// Prepend message to url.
		if (m_message.length() > 0)
		{
			m_message += " ";
		}

		m_message += status.getMediaUrl();

		// Append hashtags.
		QString hashtags = m_config->GetValue("hashtags");
		if (hashtags.length() > 0)
		{
			m_message = m_message + " " + hashtags;
		}

		qDebug("Posting link to twitter..");
		postMessage();
	}
}
