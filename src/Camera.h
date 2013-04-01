#ifndef CAMERA_H
#define CAMERA_H

#include <QObject>

namespace Camera
{
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
};

#endif // CAMERA_H
