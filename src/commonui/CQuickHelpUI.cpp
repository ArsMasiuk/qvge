#include "CQuickHelpUI.h"
#include "ui_CQuickHelpUI.h"

#include <QFile>
#include <QTextStream>


CQuickHelpUI::CQuickHelpUI(QWidget *parent): 
	QWidget(parent),
	ui(new Ui::CQuickHelpUI)
{
	ui->setupUi(this);

	//QStringList sl;
	//sl << QApplication::applicationDirPath() + "/../lang/en";
	//ui->Viewer->setSearchPaths(sl);
	//ui->Viewer->setSource(QUrl("help.htm"));

	QFile file(":/Help/Mini_EN");
	if (file.open(QFile::ReadOnly | QFile::Text))
	{
		QTextStream stream(&file);
		ui->Viewer->setHtml(stream.readAll());
	}
}


CQuickHelpUI::~CQuickHelpUI()
{
}

