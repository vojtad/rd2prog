#include "MyFile.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPlainTextDocumentLayout>

#include "MyTextEdit.h"

MyFile::MyFile(const QString & path, bool n)
{
	m_document.setDocumentLayout(new QPlainTextDocumentLayout(&m_document));
	new Highlighter(&m_document);

	QFile f(path);

	m_name = QFileInfo(f).fileName();
	if(!n)
	{
		m_fullPath = path;
	}

	if(f.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		m_document.setPlainText(f.readAll());
		m_document.setModified(false);
		f.close();
	}
}

MyFile::~MyFile()
{
}

bool MyFile::save(QString f)
{
	if(f.isEmpty())
		f = m_fullPath;

	if(f.isEmpty())
		return false;

	QFile file(f);

	if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;

	file.write(document()->toPlainText().toLocal8Bit());
	file.close();

	m_name = QFileInfo(f).fileName();
	m_fullPath = f;
	document()->setModified(false);

	return true;
}
