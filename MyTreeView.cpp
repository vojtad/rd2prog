#include "MyTreeView.h"

#include <QContextMenuEvent>

MyTreeView::MyTreeView(QWidget *parent) :
	QTreeView(parent),
	m_contextMenu(this)
{
}

void MyTreeView::contextMenuEvent(QContextMenuEvent * event)
{
	m_contextMenu.exec(event->globalPos());
}

void MyTreeView::addContextMenuAction(QAction * action)
{
	if(action == NULL)
		m_contextMenu.addSeparator();
	else
		m_contextMenu.addAction(action);
}
