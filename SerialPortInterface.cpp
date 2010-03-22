#include "SerialPortInterface.h"

#include <QDebug>
#include <QMutex>

#if defined(Q_OS_LINUX)

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/select.h>

extern "C"
{
#include <libudev.h>
}

SerialPortInterface::SerialPortInterface(const SerialPortSettings & settings, QObject * parent) :
		QIODevice(parent),
		m_settings(settings)
{
	m_fd = -1;

	connect(this, SIGNAL(readyRead()), this, SLOT(slotReadyRead()));

	int B = 0;
	switch(settings.m_baudrate)
	{
		case 110:
			B = B110; break;
		case 300:
			B = B300; break;
		case 600:
			B = B600; break;
		case 1200:
			B = B1200; break;
		case 2400:
			B = B2400; break;
		case 4800:
			B = B4800; break;
		case 9600:
			B = B9600; break;
		case 19200:
			B = B19200; break;
		case 38400:
			B = B38400; break;
		case 57600:
			B = B57600; break;
		case 115200:
			B = B115200; break;
		default: // toto by se nemelo stat
			B = B9600; break;
	}

#ifdef CBAUD
	m_termios.c_cflag &= ~CBAUD;
	m_termios.c_cflag |= B;
#else
	cfsetispeed(&m_termios, B);
	cfsetospeed(&m_termios, B);
#endif

	// 8 data bits
	m_termios.c_cflag &= ~CSIZE;
	m_termios.c_cflag |= CS8;

	// no parity
	m_termios.c_cflag &= ~PARENB;

	// 2 stop bits
	m_termios.c_cflag |= CSTOPB;

	// no flow control
	m_termios.c_cflag &= ~CRTSCTS;
	m_termios.c_iflag &= ~(IXON | IXOFF | IXANY);
}

QStringList SerialPortInterface::getPorts()
{
	QStringList ret;
	struct udev * udev = udev_new();

	if(udev == NULL)
	{
		qCritical() << "udev_new() failed";
		return ret;
	}

	for(int minor = 64; minor <= 255; ++minor)
	{
		struct udev_device * udev_device = udev_device_new_from_devnum(udev, 'c', makedev(4, minor));
		if(udev_device != NULL)
		{
			ret.append(udev_device_get_devnode(udev_device));
			udev_device_unref(udev_device);
		}
		else
			break; // mozna by se melo pokracovat ve vyhledavani..
	}

	udev_unref(udev);

	return ret;
}

bool SerialPortInterface::open(OpenMode mode)
{
	if(mode == QIODevice::NotOpen)
		return isOpen();

	if(!isOpen())
	{
		m_fd = ::open(m_settings.m_name.toAscii(), O_RDWR | O_NOCTTY | O_NDELAY);
		if(m_fd != -1)
		{

			setOpenMode(mode);
			cfmakeraw(&m_termios);
			m_termios.c_cflag |= CREAD | CLOCAL;
			m_termios.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK| ECHONL | ISIG);
			m_termios.c_iflag &= ~(INPCK | IGNPAR | PARMRK | ISTRIP | ICRNL | IXANY);
			m_termios.c_oflag &= ~OPOST;
			m_termios.c_cc[VMIN]= 0;

			tcsetattr(m_fd, TCSAFLUSH, &m_termios);

			m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
			connect(m_notifier, SIGNAL(activated(int)), this, SIGNAL(readyRead()));
		}
		else
		{
			setErrorString(QString(tr("Cannot open port %1.")).arg(m_settings.m_name));
		}
	}

	return isOpen();
}

void SerialPortInterface::close()
{
	if(isOpen())
	{
		tcflush(m_fd, TCIOFLUSH);
		::close(m_fd);
		m_notifier->deleteLater();
		QIODevice::close();
	}
}

qint64 SerialPortInterface::size() const
{
	qint64 i;
	if(ioctl(m_fd, FIONREAD, &i) < 0)
		return 0;

	return i;
}

qint64 SerialPortInterface::bytesAvailable() const
{
	if(isOpen())
	{
		qint64 i = 0;
		if (ioctl(m_fd, FIONREAD, &i) == -1)
		{
			return qint64(-1);
		}

		return i + QIODevice::bytesAvailable();
	}

	return 0;
}

void SerialPortInterface::setRTS(bool set)
{
	if(isOpen())
	{
		int rts = TIOCM_RTS;
		ioctl(m_fd, set ? TIOCMBIS : TIOCMBIC, &rts);
	}
}

void SerialPortInterface::setDTR(bool set)
{
	if(isOpen())
	{
		int dtr = TIOCM_DTR;
		ioctl(m_fd, set ? TIOCMBIS : TIOCMBIC, &dtr);
	}
}

qint64 SerialPortInterface::readData(char * data, qint64 maxSize)
{
	return ::read(m_fd, data, maxSize);
}

qint64 SerialPortInterface::writeData(const char * data, qint64 maxSize)
{
	return ::write(m_fd, data, maxSize);
}

#elif defined(Q_OS_WIN)

#include <objbase.h>
#include <initguid.h>
#include <setupapi.h>
#include <ddk/ntddser.h>

SerialPortInterface::SerialPortInterface(const SerialPortSettings & settings, QObject * parent) :
		QIODevice(parent),
		m_settings(settings)
{
	m_handle = INVALID_HANDLE_VALUE;

	//connect(this, SIGNAL(readyRead()), this, SLOT(slotReadyRead()));
	//connect(this, SIGNAL(bytesWritten(qint64)), this, SLOT(slotBytesWritten(qint64)));
}

QStringList SerialPortInterface::getPorts()
{
	QStringList ret;
	QRegExp regexp("(COM\\d+)");
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA DeviceInfoData;
	DWORD i;

	// Create a HDEVINFO with all present devices.
	hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_COMPORT, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		// Insert error handling here.
		return ret;
	}

	// Enumerate through all devices in Set.

	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	for(i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); ++i)
	{
		DWORD DataT;
		LPTSTR buffer = NULL;
		DWORD buffersize = 0;

		//
		// Call function with null to begin with,
		// then use the returned buffer size (doubled)
		// to Alloc the buffer. Keep calling until
		// success or an unknown failure.
		//
		//  Double the returned buffersize to correct
		//  for underlying legacy CM functions that
		//  return an incorrect buffersize value on
		//  DBCS/MBCS systems.
		//
		while(!SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData, SPDRP_FRIENDLYNAME, &DataT, (PBYTE)buffer,
												buffersize, &buffersize))
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				// Change the buffer size.
				if(buffer)
					LocalFree(buffer);
				// Double the size to avoid problems on
				// W2k MBCS systems per KB 888609.
				buffer = (TCHAR *)LocalAlloc(LPTR, buffersize * 2);
			}
			else
			{
				// Insert error handling here.
				break;
			}
		}

		if(QString::fromUtf16((ushort *)(buffer)).contains(regexp))
		{
			ret.append(regexp.cap(1));
		}

		if(buffer)
			LocalFree(buffer);
	}


	if(GetLastError() != NO_ERROR && GetLastError() != ERROR_NO_MORE_ITEMS)
	{
		// Insert error handling here.
		return ret;
	}

	//  Cleanup
	SetupDiDestroyDeviceInfoList(hDevInfo);

	return ret;
}

bool SerialPortInterface::open(OpenMode mode)
{
	unsigned long confSize = sizeof(COMMCONFIG);
	m_commConfig.dwSize = confSize;

	if (mode == QIODevice::NotOpen)
		return isOpen();

	if (!isOpen())
	{
		/*open the port*/
		QString port = m_settings.m_name;
		QRegExp regexp("^COM(\\d+)");
		if(port.contains(regexp) && regexp.cap(1).toInt() > 9)
			port.prepend("\\\\.\\");

		m_handle = CreateFileA(port.toAscii(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, NULL, NULL);
		if(m_handle != INVALID_HANDLE_VALUE)
		{
			qDebug() << port;
			QIODevice::open(mode);

			/*configure port settings*/
			GetCommConfig(m_handle, &m_commConfig, &confSize);
			GetCommState(m_handle, &(m_commConfig.dcb));

			/*set up parameters*/
			m_commConfig.dcb.fBinary = TRUE;
			m_commConfig.dcb.fInX = FALSE;
			m_commConfig.dcb.fOutX = FALSE;
			m_commConfig.dcb.fAbortOnError = FALSE;
			m_commConfig.dcb.fNull = FALSE;

			// no parity
			m_commConfig.dcb.Parity = 0;
			m_commConfig.dcb.fParity = FALSE;

			// 8 data bits
			m_commConfig.dcb.ByteSize = 8;

			// 2 stop bits
			m_commConfig.dcb.StopBits = 2;

			// baudrate
			switch(m_settings.m_baudrate)
			{
				case 110:
					m_commConfig.dcb.BaudRate = CBR_110; break;
				case 300:
					m_commConfig.dcb.BaudRate = CBR_300; break;
				case 600:
					m_commConfig.dcb.BaudRate = CBR_600; break;
				case 1200:
					m_commConfig.dcb.BaudRate = CBR_1200; break;
				case 2400:
					m_commConfig.dcb.BaudRate = CBR_2400; break;
				case 4800:
					m_commConfig.dcb.BaudRate = CBR_4800; break;
				case 9600:
					m_commConfig.dcb.BaudRate = CBR_9600; break;
				case 19200:
					m_commConfig.dcb.BaudRate = CBR_19200; break;
				case 38400:
					m_commConfig.dcb.BaudRate = CBR_38400; break;
				case 57600:
					m_commConfig.dcb.BaudRate = CBR_57600; break;
				case 115200:
					m_commConfig.dcb.BaudRate = CBR_115200; break;
				default: // toto by se nemelo stat
					m_commConfig.dcb.BaudRate = CBR_9600; break;
			}

			// no flow control
			m_commConfig.dcb.fOutxCtsFlow = FALSE;
			m_commConfig.dcb.fRtsControl = RTS_CONTROL_DISABLE;
			m_commConfig.dcb.fInX = FALSE;
			m_commConfig.dcb.fOutX = FALSE;

			SetCommConfig(m_handle, &m_commConfig, sizeof(COMMCONFIG));
		}
		else
		{
			return false;
		}
	}

	return isOpen();
}

void SerialPortInterface::close()
{
	if (isOpen())
	{
		FlushFileBuffers(m_handle);
		QIODevice::close(); // mark ourselves as closed
		CancelIo(m_handle);

		if(CloseHandle(m_handle))
			m_handle = INVALID_HANDLE_VALUE;
	}
}

qint64 SerialPortInterface::size() const
{
	COMSTAT Win_ComStat;
	DWORD Win_ErrorMask = 0;
	ClearCommError(m_handle, &Win_ErrorMask, &Win_ComStat);

	return (qint64)Win_ComStat.cbInQue;
}

qint64 SerialPortInterface::bytesAvailable() const
{
	if (isOpen())
	{
		COMSTAT Win_ComStat;
		DWORD Win_ErrorMask = 0;
		if(ClearCommError(m_handle, &Win_ErrorMask, &Win_ComStat))
			return Win_ComStat.cbInQue + QIODevice::bytesAvailable();

		return (qint64)-1;
	}

	return 0;
}

void SerialPortInterface::setRTS(bool set)
{
	if (isOpen())
	{
		EscapeCommFunction(m_handle, set ? SETRTS : CLRRTS);
	}
}

void SerialPortInterface::setDTR(bool set)
{
	if (isOpen())
	{
		EscapeCommFunction(m_handle, set ? SETDTR : CLRDTR);
	}
}

qint64 SerialPortInterface::readData(char * data, qint64 maxSize)
{
	DWORD retVal = 0;
	ReadFile(m_handle, (void*)data, (DWORD)maxSize, &retVal, NULL);
	return qint64(retVal);
}

qint64 SerialPortInterface::writeData(const char * data, qint64 maxSize)
{
	DWORD retVal = 0;
	WriteFile(m_handle, (void*)data, (DWORD)maxSize, &retVal, NULL);
	return qint64(retVal);
}

#else

QStringList SerialPortInterface::getPorts()
{
	qCritical() << "Serial port communication is not implemented on this OS.";
}

#endif

bool SerialPortInterface::isSequential() const
{
	return true;
}

void SerialPortInterface::debugMessage(const QString & msg) const
{
	qDebug() << m_settings.m_name << ": " << msg;
}

bool SerialPortInterface::waitForReadyRead(int msecs)
{

	QMutex mutex;

#if defined(Q_OS_LINUX)
	QMutexLocker locker(&mutex);

	if(bytesAvailable() == 0 && !m_readWaitCond.wait(&mutex, msecs))
	{
		return false;
	}

	return true;
#elif defined(Q_OS_WIN32)
	for(int i = 0; i < msecs; ++i)
	{
		if(bytesAvailable() > 0)
			return true;

		QMutexLocker locker(&mutex);
		m_readWaitCond.wait(&mutex, 1);
	}

	return false;
#endif
}

bool SerialPortInterface::waitForReadyRead(int msecs, int bytes)
{
	while(bytesAvailable() < bytes)
	{
		if(!waitForReadyRead(msecs))
			return false;
	}

	return true;
}

void SerialPortInterface::slotReadyRead()
{
	m_readWaitCond.wakeAll();
}

SerialPortInterface::~SerialPortInterface()
{
	if(isOpen())
	{
		debugMessage("deleting unclosed SerialPortInterface");
		close();
	}
}
