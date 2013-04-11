#ifndef CAMERA_H
#define CAMERA_H

#include <QObject>
#include <EDSDK.h>

class Camera : public QObject
{
	Q_OBJECT

	public:
		enum ErrorType
		{
			ErrorType_Normal,
			ErrorType_Extended,
		};

		// ErrorType_Normal error codes are the SDK errors.

		enum ExtendedError
		{
			ExtendedError_None,
			ExtendedError_CaptureError,
			ExtendedError_InternalError,
		};

		// imageDir is the directory where images will be saved.
		Camera(const QString& imageDir);

		// Start up the camera system.  Returns an error code of type
		// ErrorType_Normal the system couldn't be initialised.
		int Startup();

		// Shutdown the camera system.
		void Shutdown();

		// Connect to the first attached camera.  Returns an error code of
		// type ErrorType_Normal if the connection failed.
		int Connect();

		// Returns true if the system is connected to a camera.
		bool IsConnected();

		// Disconnects from any connected camera.
		void Disconnect();

		// Takes a photo.  Returns true on success or false if we
		// timed out waiting for the picture to be taken.
		void TakePicture();

		// Returns a human readable error message for the specified error.
		static QString GetErrorMessage(ErrorType errorType, int error);

	signals:
		// Signalled when a picture has been taken and downloaded from the camera.
		void OnTakePictureSuccess(const QString& filePath);

		// Signalled when taking a picture and there was an error.
		void OnTakePictureError(Camera::ErrorType errorType, int error);

		// Signalled when the camera has been disconnected.
		void OnCameraDisconnected();

	private:
		static EdsError EDSCALLBACK HandleStateEventImp(EdsUInt32 inEvent, EdsUInt32 inParam, EdsVoid* inContext);
		EdsError HandleStateEvent(EdsUInt32 inEvent, EdsUInt32 inParam);

		static EdsError EDSCALLBACK HandlePropertyEventImp(EdsPropertyEvent inEvent, EdsPropertyID inPropertyID, EdsUInt32 inParam, EdsVoid* inContext);
		EdsError HandlePropertyEvent(EdsPropertyEvent inEvent, EdsPropertyID inPropertyID, EdsUInt32 inParam);

		static EdsError EDSCALLBACK HandleObjectEventImp(EdsUInt32 inEvent, EdsBaseRef inRef, EdsVoid* inContext);
		EdsError HandleObjectEvent(EdsUInt32 inEvent, EdsBaseRef inRef);

		QString m_imageDir;
		EdsCameraRef m_camera;
};

#endif // CAMERA_H
