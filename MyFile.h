#ifndef MYFILE_H
#define MYFILE_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTextDocument>

#include <CompileIssueModel.h>

class MyFile;

typedef QVector<MyFile *> FileList;

class MyFile
{
	public:
		MyFile(const QString & path, bool n);
		~MyFile();

		inline const QString & name() const { return m_name; }
		inline const QString & fullPath() const { return m_fullPath; }
		inline const QString & listingPath() const { return m_listingPath; }
		inline const QString & hexPath() const { return m_hexPath; }
		inline const QString & temp() const { return m_temp; }
		inline QTextDocument * document() { return &m_document; }
		inline bool isModified() const { return m_document.isModified(); }

		inline void setListingPath(const QString & p) { m_listingPath = p; }
		inline void setHexPath(const QString & p) { m_hexPath = p; }
		inline void setTemp(const QString & t) { m_temp = t; }

		bool save(QString f = QString());

		CompileIssueVector m_compileIssues;

	private:
		QString m_name;
		QString m_fullPath;
		QString m_listingPath;
		QString m_hexPath;
		QString m_temp;
		QTextDocument m_document;
};

#endif // MYFILE_H
