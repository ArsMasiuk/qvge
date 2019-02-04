#include "CDOTExportDialog.h"
#include "ui_CDOTExportDialog.h"


CDOTExportDialog::CDOTExportDialog(QWidget *parent): 
	QDialog(parent),
	ui(new Ui::CDOTExportDialog)
{
	ui->setupUi(this);
}


CDOTExportDialog::~CDOTExportDialog()
{
}


bool CDOTExportDialog::writeBackground() const
{
	return ui->WriteBackground->isChecked();
}


bool CDOTExportDialog::writeAttributes() const
{
	return ui->WriteAttributes->isChecked();
}

