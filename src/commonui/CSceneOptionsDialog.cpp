/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QPixmapCache>

#include <appbase/CPlatformServices.h>

#include "CSceneOptionsDialog.h"
#include "ui_CSceneOptionsDialog.h"


CSceneOptionsDialog::CSceneOptionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CSceneOptionsDialog)
{
    ui->setupUi(this);

    ui->BackgroundColor->setColorScheme(QSint::OpenOfficeColors());
    ui->GridColor->setColorScheme(QSint::OpenOfficeColors());

#ifndef USE_OGDF
	ui->StartupGB->hide();
#endif
}

CSceneOptionsDialog::~CSceneOptionsDialog()
{
    delete ui;
}


int CSceneOptionsDialog::exec(CEditorScene &scene, CEditorView &view, OptionsData &data)
{
	ui->BackgroundColor->setColor(scene.backgroundBrush().color());

	QPen gridPen = scene.getGridPen();
	ui->GridColor->setColor(gridPen.color());

	ui->GridSize->setValue(scene.getGridSize());
	ui->GridVisible->setChecked(scene.gridEnabled());
	ui->GridSnap->setChecked(scene.gridSnapEnabled());

	ui->Antialiasing->setChecked(view.renderHints().testFlag(QPainter::Antialiasing));

	ui->CacheSlider->setValue(QPixmapCache::cacheLimit() / 1024);
	quint64 ram = CPlatformServices::GetTotalRAMBytes() / (1024 * 1024);	// mb
	ram /= 2;	// 50%
    ui->CacheSlider->setMaximum((int)ram);
    ui->CacheSlider->setUnitText(tr("MB"));

	ui->EnableBackups->setChecked(data.backupPeriod > 0);
	ui->BackupPeriod->setValue(data.backupPeriod);

	ui->AutoCreateGraph->setChecked(data.newGraphDialogOnStart);


	if (QDialog::exec() == QDialog::Rejected)
		return QDialog::Rejected;


	scene.setBackgroundBrush(ui->BackgroundColor->color());

	gridPen.setColor(ui->GridColor->color());
	scene.setGridPen(gridPen);

	scene.setGridSize(ui->GridSize->value());
	scene.enableGrid(ui->GridVisible->isChecked());
	scene.enableGridSnap(ui->GridSnap->isChecked());

	bool isAA = ui->Antialiasing->isChecked();
	view.setRenderHint(QPainter::Antialiasing, isAA);
	scene.setFontAntialiased(isAA);

	QPixmapCache::setCacheLimit(ui->CacheSlider->value() * 1024);

	data.backupPeriod = ui->EnableBackups->isChecked() ? ui->BackupPeriod->value() : 0;

	data.newGraphDialogOnStart = ui->AutoCreateGraph->isChecked();

	return QDialog::Accepted;
}
