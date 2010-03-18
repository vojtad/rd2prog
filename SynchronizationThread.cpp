#include "SynchronizationThread.h"

SynchronizationThread::SynchronizationThread(SerialPortInterface * spi, QWaitCondition * waitCondition)
{
	m_serialPortInterface = spi;
	m_waitCondition = waitCondition;
}

SerialPortInterface * SynchronizationThread::serialPortInterface()
{
	return m_serialPortInterface;
}

void SynchronizationThread::run()
{
	int ok = 0;
	int bad = 0;

	emit message(SYNC);

	while(ok < 12 && bad < 30)
	{
		if(m_serialPortInterface->write("U") > 0)
		{
			emit wroteBytes(".");

			if(serialPortInterface()->waitForReadyRead(100))
			{
				QByteArray byte = m_serialPortInterface->read(1);
				emit readBytes(byte);
				if(byte == "U")
				{
					++ok;
					continue;
				}
			}
		}

		++bad;
		ok = 0;
	}

	if(ok == 12)
	{
		emit message(SYNC_OK);
		myRun();
	}
	else
	{
		emit message(SYNC_FAILED);
	}
}
