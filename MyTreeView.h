#ifndef MYTREEVIEW_H
#define MYTREEVIEW_H

#include <QMenu>
#include <QTreeView>

#include "MyFile.h"

class MyTreeView : public QTreeView
{
	Q_OBJECT
	public:
		MyTreeView(QWidget *parent = 0);

		void addContextMenuAction(QAction * action);

	protected:
		void contextMenuEvent(QContextMenuEvent * event);

	private:
		QMenu m_contextMenu;
};

#endif // MYTREEVIEW_H
