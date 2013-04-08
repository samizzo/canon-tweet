#ifndef CAMERA_H
#define CAMERA_H

#include <QObject>
#include <EDSDK.h>

class Camera : public QObject
{
	Q_OBJECT

	public:
		enum ExtendedError
		{
			ExtendedError_None,
			ExtendedError_CaptureError,
			ExtendedError_InternalError,
		};

		Camera();

		// Start up the camera system.  Returns false if the system
		// couldn't be initialised.
		bool Startup();

		// Shutdown the camera system.
		void Shutdown();

		// Connect to the first attached camera.  Returns true if
		// connection was successful.
		bool Connect();

		// Returns true if the system is connected to a camera.
		bool IsConnected();

		// Disconnects from any connected camera.
		void Disconnect();

		// Takes a photo.  Returns true on success or false if we
		// timed out waiting for the picture to be taken.
		bool TakePicture(QString& imagePath);

		// Returns a human readable error message for the last error.
		QString GetLastErrorMessage();

	private:
		static EdsError EDSCALLBACK HandleStateEventImp(EdsUInt32 inEvent, EdsUInt32 inParam, EdsVoid* inContext);
		EdsError HandleStateEvent(EdsUInt32 inEvent, EdsUInt32 inParam);

		static EdsError EDSCALLBACK HandlePropertyEventImp(EdsPropertyEvent inEvent, EdsPropertyID inPropertyID, EdsUInt32 inParam, EdsVoid* inContext);
		EdsError HandlePropertyEvent(EdsPropertyEvent inEvent, EdsPropertyID inPropertyID, EdsUInt32 inParam);

		static EdsError EDSCALLBACK HandleObjectEventImp(EdsUInt32 inEvent, EdsBaseRef inRef, EdsVoid* inContext);
		EdsError HandleObjectEvent(EdsUInt32 inEvent, EdsBaseRef inRef);

		QString m_filePath;
		bool m_photoDone;
		QString m_imageDir;
		EdsCameraRef m_camera;
		EdsError m_lastError;
		ExtendedError m_extendedError;
};

#endif // CAMERA_H
