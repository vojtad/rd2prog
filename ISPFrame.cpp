#include "ISPFrame.h"

using namespace ISP;

ISPFrame::ISPFrame(qint8 recordType, qint32 argument) :
		m_loadOffset(0),
		m_recordType(recordType),
		m_checksum(0),
		m_argument(argument)
{
}

ISPFrame::~ISPFrame()
{
}

void ISPFrame::setArgument(qint32 argument)
{
	m_argument = argument;
}

void ISPFrame::appendDataByte(quint8 byte)
{
	m_data.append(byte);
}

#include <QDebug>

QByteArray ISPFrame::data()
{
	QByteArray ret, ar;

	if(m_argument > -1)
	{
		ar.append(m_argument & 0xFF);
		if(m_argument > 0xFF)
			ar.prepend((m_argument & 0xFF00) >> 8);
	}

	//ret.append(':'); // Record Mark
	ret.append(quint8(ar.size() + m_data.size())); // Reclen
	ret.append(char(0)); // Load offset 1st byte
	ret.append(char(0)); // Load offset 2nd byte
	ret.append(m_recordType);
	ret.append(ar);
	ret.append(m_data);

	quint8 crc = 0;
	for(int i = 0; i < ret.size(); ++i)
		crc += quint8(ret.at(i));

	return ret.append(~crc + 1).toHex().toUpper().prepend(':');
}

ISPFrame & operator<<(ISPFrame & isp, quint8 byte)
{
	isp.appendDataByte(byte);

	return isp;
}
