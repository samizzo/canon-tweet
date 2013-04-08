#include <QDateTime>
#include <QtDebug>
#include <QDir>
#include <QCoreApplication>
#include <QtWidgets/QApplication>
#include "Camera.h"

EdsError EDSCALLBACK Camera::HandlePropertyEventImp(EdsPropertyEvent inEvent, EdsPropertyID inPropertyID, EdsUInt32 inParam, EdsVoid* inContext)
{
	(void)inEvent;
	(void)inPropertyID;
	(void)inParam;
	Camera* camera = (Camera*)(inContext);
	return camera->HandlePropertyEvent(inEvent, inPropertyID, inParam);
}

EdsError Camera::HandlePropertyEvent(EdsPropertyEvent inEvent, EdsPropertyID inPropertyID, EdsUInt32 inParam)
{
	(void)inParam;
	(void)inPropertyID;
	(void)inEvent;
	return EDS_ERR_OK;
}

EdsError EDSCALLBACK Camera::HandleStateEventImp(EdsUInt32 inEvent, EdsUInt32 inParam, EdsVoid* inContext)
{
	(void)inParam;
	Camera* camera = (Camera*)(inContext);
	return camera->HandleStateEvent(inEvent, inParam);
}

EdsError Camera::HandleStateEvent(EdsUInt32 inEvent, EdsUInt32 inParam)
{
	(void)inParam;
	if (inEvent == kEdsStateEvent_Shutdown)
	{
		// Camera was disconnected externally so make sure we are disconnected.
		qWarning("Camera was disconnected!");
		Disconnect();
	}
	else if (inEvent == kEdsStateEvent_CaptureError)
	{
		m_lastError = 0xffffffff;
		m_extendedError = ExtendedError_CaptureError;
	}
	else if (inEvent == kEdsStateEvent_InternalError)
	{
		m_lastError = 0xffffffff;
		m_extendedError = ExtendedError_InternalError;
	}

	return EDS_ERR_OK;
}

EdsError EDSCALLBACK Camera::HandleObjectEventImp(EdsUInt32 inEvent, EdsBaseRef inRef, EdsVoid* inContext)
{
	Camera* camera = (Camera*)(inContext);
	return camera->HandleObjectEvent(inEvent, inRef);
}

EdsError Camera::HandleObjectEvent(EdsUInt32 inEvent, EdsBaseRef inRef)
{
	if (inEvent == kEdsObjectEvent_DirItemRequestTransfer)
	{
		// Transfer request event so download the image.
		EdsDirectoryItemRef dirItem = (EdsDirectoryItemRef)inRef;
		EdsDirectoryItemInfo dirItemInfo;
		EdsStreamRef stream = 0;

		// Get info about the file to transfer.
		m_lastError = EdsGetDirectoryItemInfo(dirItem, &dirItemInfo);
		if (m_lastError == EDS_ERR_OK)
		{
			// Create the output file.
			QDateTime now = QDateTime::currentDateTime();
			QDate nowDate = now.date();
			QTime nowTime = now.time();
			m_filePath.sprintf("%04i%02i%02i-%02i%02i%02i.jpg",
				nowDate.year(), nowDate.month(), nowDate.day(),
				nowTime.hour(), nowTime.minute(), nowTime.second());
			m_filePath = m_imageDir + "/" + m_filePath;
			m_lastError = EdsCreateFileStream(m_filePath.toLatin1().constData(), kEdsFileCreateDisposition_CreateAlways, kEdsAccess_ReadWrite, &stream);
			if (m_lastError == EDS_ERR_OK)
			{
				// Download the file.
				m_lastError = EdsDownload(dirItem, dirItemInfo.size, stream);
				if (m_lastError == EDS_ERR_OK)
				{
					// Signal download has finished.
					m_lastError = EdsDownloadComplete(dirItem);
				}
			}
		}

		if (stream)
		{
			EdsRelease(stream);
		}

		m_photoDone = true;
	}

	if (inRef)
	{
		EdsRelease(inRef);
	}

	return EDS_ERR_OK;
}

Camera::Camera() :
m_photoDone(false),
m_camera(0),
m_lastError(EDS_ERR_OK),
m_extendedError(ExtendedError_None)
{
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

	m_imageDir = imageDir.path();

	Q_ASSERT(!m_camera);

	m_lastError = EdsInitializeSDK();
	return m_lastError == EDS_ERR_OK;
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
	m_lastError = EdsGetCameraList(&cameraList);
	if (m_lastError == EDS_ERR_OK)
	{
		// Get number of cameras
		EdsUInt32 count = 0;
		m_lastError = EdsGetChildCount(cameraList, &count);
		if (m_lastError == EDS_ERR_OK && count > 0)
		{
			// Get first camera.
			m_lastError = EdsGetChildAtIndex(cameraList, 0, &m_camera);
			if (m_lastError == EDS_ERR_OK)
			{
				// Check if the camera is a legacy device.
				EdsDeviceInfo deviceInfo;
				m_lastError = EdsGetDeviceInfo(m_camera , &deviceInfo);
				if (m_lastError == EDS_ERR_OK)
				{
					if (deviceInfo.deviceSubType == 0)
					{
						// Legacy devices aren't supported.
						qWarning("Camera is a legacy device!  Legacy devices are not supported!");
						m_lastError = EDS_ERR_DEVICE_NOT_FOUND;
					}
					else
					{
						// Open connection to camera.
						Q_ASSERT(m_camera);
						m_lastError = EdsOpenSession(m_camera);

						if (m_lastError == EDS_ERR_OK)
						{
							// Tell the camera to save to the host PC.
							EdsUInt32 saveTo = kEdsSaveTo_Host;
							m_lastError = EdsSetPropertyData(m_camera, kEdsPropID_SaveTo, 0, sizeof(saveTo), &saveTo);

							if (m_lastError == EDS_ERR_OK)
							{
								// Tell the camera we have plenty of space to store the image.
								EdsCapacity capacity = { 0x7FFFFFFF, 0x1000, 1 };
								m_lastError = EdsSetCapacity(m_camera, capacity);
							}
						}
					}
				}
			}
		}
		else if (m_lastError == EDS_ERR_OK)
		{
			// Last error was ok so count must be 0.
			m_lastError = EDS_ERR_DEVICE_NOT_FOUND;
		}
	}

	if (cameraList)
	{
		EdsRelease(cameraList);
	}

	if (m_lastError != EDS_ERR_OK && m_camera)
	{
		// Last error wasn't ok and there's currently a camera.
		EdsRelease(m_camera);
		m_camera = 0;
	}
	else if (m_lastError == EDS_ERR_OK)
	{
		m_lastError = EdsSetCameraStateEventHandler(m_camera, kEdsStateEvent_All, HandleStateEventImp, this);
		if (m_lastError == EDS_ERR_OK)
		{
			m_lastError = EdsSetObjectEventHandler(m_camera, kEdsObjectEvent_All, HandleObjectEventImp, this);
			if (m_lastError == EDS_ERR_OK)
			{
				m_lastError = EdsSetPropertyEventHandler(m_camera, kEdsObjectEvent_All, HandlePropertyEventImp, this);
			}
		}
	}

	return m_lastError == EDS_ERR_OK;
}

bool Camera::IsConnected()
{
	return m_camera != 0;
}

void Camera::Disconnect()
{
	if (IsConnected())
	{
		EdsCloseSession(m_camera);
		EdsRelease(m_camera);
		m_camera = 0;
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

#define MAKE_EXTENDED_ERROR(code, msg) \
case code: \
{ \
	return msg; \
}

QString Camera::GetLastErrorMessage()
{
	switch (m_lastError)
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
		MAKE_ERROR_MSG(EDS_ERR_COMM_DISCONNECTED, "Camera was disconnected!");
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
		MAKE_ERROR_MSG(EDS_ERR_TAKE_PICTURE_AF_NG, "Couldn't take a photo because autofocus failed!");
		MAKE_ERROR(EDS_ERR_TAKE_PICTURE_RESERVED);
		MAKE_ERROR(EDS_ERR_TAKE_PICTURE_MIRROR_UP_NG);
		MAKE_ERROR(EDS_ERR_TAKE_PICTURE_SENSOR_CLEANING_NG);
		MAKE_ERROR(EDS_ERR_TAKE_PICTURE_SILENCE_NG);
		MAKE_ERROR(EDS_ERR_TAKE_PICTURE_NO_CARD_NG);
		MAKE_ERROR(EDS_ERR_TAKE_PICTURE_CARD_NG);
		MAKE_ERROR(EDS_ERR_TAKE_PICTURE_CARD_PROTECT_NG);
		MAKE_ERROR(EDS_ERR_TAKE_PICTURE_MOVIE_CROP_NG);
		MAKE_ERROR(EDS_ERR_TAKE_PICTURE_STROBO_CHARGE_NG);

		case 0xffffffff:
		{
			switch (m_extendedError)
			{
				MAKE_EXTENDED_ERROR(ExtendedError_CaptureError, "Couldn't take a photo because camera couldn't focus!");
				MAKE_EXTENDED_ERROR(ExtendedError_InternalError, "Internal error while taking photo!");
			}
		}
	}

	return "Unknown error";
}

bool Camera::TakePicture(QString& imagePath)
{
	Q_ASSERT(IsConnected());
	m_photoDone = false;
	m_filePath = "";
	m_lastError = EdsSendCommand(m_camera, kEdsCameraCommand_TakePicture, 0);
	if (m_lastError == EDS_ERR_OK)
	{
		// Wait for picture to download.
		while (!m_photoDone)
		{
			QApplication::processEvents();
		}

		if (m_lastError == EDS_ERR_OK)
		{
			imagePath = m_filePath;
		}
	}

	return m_lastError == EDS_ERR_OK;
}
