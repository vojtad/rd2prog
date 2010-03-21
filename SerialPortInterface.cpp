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
	Win_Handle=INVALID_HANDLE_VALUE;
		ZeroMemory(&overlap, sizeof(OVERLAPPED));
		overlap.hEvent = CreateEvent(NULL, true, false, NULL);
		winEventNotifier = 0;
		bytesToWriteLock = new QReadWriteLock;
		_bytesToWrite = 0;

	connect(this, SIGNAL(readyRead()), this, SLOT(slotReadyRead()));

	/*m_bytesToWrite = 0;
	m_handle = INVALID_HANDLE_VALUE;
	ZeroMemory(&m_overlap, sizeof(OVERLAPPED));
	m_overlap.hEvent = CreateEvent(NULL, true, false, NULL);

	m_commConfig.dwSize = sizeof(COMMCONFIG);

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
	switch(settings.m_baudrate)
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

	m_commTimeouts.ReadIntervalTimeout = MAXDWORD;
	m_commTimeouts.ReadTotalTimeoutMultiplier = 0;
	m_commTimeouts.ReadTotalTimeoutConstant = 0;
	m_commTimeouts.WriteTotalTimeoutMultiplier = 0;
	m_commTimeouts.WriteTotalTimeoutConstant = 0;*/ě
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
		Win_CommConfig.dwSize = confSize;
		DWORD dwFlagsAndAttributes = 0;
		//if (queryMode() == QextSerialPort::EventDriven)
			dwFlagsAndAttributes += FILE_FLAG_OVERLAPPED;

		QMutexLocker lock(mutex);
		if (mode == QIODevice::NotOpen)
			return isOpen();
		if (!isOpen()) {
			/*open the port*/
			Win_Handle=CreateFileA(port.toAscii(), GENERIC_READ|GENERIC_WRITE,
								  0, NULL, OPEN_EXISTING, dwFlagsAndAttributes, NULL);
			if (Win_Handle!=INVALID_HANDLE_VALUE) {
				QIODevice::open(mode);
				/*configure port settings*/
				GetCommConfig(Win_Handle, &Win_CommConfig, &confSize);
				GetCommState(Win_Handle, &(Win_CommConfig.dcb));

				/*set up parameters*/
				Win_CommConfig.dcb.fBinary=TRUE;
				Win_CommConfig.dcb.fInX=FALSE;
				Win_CommConfig.dcb.fOutX=FALSE;
				Win_CommConfig.dcb.fAbortOnError=FALSE;
				Win_CommConfig.dcb.fNull=FALSE;
				setBaudRate(Settings.BaudRate);
				setDataBits(Settings.DataBits);
				setStopBits(Settings.StopBits);
				setParity(Settings.Parity);
				setFlowControl(Settings.FlowControl);
				setTimeout(Settings.Timeout_Millisec);
				SetCommConfig(Win_Handle, &Win_CommConfig, sizeof(COMMCONFIG));

				//init event driven approach
				//if (queryMode() == QextSerialPort::EventDriven) {
					Win_CommTimeouts.ReadIntervalTimeout = MAXDWORD;
					Win_CommTimeouts.ReadTotalTimeoutMultiplier = 0;
					Win_CommTimeouts.ReadTotalTimeoutConstant = 0;
					Win_CommTimeouts.WriteTotalTimeoutMultiplier = 0;
					Win_CommTimeouts.WriteTotalTimeoutConstant = 0;
					SetCommTimeouts(Win_Handle, &Win_CommTimeouts);
					if (!SetCommMask( Win_Handle, EV_TXEMPTY | EV_RXCHAR | EV_DSR)) {
						qWarning() << "failed to set Comm Mask. Error code:", GetLastError();
						return false;
					//}
					winEventNotifier = new QWinEventNotifier(overlap.hEvent, this);
					connect(winEventNotifier, SIGNAL(activated(HANDLE)), this, SLOT(onWinEvent(HANDLE)));
					WaitCommEvent(Win_Handle, &eventMask, &overlap);
				}
			}
		} else {
			return false;
		}
		return isOpen();
}

void SerialPortInterface::close()
{
	QMutexLocker lock(mutex);
		if (isOpen()) {
			FlushFileBuffers(Win_Handle);
			QIODevice::close(); // mark ourselves as closed
			CancelIo(Win_Handle);
			if (CloseHandle(Win_Handle))
				Win_Handle = INVALID_HANDLE_VALUE;
			if (winEventNotifier)
				winEventNotifier->deleteLater();

			_bytesToWrite = 0;

			foreach(OVERLAPPED* o, pendingWrites) {
				CloseHandle(o->hEvent);
				delete o;
			}
			pendingWrites.clear();
		}
}

qint64 SerialPortInterface::size() const
{
	int availBytes;
		COMSTAT Win_ComStat;
		DWORD Win_ErrorMask=0;
		ClearCommError(Win_Handle, &Win_ErrorMask, &Win_ComStat);
		availBytes = Win_ComStat.cbInQue;
		return (qint64)availBytes;
}

qint64 SerialPortInterface::bytesAvailable() const
{
	QMutexLocker lock(mutex);
	if (isOpen()) {
		DWORD Errors;
		COMSTAT Status;
		if (ClearCommError(Win_Handle, &Errors, &Status)) {
			return Status.cbInQue + QIODevice::bytesAvailable();
		}
		return (qint64)-1;
	}
	return 0;
}

void SerialPortInterface::setRTS(bool set)
{
	QMutexLocker lock(mutex);
	if (isOpen())
	{
		EscapeCommFunction(m_handle, set ? SETRTS : CLRRTS);
	}
}

void SerialPortInterface::setDTR(bool set)
{
	QMutexLocker lock(mutex);
	if (isOpen())
	{
		EscapeCommFunction(m_handle, set ? SETDTR : CLRDTR);
	}
}

qint64 SerialPortInterface::readData(char * data, qint64 maxSize)
{
	DWORD retVal;
	QMutexLocker lock(mutex);
	retVal = 0;
		OVERLAPPED overlapRead;
		ZeroMemory(&overlapRead, sizeof(OVERLAPPED));
		if (!ReadFile(Win_Handle, (void*)data, (DWORD)maxSize, & retVal, & overlapRead)) {
			if (GetLastError() == ERROR_IO_PENDING)
				GetOverlappedResult(Win_Handle, & overlapRead, & retVal, true);
			else {
				lastErr = E_READ_FAILED;
				retVal = (DWORD)-1;
			}
		}
	return (qint64)retVal;
}

qint64 SerialPortInterface::writeData(const char * data, qint64 maxSize)
{
	QMutexLocker lock( mutex );
	DWORD retVal = 0;
		OVERLAPPED* newOverlapWrite = new OVERLAPPED;
		ZeroMemory(newOverlapWrite, sizeof(OVERLAPPED));
		newOverlapWrite->hEvent = CreateEvent(NULL, true, false, NULL);
		if (WriteFile(Win_Handle, (void*)data, (DWORD)maxSize, & retVal, newOverlapWrite)) {
			CloseHandle(newOverlapWrite->hEvent);
			delete newOverlapWrite;
		}
		else if (GetLastError() == ERROR_IO_PENDING) {
			// writing asynchronously...not an error
			QWriteLocker writelocker(bytesToWriteLock);
			_bytesToWrite += maxSize;
			pendingWrites.append(newOverlapWrite);
		}
		else {
			qDebug() << "serialport write error:" << GetLastError();
			lastErr = E_WRITE_FAILED;
			retVal = (DWORD)-1;
			if(!CancelIo(newOverlapWrite->hEvent))
				qDebug() << "serialport: couldn't cancel IO";
			if(!CloseHandle(newOverlapWrite->hEvent))
				qDebug() << "serialport: couldn't close OVERLAPPED handle";
			delete newOverlapWrite;
		}
	return (qint64)retVal;
}

void SerialPortInterface::onActivated(HANDLE h)
{
	if(h == m_overlap.hEvent)
	{
		if(m_eventMask & EV_RXCHAR)
		{
			if (sender() != this && bytesAvailable() > 0)
				emit readyRead();
		}

		if(m_eventMask & EV_TXEMPTY)
		{
			/*
			 * A write completed.  Run through the list of OVERLAPPED writes, and if
			 * they completed successfully, take them off the list and delete them.
			 * Otherwise, leave them on there so they can finish.
			 */
			qint64 totalBytesWritten = 0;
			QList<OVERLAPPED *> overlapsToDelete;
			foreach(OVERLAPPED * o, m_pendingWrites)
			{
				DWORD numBytes = 0;
				if(GetOverlappedResult(m_handle, o, &numBytes, false))
				{
					overlapsToDelete.append(o);
					totalBytesWritten += numBytes;
				}
				else if(GetLastError() != ERROR_IO_INCOMPLETE)
				{
					overlapsToDelete.append(o);
					qWarning() << "CommEvent overlapped write error:" << GetLastError();
				}
			}

			if (sender() != this && totalBytesWritten > 0)
			{
				//QWriteLocker writelocker(bytesToWriteLock);
				emit bytesWritten(totalBytesWritten);
				m_bytesToWrite = 0;
			}

			while(!overlapsToDelete.empty())
			{
				OVERLAPPED * o = overlapsToDelete.takeLast();
				CloseHandle(o->hEvent);
				delete o;
			}
		}

		WaitCommEvent(m_handle, &m_eventMask, &m_overlap);
	}
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
//	usleep(1000);

	QMutex mutex;
	QMutexLocker locker(&mutex);

	if(bytesAvailable() == 0 && !m_readWaitCond.wait(&mutex, msecs))
	{
		return false;
	}

	return true;
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
