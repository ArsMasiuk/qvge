/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QtWidgets/QApplication>

#include "qvgeMainWindow.h"


int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setWindowIcon(QIcon(":/Icons/AppIcon"));
	qvgeMainWindow w;
	w.init(argc, argv);
    w.show();
	return a.exec();
}
