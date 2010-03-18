#include "MyCloseModel.h"

MyCloseModel::MyCloseModel(const FileList & l, QObject * parent) :
		QAbstractListModel(parent),
		m_files(l)
{
}


QVariant MyCloseModel::data(const QModelIndex & index, int role) const
{
	if(role == Qt::DisplayRole)
	{
		QString ret = m_files.at(index.row())->fullPath();
		if(ret.isNull())
			ret = m_files.at(index.row())->name();
		return ret;
	}

	return QVariant();
}

int MyCloseModel::rowCount(const QModelIndex & parent) const
{
	Q_UNUSED(parent);

	return m_files.count();
}

int MyCloseModel::columnCount(const QModelIndex & parent) const
{
	Q_UNUSED(parent);

	return 1;
}

QModelIndex MyCloseModel::index(int row, int column, const QModelIndex & parent) const
{
	if(!parent.isValid() && row < m_files.size() && row >= 0)
		return createIndex(row, column, m_files.at(row));

	return QModelIndex();
}
