#ifndef MYCLOSEMODEL_H
#define MYCLOSEMODEL_H

#include <QAbstractListModel>
#include <QVector>

#include "MyFile.h"

class MyCloseModel : public QAbstractListModel
{
	public:
		MyCloseModel(const FileList & l, QObject * parent = 0);

		QVariant data(const QModelIndex &index, int role) const;
		int rowCount(const QModelIndex & parent = QModelIndex()) const;
		int columnCount(const QModelIndex & parent = QModelIndex()) const;
		QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;

		QVector<MyFile *>  m_files;
};

#endif // MYCLOSEMODEL_H
