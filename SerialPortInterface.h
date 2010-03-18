#ifndef SERIALPORTINTERFACE_H
#define SERIALPORTINTERFACE_H

#include <QIODevice>
#include <QStringList>

#if defined(Q_OS_LINUX)
#include <termios.h>

#include <QSocketNotifier>
#include <QWaitCondition>
/*#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <QSocketNotifier>*/
#elif defined(Q_OS_WIN)

#endif

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
		SerialPortInterface(SerialPortSettings settings, QObject * parent);
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
		bool waitForReadyRead(int msecs, size_t bytes);

		void setRTS(bool set);
		void setDTR(bool set);

	private:
		SerialPortSettings m_settings;
		QWaitCondition m_readWaitCond;

#if defined(Q_OS_LINUX)
		int m_fd;
		struct termios m_termios;
		QSocketNotifier * m_notifier;
#elif defined(Q_OS_WIN)
#endif

		void debugMessage(QString msg);

	private slots:
		void slotReadyRead();
};

#endif // SERIALPORTINTERFACE_H
