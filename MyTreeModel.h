#ifndef MYTREEMODEL_H
#define MYTREEMODEL_H

#include "MyFile.h"

#include <QAbstractItemModel>
#include <QStringList>

class MyTreeModel : public QAbstractItemModel
{
	Q_OBJECT

	friend class MainWindow;

	public:
		MyTreeModel(QObject * parent, FileList * files);

		QVariant data(const QModelIndex &index, int role) const;
		Qt::ItemFlags flags(const QModelIndex &index) const;
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
		QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
		QModelIndex parent(const QModelIndex & index) const;
		int rowCount(const QModelIndex & parent = QModelIndex()) const;
		int columnCount(const QModelIndex & parent = QModelIndex()) const;
		bool hasChildren(const QModelIndex & parent = QModelIndex()) const;

		bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());

	protected:
		bool canFetchMore(const QModelIndex & parent) const;
		void fetchMore(const QModelIndex & parent = QModelIndex());

	private:
		FileList * m_files;
		int m_filesCount;
};

#endif // MYTREEMODEL_H
