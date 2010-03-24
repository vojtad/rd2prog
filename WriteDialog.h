#ifndef WRITEDIALOG_H
#define WRITEDIALOG_H

#include <QListWidgetItem>

#include "ui_WriteDialog.h"

#include "SynchronizationThread.h"

class WriteThread : public SynchronizationThread
{
	public:
		WriteThread(SerialPortInterface * spi, const QList<QByteArray> & data);

		bool m_setBSB;

	private:
		int m_sent;
		QList<QByteArray> m_data;

		void myRun();
		int writeByteArray(const QByteArray & array); // -1 - send again, 0 - continue, 1 - abort
};

class WriteDialog : public QDialog
{
	Q_OBJECT
	public:
		WriteDialog(const SerialPortSettings & settings, const QList<QByteArray> & data, QWidget * parent);
		~WriteDialog();

	protected:
		void changeEvent(QEvent *e);

	private:
		Ui::WriteDialog ui;

		SerialPortInterface m_serialPortInterface;
		WriteThread m_thread;
		QListWidgetItem * m_currentItem;

	private slots:
		void on_pushButtonClose_clicked();
		void on_actionEnter_triggered();

		void onMessage(int m);
		void printBytes(const QByteArray & bytes);
		void syncOk();
		void syncFailed();
		void writingOk();
		void writingFailed();
		void aborted();

	public slots:
		void done(int r);
};

#endif // WRITEDIALOG_H
