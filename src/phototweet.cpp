#include <QNetworkAccessManager>
#include <QtWidgets/QApplication>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMultiHash>
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

void PhotoTweet::showUsage()
{
    printf("photoTweet.exe [args]\n\n");
    printf("where args are:\n\n");
    printf("-image <image path>         upload specified image\n");
    printf("-message <message text>     tweet specified status text\n");
    printf("-getconfig                  return media configuration settings\n");
	printf("-continuous <time>          take photo and tweet every <time> seconds\n");
	printf("-takePhotoAndTweet          take a photo and tweet it\n");
    printf("\n");
    printf("Note: a message must always be specified if tweeting.  Use quotes\n");
    printf("around the message text to specify multiple words.\n");
}

void PhotoTweet::takePhotoAndTweet()
{
	m_message = m_config->GetValue("message");

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

void PhotoTweet::takePictureSuccess(const QString& filePath)
{
	uploadAndTweet(m_message, filePath);
}

void PhotoTweet::takePictureError(Camera::ErrorType errorType, int error)
{
	qCritical("Couldn't take a picture!");
	qCritical("%s", Camera::GetErrorMessage(errorType, error).toLatin1().constData());
	m_idle = true;
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

void PhotoTweet::uploadAndTweet(const QString& message, const QString& imagePath)
{
    m_message = message;

	if (m_message.length() > 0)
	{
		qDebug("Tweeting message: '%s'", m_message.toLatin1().constData());
	}

    if (imagePath.length() > 0)
    {
        qDebug("Uploading image: '%s'", imagePath.toLatin1().constData());
		postMessageWithImageYfrog(imagePath);
    }
    else
    {
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

void PhotoTweet::postMessageWithImageYfrog(const QString& imagePath)
{
	if (QFile::exists(imagePath))
	{
		m_yfrog->upload(imagePath);
	}
	else
	{
		qWarning("Couldn't find file %s", imagePath.toLatin1().constData());
	}
}

void PhotoTweet::postStatusFinished(const QTweetStatus &status)
{
	qDebug("Posted status with id %llu", status.id());

	if (m_quit)
	{
		doQuit();
	}

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

	m_idle = true;

	if (m_quit)
	{
		doQuit();
	}
}

void PhotoTweet::yfrogFinished(const YfrogUploadStatus& status)
{
	if (status.getStatus() != YfrogUploadStatus::Ok)
	{
		qWarning("Error posting image to yfrog: %s", status.getStatusString().toLatin1().constData());

		if (m_quit)
		{
			doQuit();
		}

		m_idle = true;
	}
	else
	{
		qDebug("Posted image to yfrog!");
		qDebug("Url is %s", status.getMediaUrl().toLatin1().constData());
		if (m_message.length() > 0)
		{
			m_message += " ";
		}

		m_message += status.getMediaUrl();

		QString hashtags = m_config->GetValue("hashtags");
		if (hashtags.length() > 0)
		{
			m_message = m_message + " " + hashtags;
		}

		qDebug("Posting link to twitter..");
		postMessage();
	}
}
