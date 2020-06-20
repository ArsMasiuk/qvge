/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QtWidgets/QApplication>

#include "qvgeMainWindow.h"


int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
    Q_INIT_RESOURCE(commonui);
    Q_INIT_RESOURCE(appbase);
	a.setWindowIcon(QIcon(":/Icons/AppIcon"));
//	a.setStyle("fusion");
//	a.setStyle("windows");

	qvgeMainWindow w;
	w.init(QCoreApplication::arguments());

	return a.exec();
}

