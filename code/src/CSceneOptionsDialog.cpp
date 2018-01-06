/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CSceneOptionsDialog.h"
#include "ui_CSceneOptionsDialog.h"

CSceneOptionsDialog::CSceneOptionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CSceneOptionsDialog)
{
    ui->setupUi(this);

    ui->BackgroundColor->setColorScheme(QSint::OpenOfficeColors());
    ui->GridColor->setColorScheme(QSint::OpenOfficeColors());
}

CSceneOptionsDialog::~CSceneOptionsDialog()
{
    delete ui;
}


int CSceneOptionsDialog::exec(CEditorScene &scene)
{
	ui->BackgroundColor->setColor(scene.backgroundBrush().color());

	QPen gridPen = scene.getGridPen();
	ui->GridColor->setColor(gridPen.color());

	ui->GridSize->setValue(scene.getGridSize());
	ui->GridVisible->setChecked(scene.gridEnabled());
	ui->GridSnap->setChecked(scene.gridSnapEnabled());


	if (QDialog::exec() == QDialog::Rejected)
		return QDialog::Rejected;


	scene.setBackgroundBrush(ui->BackgroundColor->color());

	gridPen.setColor(ui->GridColor->color());
	scene.setGridPen(gridPen);

	scene.setGridSize(ui->GridSize->value());
	scene.enableGrid(ui->GridVisible->isChecked());
	scene.enableGridSnap(ui->GridSnap->isChecked());

	return QDialog::Accepted;
}
