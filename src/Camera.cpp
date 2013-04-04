#include "Camera.h"
#include <QDateTime>
#include <QtDebug>
#include <QDir>
#include <QCoreApplication>
#include <QtWidgets/QApplication>
#include <EDSDK.h>

static QString s_filePath;
static bool s_photoDone = false;
static QString s_imageDir;
static EdsCameraRef s_camera = 0;
static EdsError s_lastError = EDS_ERR_OK;

static EdsError EDSCALLBACK HandleStateEvent(EdsUInt32 inEvent, EdsUInt32 inParam, EdsVoid* inContext)
{
	(void)inParam;
	(void)inContext;

	if (inEvent == kEdsStateEvent_Shutdown)
	{
		// Camera was disconnected externally so make sure we are disconnected.
		qWarning("Camera was disconnected!");
		Camera::Disconnect();
	}

	return EDS_ERR_OK;
}

static EdsError EDSCALLBACK HandleObjectEvent(EdsUInt32 inEvent, EdsBaseRef inRef, EdsVoid* inContext)
{
	(void)inContext;

	if (inEvent == kEdsObjectEvent_DirItemRequestTransfer)
	{
		// Transfer request event so download the image.
		EdsDirectoryItemRef dirItem = (EdsDirectoryItemRef)inRef;
		EdsDirectoryItemInfo dirItemInfo;
		EdsStreamRef stream = 0;

		// Get info about the file to transfer.
		s_lastError = EdsGetDirectoryItemInfo(dirItem, &dirItemInfo);
		if (s_lastError == EDS_ERR_OK)
		{
			// Create the output file.
			QDateTime now = QDateTime::currentDateTime();
			QDate nowDate = now.date();
			QTime nowTime = now.time();
			s_filePath.sprintf("%04i%02i%02i-%02i%02i%02i.jpg",
				nowDate.year(), nowDate.month(), nowDate.day(),
				nowTime.hour(), nowTime.minute(), nowTime.second());
			s_filePath = s_imageDir + "/" + s_filePath;
			s_lastError = EdsCreateFileStream(s_filePath.toLatin1().constData(), kEdsFileCreateDisposition_CreateAlways, kEdsAccess_ReadWrite, &stream);
			if (s_lastError == EDS_ERR_OK)
			{
				// Download the file.
				s_lastError = EdsDownload(dirItem, dirItemInfo.size, stream);
				if (s_lastError == EDS_ERR_OK)
				{
					// Signal download has finished.
					s_lastError = EdsDownloadComplete(dirItem);
				}
			}
		}

		if (stream)
		{
			EdsRelease(stream);
		}

		s_photoDone = true;
	}

	if (inRef)
	{
		EdsRelease(inRef);
	}

	return EDS_ERR_OK;
}

bool Camera::Startup()
{
	CoInitializeEx(0, COINIT_APARTMENTTHREADED);

	QString appDir = QCoreApplication::applicationDirPath();
	QDir imageDir = appDir + "/" + "images";
	if (!imageDir.exists())
	{
		imageDir.mkdir(imageDir.path());
	}

	s_imageDir = imageDir.path();

	Q_ASSERT(!s_camera);

	s_lastError = EdsInitializeSDK();
	return s_lastError == EDS_ERR_OK;
}

void Camera::Shutdown()
{
	if (IsConnected())
	{
		Disconnect();
	}

	EdsTerminateSDK();

	CoUninitialize();
}

bool Camera::Connect()
{
	if (IsConnected())
	{
		return true;
	}

	// Get camera list
	EdsCameraListRef cameraList = 0;
	s_lastError = EdsGetCameraList(&cameraList);
	if (s_lastError == EDS_ERR_OK)
	{
		// Get number of cameras
		EdsUInt32 count = 0;
		s_lastError = EdsGetChildCount(cameraList, &count);
		if (s_lastError == EDS_ERR_OK && count > 0)
		{
			// Get first camera.
			s_lastError = EdsGetChildAtIndex(cameraList, 0, &s_camera);
			if (s_lastError == EDS_ERR_OK)
			{
				// Check if the camera is a legacy device.
				EdsDeviceInfo deviceInfo;
				s_lastError = EdsGetDeviceInfo(s_camera , &deviceInfo);
				if (s_lastError == EDS_ERR_OK)
				{
					if (deviceInfo.deviceSubType == 0)
					{
						// Legacy devices aren't supported.
						qWarning("Camera is a legacy device!  Legacy devices are not supported!");
						s_lastError = EDS_ERR_DEVICE_NOT_FOUND;
					}
					else
					{
						// Open connection to camera.
						Q_ASSERT(s_camera);
						s_lastError = EdsOpenSession(s_camera);

						if (s_lastError == EDS_ERR_OK)
						{
							// Tell the camera to save to the host PC.
							EdsUInt32 saveTo = kEdsSaveTo_Host;
							s_lastError = EdsSetPropertyData(s_camera, kEdsPropID_SaveTo, 0, sizeof(saveTo), &saveTo);

							if (s_lastError == EDS_ERR_OK)
							{
								// Tell the camera we have plenty of space to store the image.
								EdsCapacity capacity = { 0x7FFFFFFF, 0x1000, 1 };
								s_lastError = EdsSetCapacity(s_camera, capacity);
							}
						}
					}
				}
			}
		}
		else if (s_lastError == EDS_ERR_OK)
		{
			// Last error was ok so count must be 0.
			s_lastError = EDS_ERR_DEVICE_NOT_FOUND;
		}
	}

	if (cameraList)
	{
		EdsRelease(cameraList);
	}

	if (s_lastError != EDS_ERR_OK && s_camera)
	{
		// Last error wasn't ok and there's currently a camera.
		EdsRelease(s_camera);
		s_camera = 0;
	}
	else if (s_lastError == EDS_ERR_OK)
	{
		s_lastError = EdsSetCameraStateEventHandler(s_camera, kEdsStateEvent_All, HandleStateEvent, 0);
		if (s_lastError == EDS_ERR_OK)
		{
			s_lastError = EdsSetObjectEventHandler(s_camera, kEdsObjectEvent_All, HandleObjectEvent, 0);
		}
	}

	return s_lastError == EDS_ERR_OK;
}

bool Camera::IsConnected()
{
	return s_camera != 0;
}

void Camera::Disconnect()
{
	if (IsConnected())
	{
		EdsCloseSession(s_camera);
		EdsRelease(s_camera);
		s_camera = 0;
	}
}

#define MAKE_ERROR(code) \
case code: \
{ \
	return #code; \
}

#define MAKE_ERROR_MSG(code, msg) \
case code: \
{ \
	return msg; \
}

QString Camera::GetLastErrorMessage()
{
	switch (s_lastError)
	{
		MAKE_ERROR(EDS_ERR_OK);

		/* Miscellaneous errors */
		MAKE_ERROR(EDS_ERR_UNIMPLEMENTED);
		MAKE_ERROR(EDS_ERR_INTERNAL_ERROR);
		MAKE_ERROR(EDS_ERR_MEM_ALLOC_FAILED);
		MAKE_ERROR(EDS_ERR_MEM_FREE_FAILED);
		MAKE_ERROR(EDS_ERR_OPERATION_CANCELLED);
		MAKE_ERROR(EDS_ERR_INCOMPATIBLE_VERSION);
		MAKE_ERROR(EDS_ERR_NOT_SUPPORTED);
		MAKE_ERROR(EDS_ERR_UNEXPECTED_EXCEPTION);
		MAKE_ERROR(EDS_ERR_PROTECTION_VIOLATION);
		MAKE_ERROR(EDS_ERR_MISSING_SUBCOMPONENT);
		MAKE_ERROR(EDS_ERR_SELECTION_UNAVAILABLE);

		/* File errors */
		MAKE_ERROR(EDS_ERR_FILE_IO_ERROR);
		MAKE_ERROR(EDS_ERR_FILE_TOO_MANY_OPEN);
		MAKE_ERROR(EDS_ERR_FILE_NOT_FOUND);
		MAKE_ERROR(EDS_ERR_FILE_OPEN_ERROR);
		MAKE_ERROR(EDS_ERR_FILE_CLOSE_ERROR);
		MAKE_ERROR(EDS_ERR_FILE_SEEK_ERROR);
		MAKE_ERROR(EDS_ERR_FILE_TELL_ERROR);
		MAKE_ERROR(EDS_ERR_FILE_READ_ERROR);
		MAKE_ERROR(EDS_ERR_FILE_WRITE_ERROR);
		MAKE_ERROR(EDS_ERR_FILE_PERMISSION_ERROR);
		MAKE_ERROR(EDS_ERR_FILE_DISK_FULL_ERROR);
		MAKE_ERROR(EDS_ERR_FILE_ALREADY_EXISTS);
		MAKE_ERROR(EDS_ERR_FILE_FORMAT_UNRECOGNIZED);
		MAKE_ERROR(EDS_ERR_FILE_DATA_CORRUPT);
		MAKE_ERROR(EDS_ERR_FILE_NAMING_NA);

		/* Directory errors */
		MAKE_ERROR(EDS_ERR_DIR_NOT_FOUND);
		MAKE_ERROR(EDS_ERR_DIR_IO_ERROR);
		MAKE_ERROR(EDS_ERR_DIR_ENTRY_NOT_FOUND);
		MAKE_ERROR(EDS_ERR_DIR_ENTRY_EXISTS);
		MAKE_ERROR(EDS_ERR_DIR_NOT_EMPTY);

		/* Property errors */
		MAKE_ERROR(EDS_ERR_PROPERTIES_UNAVAILABLE);
		MAKE_ERROR(EDS_ERR_PROPERTIES_MISMATCH);
		MAKE_ERROR(EDS_ERR_PROPERTIES_NOT_LOADED);

		/* Function Parameter errors */
		MAKE_ERROR(EDS_ERR_INVALID_PARAMETER);
		MAKE_ERROR(EDS_ERR_INVALID_HANDLE);
		MAKE_ERROR(EDS_ERR_INVALID_POINTER);
		MAKE_ERROR(EDS_ERR_INVALID_INDEX);
		MAKE_ERROR(EDS_ERR_INVALID_LENGTH);
		MAKE_ERROR(EDS_ERR_INVALID_FN_POINTER);
		MAKE_ERROR(EDS_ERR_INVALID_SORT_FN);

		/* Device errors */
		MAKE_ERROR_MSG(EDS_ERR_DEVICE_NOT_FOUND, "No cameras found.  Make sure a camera is plugged into the computer.");
		MAKE_ERROR(EDS_ERR_DEVICE_BUSY);
		MAKE_ERROR(EDS_ERR_DEVICE_INVALID);
		MAKE_ERROR(EDS_ERR_DEVICE_EMERGENCY);
		MAKE_ERROR(EDS_ERR_DEVICE_MEMORY_FULL);
		MAKE_ERROR(EDS_ERR_DEVICE_INTERNAL_ERROR);
		MAKE_ERROR(EDS_ERR_DEVICE_INVALID_PARAMETER);
		MAKE_ERROR(EDS_ERR_DEVICE_NO_DISK);
		MAKE_ERROR(EDS_ERR_DEVICE_DISK_ERROR);
		MAKE_ERROR(EDS_ERR_DEVICE_CF_GATE_CHANGED);
		MAKE_ERROR(EDS_ERR_DEVICE_DIAL_CHANGED);
		MAKE_ERROR(EDS_ERR_DEVICE_NOT_INSTALLED);
		MAKE_ERROR(EDS_ERR_DEVICE_STAY_AWAKE);
		MAKE_ERROR(EDS_ERR_DEVICE_NOT_RELEASED);

		/* Stream errors */
		MAKE_ERROR(EDS_ERR_STREAM_IO_ERROR);
		MAKE_ERROR(EDS_ERR_STREAM_NOT_OPEN);
		MAKE_ERROR(EDS_ERR_STREAM_ALREADY_OPEN);
		MAKE_ERROR(EDS_ERR_STREAM_OPEN_ERROR);
		MAKE_ERROR(EDS_ERR_STREAM_CLOSE_ERROR);
		MAKE_ERROR(EDS_ERR_STREAM_SEEK_ERROR);
		MAKE_ERROR(EDS_ERR_STREAM_TELL_ERROR);
		MAKE_ERROR(EDS_ERR_STREAM_READ_ERROR);
		MAKE_ERROR(EDS_ERR_STREAM_WRITE_ERROR);
		MAKE_ERROR(EDS_ERR_STREAM_PERMISSION_ERROR);
		MAKE_ERROR(EDS_ERR_STREAM_COULDNT_BEGIN_THREAD);
		MAKE_ERROR(EDS_ERR_STREAM_BAD_OPTIONS);
		MAKE_ERROR(EDS_ERR_STREAM_END_OF_STREAM);

		/* Communications errors */
		MAKE_ERROR(EDS_ERR_COMM_PORT_IS_IN_USE);
		MAKE_ERROR(EDS_ERR_COMM_DISCONNECTED);
		MAKE_ERROR(EDS_ERR_COMM_DEVICE_INCOMPATIBLE);
		MAKE_ERROR(EDS_ERR_COMM_BUFFER_FULL);
		MAKE_ERROR(EDS_ERR_COMM_USB_BUS_ERR);

		/* Lock/Unlock */
		MAKE_ERROR(EDS_ERR_USB_DEVICE_LOCK_ERROR);
		MAKE_ERROR(EDS_ERR_USB_DEVICE_UNLOCK_ERROR);

		/* STI/WIA */
		MAKE_ERROR(EDS_ERR_STI_UNKNOWN_ERROR);
		MAKE_ERROR(EDS_ERR_STI_INTERNAL_ERROR);
		MAKE_ERROR(EDS_ERR_STI_DEVICE_CREATE_ERROR);
		MAKE_ERROR(EDS_ERR_STI_DEVICE_RELEASE_ERROR);
		MAKE_ERROR(EDS_ERR_DEVICE_NOT_LAUNCHED);

		MAKE_ERROR(EDS_ERR_ENUM_NA);
		MAKE_ERROR(EDS_ERR_INVALID_FN_CALL);
		MAKE_ERROR(EDS_ERR_HANDLE_NOT_FOUND);
		MAKE_ERROR(EDS_ERR_INVALID_ID);
		MAKE_ERROR(EDS_ERR_WAIT_TIMEOUT_ERROR);

		/* PTP */
		MAKE_ERROR(EDS_ERR_SESSION_NOT_OPEN);
		MAKE_ERROR(EDS_ERR_INVALID_TRANSACTIONID);
		MAKE_ERROR(EDS_ERR_INCOMPLETE_TRANSFER);
		MAKE_ERROR(EDS_ERR_INVALID_STRAGEID);
		MAKE_ERROR(EDS_ERR_DEVICEPROP_NOT_SUPPORTED);
		MAKE_ERROR(EDS_ERR_INVALID_OBJECTFORMATCODE);
		MAKE_ERROR(EDS_ERR_SELF_TEST_FAILED);
		MAKE_ERROR(EDS_ERR_PARTIAL_DELETION);
		MAKE_ERROR(EDS_ERR_SPECIFICATION_BY_FORMAT_UNSUPPORTED);
		MAKE_ERROR(EDS_ERR_NO_VALID_OBJECTINFO);
		MAKE_ERROR(EDS_ERR_INVALID_CODE_FORMAT);
		MAKE_ERROR(EDS_ERR_UNKNOWN_VENDOR_CODE);
		MAKE_ERROR(EDS_ERR_CAPTURE_ALREADY_TERMINATED);
		MAKE_ERROR(EDS_ERR_INVALID_PARENTOBJECT);
		MAKE_ERROR(EDS_ERR_INVALID_DEVICEPROP_FORMAT);
		MAKE_ERROR(EDS_ERR_INVALID_DEVICEPROP_VALUE);
		MAKE_ERROR(EDS_ERR_SESSION_ALREADY_OPEN);
		MAKE_ERROR(EDS_ERR_TRANSACTION_CANCELLED);
		MAKE_ERROR(EDS_ERR_SPECIFICATION_OF_DESTINATION_UNSUPPORTED);
		MAKE_ERROR(EDS_ERR_NOT_CAMERA_SUPPORT_SDK_VERSION);

		/* PTP Vendor */
		MAKE_ERROR(EDS_ERR_UNKNOWN_COMMAND);
		MAKE_ERROR(EDS_ERR_OPERATION_REFUSED);
		MAKE_ERROR(EDS_ERR_LENS_COVER_CLOSE);
		MAKE_ERROR(EDS_ERR_LOW_BATTERY);
		MAKE_ERROR(EDS_ERR_OBJECT_NOTREADY);
		MAKE_ERROR(EDS_ERR_CANNOT_MAKE_OBJECT);

		/* Take Picture errors */ 
		MAKE_ERROR(EDS_ERR_TAKE_PICTURE_AF_NG);
		MAKE_ERROR(EDS_ERR_TAKE_PICTURE_RESERVED);
		MAKE_ERROR(EDS_ERR_TAKE_PICTURE_MIRROR_UP_NG);
		MAKE_ERROR(EDS_ERR_TAKE_PICTURE_SENSOR_CLEANING_NG);
		MAKE_ERROR(EDS_ERR_TAKE_PICTURE_SILENCE_NG);
		MAKE_ERROR(EDS_ERR_TAKE_PICTURE_NO_CARD_NG);
		MAKE_ERROR(EDS_ERR_TAKE_PICTURE_CARD_NG);
		MAKE_ERROR(EDS_ERR_TAKE_PICTURE_CARD_PROTECT_NG);
		MAKE_ERROR(EDS_ERR_TAKE_PICTURE_MOVIE_CROP_NG);
		MAKE_ERROR(EDS_ERR_TAKE_PICTURE_STROBO_CHARGE_NG);
	}

	return "Unknown error";
}

bool Camera::TakePicture(QString& imagePath)
{
	Q_ASSERT(IsConnected());
	s_photoDone = false;
	s_filePath = "";
	s_lastError = EdsSendCommand(s_camera, kEdsCameraCommand_TakePicture, 0);
	if (s_lastError == EDS_ERR_OK)
	{
		// Wait for picture to download.
		while (!s_photoDone)
		{
			QApplication::processEvents();
		}

		if (s_lastError == EDS_ERR_OK)
		{
			imagePath = s_filePath;
		}
	}

	return s_lastError == EDS_ERR_OK;
}
