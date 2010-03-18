#ifndef COMPILEISSUEMODEL_H
#define COMPILEISSUEMODEL_H

#include <QAbstractItemModel>
#include <QList>
#include <QObject>

struct CompileIssue
{
	CompileIssue(int line, QString message, int type = 0) :
			m_type(type),
			m_line(line),
			m_message(message)
	{
	}

	CompileIssue()
	{
	}

	int m_type; // 0 - error, 1 - ok
	int m_line;
	QString m_message;
};

typedef QList<CompileIssue> CompileIssueVector;

class CompileIssueModel : public QAbstractItemModel
{
	Q_OBJECT

	public:
		CompileIssueModel(QObject * parent);
		~CompileIssueModel() {}

		QVariant data(const QModelIndex &index, int role) const;
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
		QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
		QModelIndex parent(const QModelIndex & index) const;
		int rowCount(const QModelIndex & parent = QModelIndex()) const;
		int columnCount(const QModelIndex & parent = QModelIndex()) const;
		bool hasChildren(const QModelIndex & parent = QModelIndex()) const;

		void setNewData(CompileIssueVector & v);

	private:
		CompileIssueVector * m_issues;
};

#endif // COMPILEISSUEMODEL_H
