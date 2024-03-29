#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_MainWindow.h"

#include "MyTreeModel.h"
#include "CompileIssueModel.h"
#include "SettingsDialog.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QThread>
#include <QWaitCondition>

typedef QVector<QAction *> ActionVector;

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();

	void closeEvent(QCloseEvent * event);

	bool save(MyFile * file, bool forceSaveDialog);

protected:
	void changeEvent(QEvent * e);

private:
	Ui::MainWindow ui;

	FileList m_files;
	int m_unnamedFilesCounter;

	QStringList m_recentFiles;
	QStringList m_toRemove;
	ActionVector m_recentFilesActions;
	int m_maxRecentFiles;

	QString m_lastSaveDir;
	QString m_lastOpenDir;

	QProcess m_process;
#if defined(Q_OS_LINUX)
	QString m_processOutput;
#endif

	MyTreeModel m_treeModel;

	SettingsDialog * m_settingsDialog;

	QWaitCondition m_waitCondition;

	CompileIssueModel m_compileIssueModel;

	MyFile * currentFile();
	void newFile(const QString & path = QString());
	void loadSettings();
	void saveSettings();
	void addRecentFile(const QString & path);
	void updateRecentFilesActions();
	int isOpened(const QString & s) const;
	void compileCurrentFile();
	bool isAllSaved() const;
	void updateCurrentRow();

private slots:
	void on_actionAbout_triggered();
 void on_actionFull_chip_erase_triggered();
	void on_treeViewCompileIssues_activated(const QModelIndex & index);
	void on_actionShow_HEX_triggered();
	void on_actionShow_Listing_triggered();
	void on_actionUpload_program_triggered();
	void on_actionSettings_triggered();
	void on_actionCompile_triggered();
	void on_actionClear_recent_files_triggered();
	void on_actionSelect_Font_triggered();
	void on_actionClose_All_Files_triggered();
	void on_actionSave_All_Files_triggered();
	void on_actionSave_File_triggered();
	void on_actionSave_File_As_triggered();
	void on_actionOpen_File_triggered();
	void on_textEdit_textChanged();
	void on_actionClose_File_triggered();
	void on_actionNew_triggered();

	void currentChanged(const QModelIndex & current, const QModelIndex & prev);
	void openRecentFile();
	void compiled(int exitCode, QProcess::ExitStatus exitStatus);
};

#endif // MAINWINDOW_H
