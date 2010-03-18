#include "CloseDialog.h"

#include <QDebug>

#include "MainWindow.h"

CloseDialog::CloseDialog(const FileList & l, QWidget *parent) :
	QDialog(parent),
	m_closeModel(l, this)
{
    ui.setupUi(this);

	ui.listView->setModel(&m_closeModel);
	ui.listView->selectAll();
}

void CloseDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui.retranslateUi(this);
        break;
    default:
        break;
    }
}

void CloseDialog::on_buttonBox_clicked(QAbstractButton * button)
{
	MainWindow * m = qobject_cast<MainWindow *>(parent());

	switch(ui.buttonBox->standardButton(button))
	{
		case QDialogButtonBox::Save:
		{
			foreach(const QModelIndex & index, ui.listView->selectionModel()->selectedIndexes())
			{
				m->save(static_cast<MyFile *>(index.internalPointer()), false);
			}

			done(QDialog::Accepted);
		} break;

		case QDialogButtonBox::SaveAll:
		{
			foreach(MyFile * f, m_closeModel.m_files)
			{
				m->save(f, false);
			}
			done(QDialog::Accepted);
		} break;

		case QDialogButtonBox::Discard:
		{
			done(QDialog::Accepted);
		} break;

		default:
		{
			done(QDialog::Rejected);
		} break;
	}
}
