#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "ui_SettingsDialog.h"

#include <QSettings>

#include "SerialPortInterface.h"

class SettingsDialog : public QDialog
{
	Q_OBJECT
public:
	SettingsDialog(QWidget * parent = 0);
	~SettingsDialog();

	const SerialPortSettings & portSettings() const;
	QString assemblerPath() const;
	bool saveLstHexWithSource() const;

	QSettings * settings();

protected:
	void changeEvent(QEvent * e);

private:
	Ui::SettingsDialog ui;

	SerialPortSettings m_portSettings;
	QString m_assemblerPath;
	bool m_allowCheckPort;
	QSettings * m_settings;

	void checkPort();
	void loadSettings();

private slots:
	void on_toolButtonBrowse_clicked();
 void on_pushButton_clicked();
	void on_comboBoxBaudrate_currentIndexChanged(const QString & speed);
	void on_comboBoxPort_currentIndexChanged(const QString & port);

	void saveSettings();
	void checkAssemblerPath(const QString & text);
};

#endif // SettingsDialog_H
