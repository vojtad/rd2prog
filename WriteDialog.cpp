#include "WriteDialog.h"

#include "ISPFrame.h"

#include <QListWidgetItem>
#include <QScrollBar>
#include <QDebug>

WriteThread::WriteThread(SerialPortInterface * spi, const QList<QByteArray> & data) :
		SynchronizationThread(spi),
		m_setBSB(false),
		m_data(data)
{
}

void WriteThread::myRun()
{
	emit message(WRITING);

	m_sent = 0;

	foreach(QByteArray ar, m_data)
	{

		int bad = 0;
		while(bad < 9)
		{
			int ret = writeByteArray(ar);
			if(ret == 0)
				break;
			else if(ret > 0)
			{
				emit message(WRITING_FAILED);
				return;
			}

			++bad;
		}

		if(bad >= 9)
		{
			emit message(WRITING_FAILED);
			return;
		}
	}

	if(m_setBSB)
	{
		emit message(SETTING_BSB);

		ISP::ISPFrame frame(ISP::WriteFunction, ISP::WriteFunctionArgument::ProgramBSB);
		frame.appendDataByte(0);

		int bad = 0;
		while(bad < 9)
		{
			int ret = writeByteArray(frame.data());
			if(ret == 0)
				break;
			else if(ret > 0)
				return;

			++bad;
		}

		if(bad >= 9)
		{
			emit message(WRITING_FAILED);
			return;
		}
		else
			emit message(SETTING_BSB_DONE);
	}

	emit message(WRITING_OK);
}

int WriteThread::writeByteArray(const QByteArray & array)
{
	int ret = serialPortInterface()->write(array);

	m_sent += ret;
	emit setProgress(m_sent);

	if(serialPortInterface()->waitForReadyRead(10000, ret))
	{
		emit readBytes(serialPortInterface()->read(ret));

		QByteArray buf;
		while(buf.right(2) != "\r\n" && serialPortInterface()->waitForReadyRead(10000))
		{
			buf.append(serialPortInterface()->read(1));
		}

		if(buf.right(2) == "\r\n")
		{
			emit readBytes(buf.left(buf.size() - 2));
			switch(buf.at(buf.size() - 3))
			{
				case 'X':
				{
					emit message(WRITING_CHECKSUM_ERROR);
					return -1;
				}

				case 'P':
				case 'L':
				{
					emit message(WRITING_SECURITY_ERROR);
					return 1;
				}

				default:
					return 0;
			}
		}
		else
			emit readBytes(buf);
	}

	return 1;
}

WriteDialog::WriteDialog(const SerialPortSettings & settings, const QList<QByteArray> & data, QWidget * parent) :
	QDialog(parent),
	m_serialPortInterface(settings, this),
	m_thread(&m_serialPortInterface, data)
{
	ui.setupUi(this);

	connect(&m_thread, SIGNAL(readBytes(QByteArray)), this, SLOT(printBytes(QByteArray)));
	connect(&m_thread, SIGNAL(wroteBytes(QByteArray)), this, SLOT(printBytes(QByteArray)));
	connect(&m_thread, SIGNAL(message(int)), this, SLOT(onMessage(int)));
	connect(&m_thread, SIGNAL(setProgress(int)), ui.progressBar, SLOT(setValue(int)));
	connect(&m_thread, SIGNAL(terminated()), this, SLOT(aborted()));

	if(!m_serialPortInterface.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
	{
		done(0);
		return;
	}

	m_serialPortInterface.setRTS(true);

	int total = 0;
	foreach(QByteArray ar, data)
		total += ar.size();

	ui.progressBar->setMaximum(total != 0 ? total : 1);

	ui.listWidget->addItem(tr("Reset microprocessor and press Enter."));
}

WriteDialog::~WriteDialog()
{
}

void WriteDialog::changeEvent(QEvent *e)
{
	QDialog::changeEvent(e);
	switch (e->type())
	{
	case QEvent::LanguageChange:
		ui.retranslateUi(this);
		break;
	default:
		break;
	}
}

void WriteDialog::on_actionEnter_triggered()
{
	if(!m_thread.isRunning())
	{
		QFont font(ui.labelSync->font());
		font.setBold(true);
		ui.labelSync->setFont(font);
		font.setBold(false);
		ui.labelWriting->setFont(font);

		ui.labelSyncIcon->setPixmap(QPixmap());
		ui.labelWritingIcon->setPixmap(QPixmap());

		ui.labelSyncStatus->setText(QString());
		ui.labelWritingStatus->setText(QString());

		ui.checkBoxBSB->setEnabled(false);

		ui.progressBar->setValue(0);

		ui.pushButton->setText(tr("Abort"));
		ui.pushButtonClose->setEnabled(false);

		m_thread.m_setBSB = ui.checkBoxBSB->isChecked();
		m_thread.start();
	}
	else
	{
		m_thread.terminate();
	}
}

void WriteDialog::onMessage(int m)
{
	switch(m)
	{
		case SYNC:
		{
			ui.listWidget->addItem(tr("Synchronization of microprocessor and PC:"));
			m_currentItem = new QListWidgetItem(ui.listWidget);
		} break;
		case SYNC_FAILED:
		{
			ui.listWidget->addItem(tr("Synchronization failed."));
			ui.listWidget->addItem(tr("Click on Retry to write again."));
			syncFailed();
		} break;
		case SYNC_OK:
		{
			syncOk();
		} break;
		case WRITING:
		{
			ui.listWidget->addItem(tr("Writing:"));
		} break;
		case WRITING_FAILED:
		{
			ui.listWidget->addItem(tr("Writing failed."));
			ui.listWidget->addItem(tr("Click on Retry to write again."));
			writingFailed();
		} break;
		case WRITING_SECURITY_ERROR:
		{
			ui.listWidget->addItem(tr("Security error."));
			writingFailed();
		} break;
		case WRITING_CANNOT_WRITE_TO_PORT:
		{
			ui.listWidget->addItem(tr("Cannot write to serial port."));
			ui.listWidget->addItem(tr("Click on Retry to write again."));
			writingFailed();
		} break;
		case WRITING_CHECKSUM_ERROR:
		{
			ui.listWidget->addItem(tr("Checksum error. Sending again."));
		} break;
		case WRITING_OK:
		{
			ui.listWidget->addItem(tr("Writing done."));
			ui.listWidget->addItem(tr("Reset microprocessor to run your program."));
			writingOk();
		} break;
		case SETTING_BSB:
		{
			ui.listWidget->addItem(tr("Writing BSB byte:"));
		} break;
		case SETTING_BSB_DONE:
		{
			ui.listWidget->addItem(tr("BSB byte was successfully set to 00h."));
		} break;
		default:
		{
			qDebug("WriteDialog: unknown message %d", m);
		} break;
	}

	ui.listWidget->scrollToBottom();
}

void WriteDialog::syncFailed()
{
	QFont font(ui.labelSync->font());
	font.setBold(false);
	ui.labelSync->setFont(font);

	ui.labelSyncStatus->setText(tr("failed."));
	ui.labelSyncIcon->setPixmap(QPixmap(":/icons/icon-cancel.png"));

	ui.pushButton->setText(tr("Retry"));
	ui.pushButtonClose->setEnabled(true);
}

void WriteDialog::syncOk()
{
	QFont font(ui.labelSync->font());
	font.setBold(false);
	ui.labelSync->setFont(font);

	ui.labelSyncStatus->setText(tr("done."));
	ui.labelSyncIcon->setPixmap(QPixmap(":/icons/icon-ok.png"));

	font.setBold(true);
	ui.labelWriting->setFont(font);
}

void WriteDialog::writingFailed()
{
	QFont font(ui.labelWriting->font());
	font.setBold(false);
	ui.labelWriting->setFont(font);

	ui.labelWritingStatus->setText(tr("failed."));
	ui.labelWritingIcon->setPixmap(QPixmap(":/icons/icon-cancel.png"));

	ui.pushButton->setText(tr("Retry"));
	ui.pushButtonClose->setEnabled(true);
}

void WriteDialog::writingOk()
{
	m_serialPortInterface.setRTS(false);

	QFont font(ui.labelWriting->font());
	font.setBold(false);
	ui.labelWriting->setFont(font);

	ui.progressBar->setVisible(false);

	ui.labelWritingStatus->setText(tr("done."));
	ui.labelWritingIcon->setPixmap(QPixmap(":/icons/icon-ok.png"));

	ui.actionEnter->setEnabled(false);
	ui.pushButton->setEnabled(false);
	ui.pushButton->setText(tr("Done"));
	ui.pushButtonClose->setEnabled(true);
}

void WriteDialog::aborted()
{
	ui.listWidget->addItem(tr("Aborted."));
	ui.listWidget->addItem(tr("Click on Retry to write again."));
	ui.listWidget->scrollToBottom();

	QFont font(ui.labelSync->font());
	font.setBold(false);
	ui.labelSync->setFont(font);
	ui.labelWriting->setFont(font);

	ui.labelSyncIcon->setPixmap(QPixmap());
	ui.labelWritingIcon->setPixmap(QPixmap());

	ui.labelSyncStatus->setText(QString());
	ui.labelWritingStatus->setText(QString());

	ui.checkBoxBSB->setEnabled(true);

	ui.progressBar->setValue(0);

	ui.pushButton->setText(tr("Retry"));
	ui.pushButtonClose->setEnabled(true);
}

void WriteDialog::printBytes(const QByteArray & bytes)
{
	if(!bytes.isEmpty())
	{
		if(bytes.at(0) == ':')
			m_currentItem = new QListWidgetItem(ui.listWidget);

		m_currentItem->setText(m_currentItem->text().append(bytes));
		ui.listWidget->scrollToBottom();
	}
}

void WriteDialog::done(int r)
{
	if(m_thread.isRunning())
	{
		m_thread.terminate();
	}

	if(m_serialPortInterface.isOpen())
	{
		m_serialPortInterface.setRTS(false);
		m_serialPortInterface.close();
	}

	QDialog::done(r);
}

void WriteDialog::on_pushButtonClose_clicked()
{
	done(0);
}
