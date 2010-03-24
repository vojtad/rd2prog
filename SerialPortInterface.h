#ifndef SERIALPORTINTERFACE_H
#define SERIALPORTINTERFACE_H

#include <QIODevice>
#include <QStringList>

#if defined(Q_OS_LINUX)
#include <termios.h>
#include <QSocketNotifier>
#elif defined(Q_OS_WIN)
#include <windows.h>
#endif

#include <QMutex>
#include <QWaitCondition>

struct SerialPortSettings
{
	QString m_name;
	int m_baudrate;
};

/*
 * Data bits: 8
 * Parity: none
 * Stop bits: 2
 * Flow control: none
 */

class SerialPortInterface : public QIODevice
{
	Q_OBJECT

	public:
		SerialPortInterface(const SerialPortSettings & settings, QObject * parent);
		~SerialPortInterface();

		static QStringList getPorts();

		bool isSequential() const;

		bool open(OpenMode mode);
		void close();

		qint64 size() const;
		qint64 bytesAvailable() const;

		qint64 readData(char * data, qint64 maxSize);
		qint64 writeData(const char * data, qint64 maxSize);

		bool waitForReadyRead(int msecs);
		bool waitForReadyRead(int msecs, int bytes);

		void setRTS(bool set);
		void setDTR(bool set);

	private:
		SerialPortSettings m_settings;
		bool m_forceClose;

		void debugMessage(const QString & msg) const;

#if defined(Q_OS_LINUX)
		int m_fd;
		struct termios m_termios;
#elif (defined Q_OS_WIN)
		HANDLE m_handle;
		COMMCONFIG m_commConfig;
#endif
};

#endif // SERIALPORTINTERFACE_H
