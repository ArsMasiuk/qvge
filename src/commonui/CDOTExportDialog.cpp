/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

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

