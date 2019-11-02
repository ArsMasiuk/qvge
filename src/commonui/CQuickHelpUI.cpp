#include "CQuickHelpUI.h"
#include "ui_CQuickHelpUI.h"


CQuickHelpUI::CQuickHelpUI(QWidget *parent): 
	QWidget(parent),
	ui(new Ui::CQuickHelpUI)
{
	ui->setupUi(this);

	// test only
#ifdef Q_OS_WIN32
	QStringList sl;
	sl << QApplication::applicationDirPath() + "/../lang/en";
	ui->Viewer->setSearchPaths(sl);
	ui->Viewer->setSource(QUrl("help.htm"));
#endif
}


CQuickHelpUI::~CQuickHelpUI()
{
}

