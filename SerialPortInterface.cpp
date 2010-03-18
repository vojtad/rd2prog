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

SerialPortInterface::SerialPortInterface(SerialPortSettings settings, QObject * parent) :
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
		setOpenMode(0);
		tcflush(m_fd, TCIOFLUSH);
		::close(m_fd);
		delete m_notifier;
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
	#include <dbt.h>

// http://msdn.microsoft.com/en-us/library/ms791134.aspx
#ifndef GUID_DEVCLASS_PORTS
	DEFINE_GUID(GUID_DEVCLASS_PORTS, 0x4D36E978, 0xE325, 0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18 );
#endif

#include <Windows.h>

SerialPortInterface::SerialPortInterface(SerialPortSettings settings, QObject * parent) :
		QIODevice(parent)
{
}

QStringList SerialPortInterface::getPorts()
{
	QStringList ret;
	GUID * guid = (GUID *)(&GUID_DEVCLASS_PORTS);

	HDEVINFO devInfo = SetupDiGetClassDevs(guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if(devInfo != INVALID_HANDLE_VALUE)
	{
		size_t detDataSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) + 256;

		SP_DEVINFO_DATA devInfoData = { sizeof(SP_DEVINFO_DATA) };
		SP_DEVICE_INTERFACE_DATA intData;
                SP_DEVICE_INTERFACE_DETAIL_DATA * detData = (SP_DEVICE_INTERFACE_DETAIL_DATA *) new char[detDataSize];

		intData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		detData->cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

                for(int i = 0; SetupDiGetDeviceInterfaceDetail(devInfo, &intData, detData, detDataSize, NULL, &devInfoData); ++i)
		{
                        DWORD bufSize;
                        SetupDiGetDeviceRegistryProperty(devInfo, &devInfoData, SPDRP_FRIENDLYNAME, NULL, NULL, 0, &bufSize);
                        BYTE buff[bufSize];
                        SetupDiGetDeviceRegistryProperty(devInfo, &devInfoData, SPDRP_FRIENDLYNAME, NULL, buff, bufSize, NULL);
			ret.append(QString::fromUtf16((ushort*)(buff)));
		}

		SetupDiDestroyDeviceInfoList(devInfo);
	}
	/*

	DWORD cbNeeded, dwPorts;
	EnumPorts(NULL, 1, NULL, 0, &cbNeeded, &dwPorts);

	BYTE data[cbNeeded];
	if(EnumPorts(NULL, 1, data, cbNeeded, &cbNeeded, &dwPorts))
	{
		QRegExp rx("(COM\\d+).*");
		PORT_INFO_1 * portInfo = reinterpret_cast<PORT_INFO_1 *>(data);
		for(DWORD i = 0; i < dwPorts; ++i, ++portInfo)
		{

			QString port = QString::fromWCharArray(portInfo->pName);
			if(rx.indexIn(port, 0) != -1)
				ret.append(rx.cap(1));
		}
	}*/

	return ret;
}

bool SerialPortInterface::open(OpenMode mode)
{
       return isOpen();
}

void SerialPortInterface::close()
{
}

qint64 SerialPortInterface::size() const
{
        return 0;
}

qint64 SerialPortInterface::bytesAvailable() const
{
        return 0;
}

void SerialPortInterface::setRTS(bool set)
{
}

void SerialPortInterface::setDTR(bool set)
{
}

qint64 SerialPortInterface::readData(char * data, qint64 maxSize)
{
        return 0;
}

qint64 SerialPortInterface::writeData(const char * data, qint64 maxSize)
{
        return 0;
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

void SerialPortInterface::debugMessage(QString msg)
{
	qDebug() << m_settings.m_name << ": " << msg;
}

bool SerialPortInterface::waitForReadyRead(int msecs)
{
	usleep(1000);

	QMutex mutex;

	if(bytesAvailable() == 0 && !m_readWaitCond.wait(&mutex, msecs))
	{
		mutex.unlock();
		return false;
	}

	mutex.unlock();
	return true;
}

bool SerialPortInterface::waitForReadyRead(int msecs, size_t bytes)
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
