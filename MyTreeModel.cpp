#include "MyTreeModel.h"

#include <QBrush>
#include <QIcon>

MyTreeModel::MyTreeModel(QObject * parent, FileList * files) :
		QAbstractItemModel(parent),
		m_files(files),
		m_filesCount(0)
{
}

bool MyTreeModel::canFetchMore(const QModelIndex & parent) const
{
	if(parent.isValid())
		return false;

	return m_filesCount < m_files->size();
}

void MyTreeModel::fetchMore(const QModelIndex & parent)
{
	Q_UNUSED(parent);

	beginInsertRows(QModelIndex(), m_filesCount, m_files->size() - 1);
	m_filesCount = m_files->size();
	endInsertRows();
}

QVariant MyTreeModel::data(const QModelIndex & index, int role) const
{
	if(index.row() >= m_files->size())
		return QVariant();

	MyFile * f = static_cast<MyFile *>(index.internalPointer());

	if(role == Qt::DisplayRole && index.column() == 1)
	{
		return f->name();
	}
	else if(role == Qt::DecorationRole && index.column() == 0)
	{
		if(f->document()->isModified())
			return QIcon(":/icons/file-save.png");
		else
			return QIcon();
	}
	else if(role == Qt::BackgroundRole)
	{
		if(f->isModified())
		{
			if(index.row() % 2 == 0)
				return QBrush(QColor("#FFEBEB"));
			else
				return QBrush(QColor("#FFCDCD"));
		}
		else
		{
			if(index.row() % 2 == 0)
				return QBrush(QColor("#EBEBFF"));
			else
				return QBrush(QColor("#CDCDFF"));
		}
	}

	return QVariant();
}

Qt::ItemFlags MyTreeModel::flags(const QModelIndex & index) const
{
	Q_UNUSED(index);
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant MyTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	Q_UNUSED(orientation);

	if(role == Qt::DisplayRole)
	{
		if(section == 1)
			return tr("Opened files");
	}

	return QVariant();
}

QModelIndex MyTreeModel::index(int row, int column, const QModelIndex & parent) const
{
	if(!parent.isValid() && row < m_files->size() && row >= 0)
		return createIndex(row, column, m_files->at(row));

	return QModelIndex();
}

QModelIndex MyTreeModel::parent(const QModelIndex & index) const
{
	Q_UNUSED(index);

	return QModelIndex();
}

int MyTreeModel::rowCount(const QModelIndex & parent) const
{
	Q_UNUSED(parent);

	if(!parent.isValid())
		return m_filesCount;
	else
		return 0;
}

int MyTreeModel::columnCount(const QModelIndex & parent) const
{
	Q_UNUSED(parent);

	return 2;
}

bool MyTreeModel::hasChildren(const QModelIndex & parent) const
{
	Q_UNUSED(parent);

	return !parent.isValid();
}

bool MyTreeModel::removeRows(int row, int count, const QModelIndex & parent)
{
	if(!parent.isValid())
	{
		beginRemoveRows(parent, row, row + count - 1);
		m_filesCount -= count;
		endRemoveRows();

		return true;
	}

	return false;
}
