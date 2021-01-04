#include "CDOTPreviewPage.h"
#include "ui_CDOTPreviewPage.h"

#include <QFile>
#include <QTextStream>


CDOTPreviewPage::CDOTPreviewPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CDOTPreviewPage)
{
    ui->setupUi(this);
}


CDOTPreviewPage::~CDOTPreviewPage()
{
    delete ui;
}


bool CDOTPreviewPage::load(const QString &fileName, QString *lastError)
{
	// fake test
	QFile f(fileName);
	if (!f.open(QFile::ReadOnly))
	{
		if (lastError) *lastError = f.errorString();
		return false;
	}

	QTextStream ts(&f);
	ui->DotEditor->setPlainText(ts.readAll());
	f.close();

    return true;
}
