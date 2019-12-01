/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2019 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CImageExportDialog.h"
#include "ui_CImageExportDialog.h"

#include <qvge/CEditorScene.h>


CImageExportDialog::CImageExportDialog(QWidget *parent): 
	QDialog(parent),
	ui(new Ui::CImageExportDialog)
{
	ui->setupUi(this);

	QImage temp(QSize(100,100), QImage::Format_ARGB32);
	m_dpi = temp.physicalDpiX();
	if (m_dpi > 0)
		ui->Resolution->setCurrentText(QString::number(m_dpi));
	else
		m_dpi = 96; // default

	connect(ui->Resolution, &QComboBox::currentTextChanged, this, &CImageExportDialog::updateTargetSize);
}


CImageExportDialog::~CImageExportDialog()
{
}


void CImageExportDialog::setScene(CEditorScene& scene)
{
	CEditorScene* tempScene = scene.clone();
	tempScene->crop();
	m_size = tempScene->sceneRect().size().toSize();
	delete tempScene;

	updateTargetSize();
}


void CImageExportDialog::updateTargetSize()
{
	int res = resolution();
	if (res <= 0)
		res = m_dpi;

	double coeff = (double)res / (double)m_dpi;
	QSize newSize = m_size * coeff;
	ui->ImageSize->setText(QString("%1 x %2").arg(newSize.width()).arg(newSize.height()));
}


bool CImageExportDialog::writeBackground() const
{
	return ui->WriteBackground->isChecked();
}


int CImageExportDialog::resolution() const
{
	int res = ui->Resolution->currentText().toUInt();
	return res;
}

