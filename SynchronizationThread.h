#ifndef SYNCHRONIZATIONTHREAD_H
#define SYNCHRONIZATIONTHREAD_H

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include "SerialPortInterface.h"

enum Messages
{
	SYNC = 0,
	SYNC_OK,
	SYNC_FAILED,
	WRITING,
	WRITING_OK,
	WRITING_FAILED,
	WRITING_CANNOT_WRITE_TO_PORT,
	WRITING_CHECKSUM_ERROR,
	WRITING_SECURITY_ERROR,
	SETTING_BLJB,
	SETTING_BLJB_DONE,
};

class SynchronizationThread : public QThread
{
	Q_OBJECT

	public:
		SynchronizationThread(SerialPortInterface * spi, QWaitCondition * waitCondition);

		SerialPortInterface * serialPortInterface();

	private:
		bool m_synchronized;
		bool m_result;
		SerialPortInterface * m_serialPortInterface;
		QMutex m_mutex;
		QWaitCondition * m_waitCondition;

		virtual void myRun() = 0;

	protected:
		void run();

	signals:
		void wroteBytes(QByteArray bytes);
		void readBytes(QByteArray bytes);
		void message(int m);
		void synchronized(bool success, QString message);
		void completed(bool success, QString mesage);
		void setProgress(int p);
};

#endif // SYNCHRONIZATIONTHREAD_H
