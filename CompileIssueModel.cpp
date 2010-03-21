#include "CompileIssueModel.h"

#include <QIcon>

CompileIssueModel::CompileIssueModel(QObject * parent) :
		QAbstractItemModel(parent),
		m_issues(NULL)
{
}

QVariant CompileIssueModel::data(const QModelIndex & index, int role) const
{
	if(index.row() >= m_issues->size())
		return QVariant();

	if(role == Qt::DisplayRole)
	{
		if(index.column() == 1)
		{
			if(m_issues->at(index.row()).m_line == -1)
				return QVariant();

			return m_issues->at(index.row()).m_line;
		}
		else if(index.column() == 2)
			return m_issues->at(index.row()).m_message;
	}
	else if(role == Qt::DecorationRole && index.column() == 0)
	{
		if(m_issues->at(index.row()).m_type == 1)
			return QIcon(":/icons/icon-ok.png");
		else if(m_issues->at(index.row()).m_type == 0)
			return QIcon(":/icons/edit-delete.png");
		else
			return QIcon(":/icons/information.png");
	}

	return QVariant();
}

QVariant CompileIssueModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	Q_UNUSED(section);
	Q_UNUSED(orientation);

	if(role == Qt::DisplayRole)
	{
		if(section == 1)
			return tr("Line");
		else if(section == 2)
			return tr("Message text");
	}

	return QVariant();
}

QModelIndex CompileIssueModel::index(int row, int column, const QModelIndex & parent) const
{
	if(!parent.isValid() && m_issues != NULL && row < m_issues->size() && row >= 0)
	{
		return createIndex(row, column, (void *)(&m_issues->at(row)));
	}

	return QModelIndex();
}

QModelIndex CompileIssueModel::parent(const QModelIndex & index) const
{
	Q_UNUSED(index);

	return QModelIndex();
}

int CompileIssueModel::rowCount(const QModelIndex & parent) const
{
	if(!parent.isValid() && m_issues != NULL)
		return m_issues->size();
	else
		return 0;
}

int CompileIssueModel::columnCount(const QModelIndex & parent) const
{
	Q_UNUSED(parent);

	return 3;
}

bool CompileIssueModel::hasChildren(const QModelIndex & parent) const
{
	Q_UNUSED(parent);

	return !parent.isValid();
}

void CompileIssueModel::setNewData(CompileIssueVector & v)
{
	m_issues = &v;

	reset();
}
