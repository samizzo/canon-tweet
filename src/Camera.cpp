#include <QDateTime>
#include <QtDebug>
#include <QDir>
#include <QCoreApplication>
#include <QtWidgets/QApplication>
#include "Camera.h"
#include "../QsLog/QsLog.h"

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
		QLOG_WARN() << "Camera was disconnected!";
		Disconnect();
	}
	else if (inEvent == kEdsStateEvent_CaptureError)
	{
		emit OnTakePictureError(ErrorType_Extended, ExtendedError_CaptureError);
	}
	else if (inEvent == kEdsStateEvent_InternalError)
	{
		emit OnTakePictureError(ErrorType_Extended, ExtendedError_InternalError);
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

		QString filePath;

		// Get info about the file to transfer.
		EdsError lastError = EdsGetDirectoryItemInfo(dirItem, &dirItemInfo);
		if (lastError == EDS_ERR_OK)
		{
			// Create the output file.
			QDateTime now = QDateTime::currentDateTime();
			QDate nowDate = now.date();
			QTime nowTime = now.time();
			filePath.sprintf("%04i%02i%02i-%02i%02i%02i.jpg",
				nowDate.year(), nowDate.month(), nowDate.day(),
				nowTime.hour(), nowTime.minute(), nowTime.second());
			filePath = m_imageDir + "/" + filePath;
			lastError = EdsCreateFileStream(filePath.toLatin1().constData(), kEdsFileCreateDisposition_CreateAlways, kEdsAccess_ReadWrite, &stream);
			if (lastError == EDS_ERR_OK)
			{
				// Download the file.
				lastError = EdsDownload(dirItem, dirItemInfo.size, stream);
				if (lastError == EDS_ERR_OK)
				{
					// Signal download has finished.
					lastError = EdsDownloadComplete(dirItem);
				}
			}
		}

		if (stream)
		{
			EdsRelease(stream);
		}

		// Signal success or failure.
		if (lastError == EDS_ERR_OK)
		{
			emit OnTakePictureSuccess(filePath);
		}
		else
		{
			emit OnTakePictureError(ErrorType_Normal, lastError);
		}
	}

	if (inRef)
	{
		EdsRelease(inRef);
	}

	return EDS_ERR_OK;
}

Camera::Camera(const QString& imageDir) :
m_camera(0),
m_imageDir(imageDir)
{
	QDir dir(imageDir);
	if (!dir.exists())
	{
		dir.mkdir(dir.path());
	}
}

int Camera::Startup()
{
	CoInitializeEx(0, COINIT_APARTMENTTHREADED);

	Q_ASSERT(!m_camera);

	EdsError err = EdsInitializeSDK();

	if (err == EDS_ERR_OK)
	{
		err = Connect();
	}

	return err;
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

int Camera::Connect()
{
	if (IsConnected())
	{
		return EDS_ERR_OK;
	}

	// Get camera list
	EdsCameraListRef cameraList = 0;
	EdsError lastError = EdsGetCameraList(&cameraList);
	if (lastError == EDS_ERR_OK)
	{
		// Get number of cameras
		EdsUInt32 count = 0;
		lastError = EdsGetChildCount(cameraList, &count);
		if (lastError == EDS_ERR_OK && count > 0)
		{
			// Get first camera.
			lastError = EdsGetChildAtIndex(cameraList, 0, &m_camera);
			if (lastError == EDS_ERR_OK)
			{
				// Check if the camera is a legacy device.
				EdsDeviceInfo deviceInfo;
				lastError = EdsGetDeviceInfo(m_camera , &deviceInfo);
				if (lastError == EDS_ERR_OK)
				{
					if (deviceInfo.deviceSubType == 0)
					{
						// Legacy devices aren't supported.
						QLOG_ERROR() << "Camera is a legacy device!  Legacy devices are not supported!";
						lastError = EDS_ERR_DEVICE_NOT_FOUND;
					}
					else
					{
						// Open connection to camera.
						Q_ASSERT(m_camera);
						lastError = EdsOpenSession(m_camera);

						if (lastError == EDS_ERR_OK)
						{
							// Tell the camera to save to the host PC.
							EdsUInt32 saveTo = kEdsSaveTo_Host;
							lastError = EdsSetPropertyData(m_camera, kEdsPropID_SaveTo, 0, sizeof(saveTo), &saveTo);

							if (lastError == EDS_ERR_OK)
							{
								// Tell the camera we have plenty of space to store the image.
								EdsCapacity capacity = { 0x7FFFFFFF, 0x1000, 1 };
								lastError = EdsSetCapacity(m_camera, capacity);
							}
						}
					}
				}
			}
		}
		else if (lastError == EDS_ERR_OK)
		{
			// Last error was ok so count must be 0.
			lastError = EDS_ERR_DEVICE_NOT_FOUND;
		}
	}

	if (cameraList)
	{
		EdsRelease(cameraList);
	}

	if (lastError != EDS_ERR_OK && m_camera)
	{
		// Last error wasn't ok and there's currently a camera.
		EdsRelease(m_camera);
		m_camera = 0;
	}
	else if (lastError == EDS_ERR_OK)
	{
		lastError = EdsSetCameraStateEventHandler(m_camera, kEdsStateEvent_All, HandleStateEventImp, this);
		if (lastError == EDS_ERR_OK)
		{
			lastError = EdsSetObjectEventHandler(m_camera, kEdsObjectEvent_All, HandleObjectEventImp, this);
			if (lastError == EDS_ERR_OK)
			{
				lastError = EdsSetPropertyEventHandler(m_camera, kEdsObjectEvent_All, HandlePropertyEventImp, this);
			}
		}
	}

	if (lastError == EDS_ERR_OK)
	{
		QLOG_INFO() << "Camera connected!";
	}

	return lastError;
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

QString Camera::GetErrorMessage(ErrorType errorType, int error)
{
	if (errorType == ErrorType_Normal)
	{
		switch (error)
		{
			MAKE_ERROR(EDS_ERR_OK);

			/* Miscellaneous errors */
			MAKE_ERROR(EDS_ERR_UNIMPLEMENTED);
			MAKE_ERROR_MSG(EDS_ERR_INTERNAL_ERROR, "Internal error! Try turning the camera off and on again.");
			MAKE_ERROR_MSG(EDS_ERR_MEM_ALLOC_FAILED, "Failed to allocate memory! Try restarting the application.");
			MAKE_ERROR_MSG(EDS_ERR_MEM_FREE_FAILED, "Failed to free memory! Try restarting the application.");
			MAKE_ERROR(EDS_ERR_OPERATION_CANCELLED);
			MAKE_ERROR(EDS_ERR_INCOMPATIBLE_VERSION);
			MAKE_ERROR(EDS_ERR_NOT_SUPPORTED);
			MAKE_ERROR_MSG(EDS_ERR_UNEXPECTED_EXCEPTION, "Unexpected exception! Try restarting the application.");
			MAKE_ERROR_MSG(EDS_ERR_PROTECTION_VIOLATION, "Protection violation! Try restarting the application.");
			MAKE_ERROR(EDS_ERR_MISSING_SUBCOMPONENT);
			MAKE_ERROR(EDS_ERR_SELECTION_UNAVAILABLE);

			/* File errors */
			MAKE_ERROR_MSG(EDS_ERR_FILE_IO_ERROR, "IO error writing to the output file!");
			MAKE_ERROR_MSG(EDS_ERR_FILE_TOO_MANY_OPEN, "Too many files open!");
			MAKE_ERROR(EDS_ERR_FILE_NOT_FOUND);
			MAKE_ERROR_MSG(EDS_ERR_FILE_OPEN_ERROR, "Failed to open the output file!");
			MAKE_ERROR(EDS_ERR_FILE_CLOSE_ERROR);
			MAKE_ERROR(EDS_ERR_FILE_SEEK_ERROR);
			MAKE_ERROR(EDS_ERR_FILE_TELL_ERROR);
			MAKE_ERROR(EDS_ERR_FILE_READ_ERROR);
			MAKE_ERROR_MSG(EDS_ERR_FILE_WRITE_ERROR, "Failed to write to the disk!");
			MAKE_ERROR_MSG(EDS_ERR_FILE_PERMISSION_ERROR, "Failed to write to the disk because of a permissions error!");
			MAKE_ERROR_MSG(EDS_ERR_FILE_DISK_FULL_ERROR, "The disk is full!");
			MAKE_ERROR_MSG(EDS_ERR_FILE_ALREADY_EXISTS, "The output file already exists!");
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
			MAKE_ERROR_MSG(EDS_ERR_DEVICE_NOT_FOUND, "No cameras found. Make sure a camera is plugged into the computer.");
			MAKE_ERROR_MSG(EDS_ERR_DEVICE_BUSY, "The device is busy! If this happens again try turning the camera off and on.");
			MAKE_ERROR(EDS_ERR_DEVICE_INVALID);
			MAKE_ERROR_MSG(EDS_ERR_DEVICE_EMERGENCY, "Device emergency! Try turning the camera off and on again.");
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
			MAKE_ERROR_MSG(EDS_ERR_STREAM_PERMISSION_ERROR, "Permission error!");
			MAKE_ERROR(EDS_ERR_STREAM_COULDNT_BEGIN_THREAD);
			MAKE_ERROR(EDS_ERR_STREAM_BAD_OPTIONS);
			MAKE_ERROR_MSG(EDS_ERR_STREAM_END_OF_STREAM, "The stream was terminated!");

			/* Communications errors */
			MAKE_ERROR_MSG(EDS_ERR_COMM_PORT_IS_IN_USE, "The camera's communications port is already in use!");
			MAKE_ERROR_MSG(EDS_ERR_COMM_DISCONNECTED, "The camera was disconnected!");
			MAKE_ERROR(EDS_ERR_COMM_DEVICE_INCOMPATIBLE);
			MAKE_ERROR_MSG(EDS_ERR_COMM_BUFFER_FULL, "The camera's communications buffer is full!");
			MAKE_ERROR_MSG(EDS_ERR_COMM_USB_BUS_ERR, "USB bus error!");

			/* Lock/Unlock */
			MAKE_ERROR(EDS_ERR_USB_DEVICE_LOCK_ERROR);
			MAKE_ERROR(EDS_ERR_USB_DEVICE_UNLOCK_ERROR);

			/* STI/WIA */
			MAKE_ERROR(EDS_ERR_STI_UNKNOWN_ERROR);
			MAKE_ERROR(EDS_ERR_STI_INTERNAL_ERROR);
			MAKE_ERROR_MSG(EDS_ERR_STI_DEVICE_CREATE_ERROR, "The camera device creation failed!");
			MAKE_ERROR_MSG(EDS_ERR_STI_DEVICE_RELEASE_ERROR, "The camera shutdown release step failed!");
			MAKE_ERROR_MSG(EDS_ERR_DEVICE_NOT_LAUNCHED, "The camera startup failed!");

			MAKE_ERROR(EDS_ERR_ENUM_NA);
			MAKE_ERROR(EDS_ERR_INVALID_FN_CALL);
			MAKE_ERROR(EDS_ERR_HANDLE_NOT_FOUND);
			MAKE_ERROR(EDS_ERR_INVALID_ID);
			MAKE_ERROR(EDS_ERR_WAIT_TIMEOUT_ERROR);

			/* PTP */
			MAKE_ERROR_MSG(EDS_ERR_SESSION_NOT_OPEN, "Tried to talk to the camera but a session is not open!");
			MAKE_ERROR(EDS_ERR_INVALID_TRANSACTIONID);
			MAKE_ERROR_MSG(EDS_ERR_INCOMPLETE_TRANSFER, "The image transfer from the camera to the computer has failed!");
			MAKE_ERROR(EDS_ERR_INVALID_STRAGEID);
			MAKE_ERROR(EDS_ERR_DEVICEPROP_NOT_SUPPORTED);
			MAKE_ERROR(EDS_ERR_INVALID_OBJECTFORMATCODE);
			MAKE_ERROR_MSG(EDS_ERR_SELF_TEST_FAILED, "Camera self-test failed!");
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
			MAKE_ERROR_MSG(EDS_ERR_LENS_COVER_CLOSE, "The camera lens cover is closed!");
			MAKE_ERROR_MSG(EDS_ERR_LOW_BATTERY, "The camera's battery is low!");
			MAKE_ERROR(EDS_ERR_OBJECT_NOTREADY);
			MAKE_ERROR(EDS_ERR_CANNOT_MAKE_OBJECT);

			/* Take Picture errors */ 
			MAKE_ERROR_MSG(EDS_ERR_TAKE_PICTURE_AF_NG, "Couldn't take a photo because autofocus failed!");
			MAKE_ERROR(EDS_ERR_TAKE_PICTURE_RESERVED);
			MAKE_ERROR_MSG(EDS_ERR_TAKE_PICTURE_MIRROR_UP_NG, "Currently configuring mirror up");
			MAKE_ERROR_MSG(EDS_ERR_TAKE_PICTURE_SENSOR_CLEANING_NG, "Currently cleaning camera sensor");
			MAKE_ERROR(EDS_ERR_TAKE_PICTURE_SILENCE_NG);
			MAKE_ERROR_MSG(EDS_ERR_TAKE_PICTURE_NO_CARD_NG, "No memory card installed in the camera!");
			MAKE_ERROR_MSG(EDS_ERR_TAKE_PICTURE_CARD_NG, "Error writing to the memory card in the camera!");
			MAKE_ERROR_MSG(EDS_ERR_TAKE_PICTURE_CARD_PROTECT_NG, "The memory card in the camera is write protected");
			MAKE_ERROR(EDS_ERR_TAKE_PICTURE_MOVIE_CROP_NG);
			MAKE_ERROR(EDS_ERR_TAKE_PICTURE_STROBO_CHARGE_NG);

			// Got this error when battery was low and about to die.
			MAKE_ERROR_MSG(0xAD19, "Camera failed to release the shutter! Check the battery.");
		}
	}
	else
	{
		Q_ASSERT(errorType == ErrorType_Extended);

		switch (error)
		{
			MAKE_EXTENDED_ERROR(ExtendedError_CaptureError, "Couldn't take a photo because the camera couldn't focus!");
			MAKE_EXTENDED_ERROR(ExtendedError_InternalError, "Internal error while taking a photo!");
		}
	}

	return "Unknown error";
}

void Camera::TakePicture()
{
	EdsError error;

	if (!IsConnected())
	{
		error = Connect();
		if (error != EDS_ERR_OK)
		{
			// Couldn't connect, just bail out.
			return emit OnTakePictureError(ErrorType_Normal, error);
		}
	}

	error = EdsSendCommand(m_camera, kEdsCameraCommand_TakePicture, 0);

	if (error != EDS_ERR_OK)
	{
		emit OnTakePictureError(ErrorType_Normal, error);
	}
}
