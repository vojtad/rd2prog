#include <QtGui/QApplication>
#include "MainWindow.h"

#include <QTextCodec>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

	a.setOrganizationName("Drbohlav");
	a.setApplicationName("RD2prog");
	a.setApplicationVersion("1.0");

	QTextCodec::setCodecForTr(QTextCodec::codecForName("utf8"));

	QTranslator t;
	t.load(a.applicationName() + "_" + QLocale::system().name());

	a.installTranslator(&t);

	MainWindow w(argc, argv);
    w.show();

    return a.exec();
}
