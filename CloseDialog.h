#ifndef CLOSEDIALOG_H
#define CLOSEDIALOG_H

#include "ui_CloseDialog.h"

#include "MyFile.h"
#include "MyCloseModel.h"

class CloseDialog : public QDialog
{
    Q_OBJECT
	public:
		CloseDialog(const FileList & l, QWidget * parent = 0);

	protected:
		void changeEvent(QEvent *e);

	private:
		Ui::CloseDialog ui;
		MyCloseModel m_closeModel;

private slots:
	void on_buttonBox_clicked(QAbstractButton* button);
};

#endif // CLOSEDIALOG_H
