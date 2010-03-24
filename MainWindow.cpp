#include "MainWindow.h"

#include "CloseDialog.h"
#include "ISPFrame.h"
#include "SettingsDialog.h"
#include "WriteDialog.h"

#include <QFontDialog>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>

static QVector<char> g_chars;

QString randomString(int len)
{
	if(g_chars.isEmpty())
	{
		for(int i = 48; i <= 57; ++i)
			g_chars.push_back(char(i));

		for(int i = 65; i <= 90; ++i)
		{
			g_chars.push_back(char(i));
			g_chars.push_back(char(i + 32));
		}
	}

	QString ret;
	while(ret.size() < len)
		ret += QString(g_chars.at(qrand() % g_chars.size()));

	return ret;
}

QString generateTempPath()
{
	QString path = QDir::temp().filePath(randomString(4));
	while(QFile::exists(QString("%1.asm").arg(path)))
		path = QDir::temp().filePath(randomString(4));

	return path;
}

MainWindow::MainWindow(QWidget * parent) :
	QMainWindow(parent),
	m_unnamedFilesCounter(0),
	m_treeModel(this, &m_files),
	m_settingsDialog(new SettingsDialog(this)),
	m_compileIssueModel(this)
{
	ui.setupUi(this);

	ui.actionNew->setShortcut(QKeySequence::New);
	ui.actionOpen_File->setShortcut(QKeySequence::Open);
//	ui.actionSave_All_Files->setShortcut(QKeySequence::);
	ui.actionSave_File->setShortcut(QKeySequence::Save);
	ui.actionSave_File_As->setShortcut(QKeySequence::SaveAs);
	ui.actionClose_File->setShortcut(QKeySequence::Close);
//	ui.actionClose_All_Files->setShortcut(QKeySequence::);
	ui.actionExit->setShortcut(QKeySequence::Quit);

	ui.actionUndo->setShortcut(QKeySequence::Undo);
	ui.actionRedo->setShortcut(QKeySequence::Redo);
	ui.actionCut->setShortcut(QKeySequence::Cut);
	ui.actionCopy->setShortcut(QKeySequence::Copy);
	ui.actionPaste->setShortcut(QKeySequence::Paste);
	ui.actionDelete->setShortcut(QKeySequence::Delete);
	ui.actionSelect_All->setShortcut(QKeySequence::SelectAll);

//	ui.actionOptions->setShortcut(QKeySequence::Preferences);

	ui.textEdit->addContextMenuAction(ui.actionUndo);
	ui.textEdit->addContextMenuAction(ui.actionRedo);
	ui.textEdit->addContextMenuAction(NULL);
	ui.textEdit->addContextMenuAction(ui.actionCut);
	ui.textEdit->addContextMenuAction(ui.actionCopy);
	ui.textEdit->addContextMenuAction(ui.actionPaste);
	ui.textEdit->addContextMenuAction(ui.actionDelete);
	ui.textEdit->addContextMenuAction(NULL);
	ui.textEdit->addContextMenuAction(ui.actionSelect_All);
	ui.textEdit->addContextMenuAction(NULL);
	ui.textEdit->addContextMenuAction(ui.actionCompile);
//	ui.textEdit->addContextMenuAction(ui.actionCopy_HEX);

	ui.treeView->addContextMenuAction(ui.actionNew);
	ui.treeView->addContextMenuAction(NULL);
	ui.treeView->addContextMenuAction(ui.actionOpen_File);
	ui.treeView->addContextMenuAction(ui.actionRecent_Files);
	ui.treeView->addContextMenuAction(NULL);
	ui.treeView->addContextMenuAction(ui.actionSave_File);
	ui.treeView->addContextMenuAction(ui.actionSave_File_As);
	ui.treeView->addContextMenuAction(ui.actionSave_All_Files);
	ui.treeView->addContextMenuAction(NULL);
	ui.treeView->addContextMenuAction(ui.actionClose_File);
	ui.treeView->addContextMenuAction(ui.actionClose_All_Files);

	ui.treeView->setModel(&m_treeModel);
	ui.treeView->resizeColumnToContents(0);

	ui.treeViewCompileIssues->setModel(&m_compileIssueModel);
	ui.treeViewCompileIssues->resizeColumnToContents(0);
	ui.treeViewCompileIssues->resizeColumnToContents(1);
	ui.treeViewCompileIssues->resizeColumnToContents(2);

	connect(ui.treeView->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(currentChanged(QModelIndex, QModelIndex)));
	connect(&m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(compiled(int, QProcess::ExitStatus)));
	connect(ui.actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

	if(qApp->arguments().size() > 1)
	{
		for(QStringList::const_iterator it = qApp->arguments().begin() + 1; it != qApp->arguments().end(); ++it)
			m_files.append(new MyFile(*it, false));
	}

	loadSettings();

	if(m_files.isEmpty())
		newFile();

	ui.splitter->setStretchFactor(ui.splitter->indexOf(ui.splitter_2), 1);
	ui.splitter_2->setStretchFactor(ui.splitter_2->indexOf(ui.textEdit), 1);

	m_recentFilesActions.fill(NULL, m_maxRecentFiles);
	ui.actionRecent_Files->setMenu(new QMenu(this));
	for(int i = 0; i < m_maxRecentFiles; ++i)
	{
		m_recentFilesActions[i] = new QAction(this);
		ui.actionRecent_Files->menu()->addAction(m_recentFilesActions[i]);
		m_recentFilesActions[i]->setVisible(false);
		connect(m_recentFilesActions[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
	}
	ui.actionRecent_Files->menu()->addSeparator();
	ui.actionRecent_Files->menu()->addAction(ui.actionClear_recent_files);

	updateRecentFilesActions();
}

MainWindow::~MainWindow()
{
	qDeleteAll(m_files);

	foreach(const QString & s, m_toRemove)
	{
		QFile::remove(s);
	}
}

MyFile * MainWindow::currentFile()
{
	if(ui.treeView->currentIndex().isValid())
		return static_cast<MyFile *>(ui.treeView->currentIndex().internalPointer());

	return NULL;
}

void MainWindow::newFile(const QString & path)
{
	MyFile * f;
	if(path.isEmpty())
		f = new MyFile(QString(tr("Unnamed %1")).arg(++m_unnamedFilesCounter), true);
	else
		f = new MyFile(path, false);

	m_files.push_back(f);
	m_treeModel.fetchMore();

	ui.treeView->setCurrentIndex(m_treeModel.index(m_files.size() - 1, 0));
}

void MainWindow::closeEvent(QCloseEvent * event)
{
	FileList list;
	foreach(MyFile * f, m_files)
	{
		if(f->document()->isModified())
			list.append(f);
	}

	if(!list.isEmpty())
	{
		CloseDialog cd(list, this);

		if(cd.exec() == QDialog::Rejected)
		{
			event->ignore();
			return;
		}
	}

	event->accept();
	saveSettings();
}

void MainWindow::changeEvent(QEvent * e)
{
	QMainWindow::changeEvent(e);
	switch (e->type())
	{
	case QEvent::LanguageChange:
		ui.retranslateUi(this);
		break;
	default:
		break;
	}
}

void MainWindow::on_actionNew_triggered()
{
	newFile();
}

void MainWindow::on_actionClose_File_triggered()
{
	int row = ui.treeView->currentIndex().row();

	m_treeModel.removeRow(row);
	MyFile * f = m_files.at(row);
	m_files.remove(row);

	if(m_files.isEmpty())
		newFile();
	else
	{
		updateCurrentRow();
		ui.treeView->setCurrentIndex(m_treeModel.index(m_files.size() - 1, 0));
	}

	if(f->document()->isModified())
		save(f, false);

	delete f;
}

void MainWindow::currentChanged(const QModelIndex & current, const QModelIndex & prev)
{
	Q_UNUSED(prev);

	if(current.isValid())
	{
		currentFile()->document()->setDefaultFont(ui.textEdit->font());
		ui.textEdit->setDocument(currentFile()->document());
		ui.textEdit->updateLineNumberAreaWidth(0);
		ui.textEdit->clearHighlightedLines();
		foreach(const CompileIssue & c, currentFile()->m_compileIssues)
		{
			if(c.m_line != -1)
			{
				ui.textEdit->addHighlightedLine(c.m_line - 1, QColor("#FFCDCD"));
			}
		}
		ui.textEdit->highlightLines();

		m_compileIssueModel.setNewData(currentFile()->m_compileIssues);

		ui.treeViewCompileIssues->resizeColumnToContents(0);
		ui.treeViewCompileIssues->resizeColumnToContents(1);
		ui.treeViewCompileIssues->resizeColumnToContents(2);

		setWindowTitle("RD2prog - " + currentFile()->name());
	}

	ui.textEdit->setFocus();
}

void MainWindow::on_textEdit_textChanged()
{
	updateCurrentRow();

	ui.actionUndo->setEnabled(ui.textEdit->document()->isUndoAvailable());
	ui.actionRedo->setEnabled(ui.textEdit->document()->isRedoAvailable());
	ui.actionSave_File->setEnabled(ui.textEdit->document()->isModified());
	ui.actionSave_All_Files->setEnabled(!isAllSaved());

	ui.textEdit->clearHighlightedLines();
	ui.textEdit->highlightLines();
}

int MainWindow::isOpened(const QString & s) const
{
	int i = 0;
	foreach(MyFile * f, m_files)
	{
		if(f->fullPath() == s)
			return i;
		++i;
	}

	return -1;
}

void MainWindow::on_actionOpen_File_triggered()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open files"), m_lastOpenDir,
														tr("Assembler source code (*.asm *.a51);;All files (*)"));

	foreach(QString s, fileNames)
	{
		int opened = isOpened(s);
		if(opened == -1)
		{
			newFile(s);
			addRecentFile(s);
		}
		else
		{
			ui.treeView->setCurrentIndex(m_treeModel.index(opened, 0, QModelIndex()));
		}

		m_lastOpenDir = QFileInfo(s).absolutePath();
	}

	ui.textEdit->setFocus();
}

void MainWindow::on_actionSave_File_As_triggered()
{
	if(save(currentFile(), true) && isAllSaved())
		ui.actionSave_All_Files->setEnabled(false);
}

void MainWindow::updateCurrentRow()
{
	emit m_treeModel.dataChanged(m_treeModel.index(ui.treeView->currentIndex().row(), 0),
								 m_treeModel.index(ui.treeView->currentIndex().row(), 1));
}

bool MainWindow::save(MyFile * file, bool forceSaveDialog)
{
	if(!forceSaveDialog && !file->fullPath().isEmpty())
	{
		if(file->save())
		{
			updateCurrentRow();
			addRecentFile(file->fullPath());
			return true;
		}
	}

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save %1 as...").arg(file->name()), m_lastSaveDir,
													tr("Assembler source code (*.asm *.a51);;All files (*)"));

	if(!fileName.isEmpty())
	{
		m_lastSaveDir = QFileInfo(fileName).absolutePath();
	}

	while(!fileName.isEmpty() && !file->save(fileName))
	{
		QMessageBox::StandardButton ret = QMessageBox::critical(this, tr("Cannot save file"),
							  tr("File %1 could not be saved. Do you want to save it with another name?").arg(fileName),
							  QMessageBox::Yes | QMessageBox::No);

		if(ret == QMessageBox::No)
			return false;
	}

	if(!fileName.isEmpty())
	{
		if(!file->fullPath().isEmpty())
			addRecentFile(file->fullPath());

		updateCurrentRow();

		return true;
	}

	return false;
}

void MainWindow::loadSettings()
{
	QSettings * s = m_settingsDialog->settings();

	restoreGeometry(s->value("MainWindow/geometry").toByteArray());
	restoreState(s->value("MainWindow/state").toByteArray());

	QByteArray ar = s->value("splitter/state").toByteArray();
	if(!ar.isNull())
		ui.splitter->restoreState(ar);
	else
	{
		QList<int> sizes;
		sizes << 150 << 1;
		ui.splitter->setSizes(sizes);
	}

	ar = s->value("splitter_2/state").toByteArray();
	if(!ar.isNull())
		ui.splitter_2->restoreState(ar);
	else
	{
		QList<int> sizes;
		sizes << 400 << 1;
		ui.splitter_2->setSizes(sizes);
	}

	m_lastSaveDir = s->value("lastSaveDir", QDir::tempPath()).toString();
	m_lastOpenDir = s->value("lastOpenDir", QDir::tempPath()).toString();

	QFont font;
	QString str = s->value("textEdit/font").toString();
	if(str.isNull())
	{
		font.setFamily("Courier");
		font.setPointSize(10);
	}
	else
		font.fromString(str);

	ui.textEdit->setFont(font);

	m_maxRecentFiles = s->value("recentFiles/max", 15).toInt();
	m_recentFiles = s->value("recentFiles/list").toStringList();
}

void MainWindow::saveSettings()
{
	QSettings * s = m_settingsDialog->settings();

	s->setValue("MainWindow/geometry", saveGeometry());
	s->setValue("MainWindow/state", saveState());

	s->setValue("splitter/state", ui.splitter->saveState());
	s->setValue("splitter_2/state", ui.splitter_2->saveState());

	s->setValue("lastSaveDir", m_lastSaveDir);
	s->setValue("lastOpenDir", m_lastOpenDir);

	s->setValue("textEdit/font", ui.textEdit->font().toString());

	s->setValue("recentFiles/max", m_maxRecentFiles);
	s->setValue("recentFiles/list", m_recentFiles);
}

void MainWindow::addRecentFile(const QString & path)
{
	m_recentFiles.removeAll(path);
	m_recentFiles.prepend(path);
	while(m_recentFiles.size() > m_maxRecentFiles)
	{
		m_recentFiles.removeLast();
	}

	updateRecentFilesActions();
}

void MainWindow::updateRecentFilesActions()
{
	for(int i = 0; i < m_recentFiles.count(); ++i)
	{
		m_recentFilesActions[i]->setText(m_recentFiles.at(i));
		m_recentFilesActions[i]->setData(m_recentFiles.at(i));
		m_recentFilesActions[i]->setVisible(true);
		m_recentFilesActions[i]->setEnabled(true);
	}

	for(int i = m_recentFiles.count(); i < m_maxRecentFiles; ++i)
	{
		m_recentFilesActions[i]->setVisible(false);
	}

	if(m_recentFiles.isEmpty())
	{
		m_recentFilesActions[0]->setVisible(true);
		m_recentFilesActions[0]->setEnabled(false);
		m_recentFilesActions[0]->setText(tr("No Recent Files"));
	}
}

void MainWindow::on_actionSave_File_triggered()
{
	if(save(currentFile(), false))
		ui.actionSave_File->setEnabled(false);

	if(isAllSaved())
		ui.actionSave_All_Files->setEnabled(false);
}

bool MainWindow::isAllSaved() const
{
	foreach(MyFile * f, m_files)
	{
		if(f->document()->isModified())
		{
			return false;
		}
	}

	return true;
}

void MainWindow::on_actionSave_All_Files_triggered()
{
	bool enabled = false;
	foreach(MyFile * f, m_files)
	{
		if(f->document()->isModified())
		{
			if(!save(f, false))
				enabled = true;
			else
				ui.actionSave_File->setEnabled(false);
		}
	}

	ui.actionSave_All_Files->setEnabled(enabled);
}

void MainWindow::on_actionClose_All_Files_triggered()
{
	FileList tmp = m_files;

	m_treeModel.removeRows(0, m_files.size(), QModelIndex());
	m_files.clear();

	newFile();

	for(FileList::iterator it = tmp.begin(); it != tmp.end(); ++it)
	{
		MyFile * f = *it;
		if(f->document()->isModified())
			save(f, false);

		delete f;
	}
}

void MainWindow::on_actionSelect_Font_triggered()
{
	bool ok;
	QFont font = QFontDialog::getFont(&ok, ui.textEdit->font(), this);

	if(ok)
	{
		ui.textEdit->setFont(font);
	}
}

void MainWindow::openRecentFile()
{
	QAction * action = qobject_cast<QAction *>(sender());
	newFile(action->data().toString());
}

void MainWindow::on_actionClear_recent_files_triggered()
{
	m_recentFiles.clear();
	updateRecentFilesActions();
}

void MainWindow::on_actionCompile_triggered()
{
	if(!QFileInfo(m_settingsDialog->assemblerPath()).exists())
	{
		currentFile()->m_compileIssues.clear();
		ui.textEdit->clearHighlightedLines();

		currentFile()->m_compileIssues.push_back(CompileIssue(-1, tr("Assembler wasn't found. You can change its path in Settings.")));

		m_compileIssueModel.setNewData(currentFile()->m_compileIssues);

		ui.treeViewCompileIssues->resizeColumnToContents(0);
		ui.treeViewCompileIssues->resizeColumnToContents(1);
		ui.treeViewCompileIssues->resizeColumnToContents(2);

		ui.textEdit->highlightLines();
		return;
	}

	bool temp = false;
	QString path = currentFile()->fullPath();
	QString asmPath = currentFile()->fullPath();

	if(path.isEmpty() || (currentFile()->isModified() && !currentFile()->save()))
	{
		if(currentFile()->temp().isEmpty())
			currentFile()->setTemp(generateTempPath());

		path = currentFile()->temp();

		asmPath = path + ".asm";

		QFile f(asmPath);
		if(f.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			f.write(currentFile()->document()->toPlainText().toLocal8Bit());
			f.close();
		}

		temp = true;
		m_toRemove.append(QDir::toNativeSeparators(asmPath));
	}
	else
	{
		path = QFileInfo(path).absoluteDir().filePath(QFileInfo(path).baseName());
		updateCurrentRow();
	}

	path = QDir::toNativeSeparators(path);
	asmPath = QDir::toNativeSeparators(asmPath);

	QString lstPath = path + ".lst";
	QString hexPath = path + ".hex";

	if(!m_settingsDialog->saveLstHexWithSource())
	{
		lstPath = QDir::temp().filePath(QFileInfo(path).baseName() + ".lst");
		hexPath = QDir::temp().filePath(QFileInfo(path).baseName() + ".hex");
	}

	if(temp || !m_settingsDialog->saveLstHexWithSource())
	{
		m_toRemove.append(lstPath);
		m_toRemove.append(hexPath);
	}

	currentFile()->setListingPath(lstPath);
	currentFile()->setHexPath(hexPath);

#if defined(Q_OS_LINUX)
	QString p = QString("\"%1\" \"%2\" \"%3\" \"%4\"").arg(m_settingsDialog->assemblerPath(), asmPath, hexPath, lstPath);
#elif defined(Q_OS_WIN)
	QString p = QString("cmd /C \"%1\" \"%2\" \"%3\" \"%4\"").arg(m_settingsDialog->assemblerPath(), asmPath, hexPath, lstPath);
#endif

	m_process.start(p, QIODevice::ReadOnly);
	m_process.waitForFinished();
}

void MainWindow::on_actionSettings_triggered()
{
	m_settingsDialog->exec();
}

void MainWindow::on_actionUpload_program_triggered()
{
	QList<QByteArray> data;

	QFile f(currentFile()->hexPath());
	if(f.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QTextStream stream(&f);
		QString line = stream.readLine();
		while(!line.isNull())
		{
			data.push_back(line.toAscii());
			line = stream.readLine();
		}
		f.close();
	}

	WriteDialog dialog(m_settingsDialog->portSettings(), data, this);

	dialog.exec();
}

#include <QTextBrowser>

void MainWindow::on_actionShow_Listing_triggered()
{
	QFile f(currentFile()->listingPath());
	if(f.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QDialog d(this);
		QHBoxLayout l(&d);
		QTextBrowser browser(&d);
		browser.setPlainText(f.readAll());
		browser.setFont(ui.textEdit->font());
		l.addWidget(&browser);
		d.resize(QSize(800, 600));
		d.setWindowTitle(tr("Listing file: %1").arg(currentFile()->listingPath()));
		d.exec();
	}
	else
	{
		QMessageBox::critical(this, tr("Cannot open Listing file"),
							  tr("Listing file doesn't exist. You have to Compile source code at first."),
							  QMessageBox::Ok);
	}
}

void MainWindow::compiled(int exitCode, QProcess::ExitStatus exitStatus)
{
	Q_UNUSED(exitStatus);
	Q_UNUSED(exitCode);

	currentFile()->m_compileIssues.clear();
	ui.textEdit->clearHighlightedLines();

	QFile f(currentFile()->listingPath());
	if(f.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		int lineNumber = -1;
		QRegExp lineNumberExp("(\\d+):");
		QRegExp errorExp("@ ([^@]+) @");
		while(!f.atEnd())
		{
			QString line = f.readLine();
			if(lineNumberExp.indexIn(line, 0) != -1)
			{
				lineNumber = lineNumberExp.cap(1).toInt();
			}
			else if(errorExp.indexIn(line, 0) != -1)
			{
				currentFile()->m_compileIssues.push_back(CompileIssue(lineNumber, errorExp.cap(1)));
				if(lineNumber > 0)
				{
					ui.textEdit->addHighlightedLine(lineNumber - 1, QColor("#FFCDCD"));
				}
			}
		}
		f.close();

		if(currentFile()->m_compileIssues.isEmpty()) // no errors :P
		{
			currentFile()->m_compileIssues.push_back(CompileIssue(-1, tr("No errors found. Compiled successfully."), 1));
		}
		else
		{
			currentFile()->m_compileIssues.push_back(CompileIssue(-1, tr("%1 errors found.").arg(currentFile()->m_compileIssues.count())));
		}

		currentFile()->m_compileIssues.push_back(CompileIssue(-1, tr("Listing file: %1").arg(currentFile()->listingPath()), 2));
		currentFile()->m_compileIssues.push_back(CompileIssue(-1, tr("HEX file: %1").arg(currentFile()->hexPath()), 3));
	}
	else
	{
		currentFile()->m_compileIssues.push_back(CompileIssue(-1, tr("Cannot open listing file: %1").arg(currentFile()->listingPath())));
	}

	m_compileIssueModel.setNewData(currentFile()->m_compileIssues);

	ui.treeViewCompileIssues->resizeColumnToContents(0);
	ui.treeViewCompileIssues->resizeColumnToContents(1);
	ui.treeViewCompileIssues->resizeColumnToContents(2);

	ui.textEdit->highlightLines();
}

void MainWindow::on_actionShow_HEX_triggered()
{
	QFile f(currentFile()->hexPath());
	if(f.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QDialog d(this);
		QHBoxLayout l(&d);
		QTextBrowser browser(&d);
		browser.setPlainText(f.readAll());
		browser.setFont(ui.textEdit->font());
		l.addWidget(&browser);
		d.resize(QSize(800, 600));
		d.setWindowTitle(tr("HEX file: %1").arg(currentFile()->hexPath()));
		d.exec();
	}
	else
	{
		QMessageBox::critical(this, tr("Cannot open HEX file"),
							  tr("HEX file doesn't exist. You have to Compile source code at first."), QMessageBox::Ok);
	}
}

void MainWindow::on_treeViewCompileIssues_activated(const QModelIndex & index)
{
	if(index.isValid())
	{
		CompileIssue * i = static_cast<CompileIssue *>(index.internalPointer());
		switch(i->m_type)
		{
			case 0: // go to line m_line
							{
				if(i->m_line == -1)
					return;
								QTextCursor c(ui.textEdit->textCursor());
								c.setPosition(ui.textEdit->document()->findBlockByLineNumber(i->m_line - 1).position());
								ui.textEdit->setTextCursor(c);
								ui.textEdit->setFocus();
							}
				break;
			case 1: // do nothing
				break;
			case 2: // show listing
				on_actionShow_Listing_triggered();
				break;
			case 3: // show hex
				on_actionShow_HEX_triggered();
				break;
		}
	}
}

void MainWindow::on_actionFull_chip_erase_triggered()
{
	ISP::ISPFrame frame(ISP::WriteFunction, ISP::WriteFunctionArgument::FullChipErase);

	WriteDialog dialog(m_settingsDialog->portSettings(), QList<QByteArray>() << frame.data(), this);

	dialog.setWindowTitle(tr("RD2prog - full chip erase"));
	dialog.exec();
}
