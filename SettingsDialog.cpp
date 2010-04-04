#include "SettingsDialog.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>

SettingsDialog::SettingsDialog(QWidget * parent) :
	QDialog(parent),
	m_allowCheckPort(false)
{
	if(!QFileInfo("G:\\profily\\").isDir())
	{
		m_settings =  new QSettings(this);
	}
	else
	{
		m_settings = new QSettings("G:\\profily\\RD2prog\\settings.ini", QSettings::IniFormat, this);
	}

	ui.setupUi(this);

	ui.comboBoxPort->addItems(SerialPortInterface::getPorts());

	connect(this, SIGNAL(accepted()), this, SLOT(saveSettings()));
	connect(ui.lineEditPath, SIGNAL(textEdited(QString)), this, SLOT(checkAssemblerPath(QString)));

	loadSettings();

	checkAssemblerPath(ui.lineEditPath->text());

	m_allowCheckPort = true;
	checkPort();
}

SettingsDialog::~SettingsDialog()
{
	delete m_settings;
}

void SettingsDialog::changeEvent(QEvent *e)
{
	QDialog::changeEvent(e);
	switch (e->type())
	{
	case QEvent::LanguageChange:
		ui.retranslateUi(this);
		break;
	default:
		break;
	}
}

const SerialPortSettings & SettingsDialog::portSettings() const
{
	return m_portSettings;
}

QString SettingsDialog::assemblerPath() const
{
	return ui.lineEditPath->text();
}

bool SettingsDialog::saveLstHexWithSource() const
{
	return ui.checkBoxSaveLstHEX->isChecked();
}

void SettingsDialog::on_comboBoxPort_currentIndexChanged(const QString & port)
{
	m_portSettings.m_name = port;

	checkPort();
}

void SettingsDialog::on_comboBoxBaudrate_currentIndexChanged(const QString & speed)
{
	m_portSettings.m_baudrate = speed.toInt();

	checkPort();
}

void SettingsDialog::checkPort()
{
	if(!m_allowCheckPort)
		return;

	SerialPortInterface spi(m_portSettings, this);
	if(spi.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
	{
		ui.labelAvailIcon->setPixmap(QPixmap(":/icons/icon-ok.png"));
		ui.labelAvailText->setText(QString(tr("%1 is available")).arg(m_portSettings.m_name));
		spi.close();
	}
	else
	{
		ui.labelAvailIcon->setPixmap(QPixmap(":/icons/icon-cancel.png"));
		ui.labelAvailText->setText(QString(tr("%1 is not available")).arg(m_portSettings.m_name));
	}
}

QSettings * SettingsDialog::settings()
{
	return m_settings;
}

void SettingsDialog::loadSettings()
{
	m_portSettings.m_name = m_settings->value("settings/portName", ui.comboBoxPort->currentText()).toString();
	m_portSettings.m_baudrate = m_settings->value("settings/portBaudrate", 9600).toInt();

#define ASSEMBLER_EXEC "asemw"
#if defined(Q_OS_WIN)
#undef ASSEMBLER_EXEC
#define ASSEMBLER_EXEC "asemw.exe"
#endif

	ui.lineEditPath->setText(m_settings->value("settings/assemblerPath",
											   QDir::current().filePath(ASSEMBLER_EXEC)).toString());
	ui.checkBoxSaveLstHEX->setChecked(m_settings->value("settings/saveLstHexWithSource", true).toBool());

	if(!m_portSettings.m_name.isEmpty() && ui.comboBoxPort->currentText() != m_portSettings.m_name)
	{
		for(int i = 0; i < ui.comboBoxPort->count(); ++i)
		{
			if(ui.comboBoxPort->itemText(i) == m_portSettings.m_name)
			{
				ui.comboBoxPort->setCurrentIndex(i);
				break;
			}
		}
	}

	for(int i = 0; i < ui.comboBoxBaudrate->count(); ++i)
	{
		if(ui.comboBoxBaudrate->itemText(i).toInt() == m_portSettings.m_baudrate)
		{
			ui.comboBoxBaudrate->setCurrentIndex(i);
			break;
		}
	}
}

void SettingsDialog::saveSettings()
{
	m_settings->setValue("settings/portName", m_portSettings.m_name);
	m_settings->setValue("settings/portBaudrate", m_portSettings.m_baudrate);
	m_settings->setValue("settings/assemblerPath", assemblerPath());
	m_settings->setValue("settings/saveLstHexWithSource", saveLstHexWithSource());
}

void SettingsDialog::on_pushButton_clicked()
{
	m_allowCheckPort = false;

	QString name = ui.comboBoxPort->currentText();
	ui.comboBoxPort->clear();
	ui.comboBoxPort->addItems(SerialPortInterface::getPorts());

	if(ui.comboBoxPort->currentText() != name)
	{
		for(int i = 0; i < ui.comboBoxPort->count(); ++i)
		{
			if(ui.comboBoxPort->itemText(i) == name)
			{
				ui.comboBoxPort->setCurrentIndex(i);
				break;
			}
		}
	}

	m_allowCheckPort = true;
	checkPort();
}

void SettingsDialog::checkAssemblerPath(const QString & text)
{
	QFileInfo fi(text);

	if(fi.isFile() && fi.isExecutable())
		ui.labePathStatus->setPixmap(QPixmap(":/icons/icon-ok.png"));
	else
		ui.labePathStatus->setPixmap(QPixmap(":/icons/icon-cancel.png"));
}

void SettingsDialog::on_toolButtonBrowse_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select assembler"), QDir::currentPath(),
														tr("All files (*)"));
	if(!fileName.isEmpty())
	{
		ui.lineEditPath->setText(fileName);
		checkAssemblerPath(fileName);
	}
}
