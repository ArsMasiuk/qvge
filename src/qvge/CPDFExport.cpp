/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2017 Ars L.Masiuk(ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QFileDialog> 
#include <QPainter> 
#include <QPrinter> 

#include "CPDFExport.h"
#include "CEditorScene.h"


bool CPDFExport::save(const QString& fileName, CEditorScene& scene, QString* /*lastError*/) const
{
	CEditorScene* tempScene = scene.clone();

	tempScene->crop();

	QPrinter printer(QPrinter::HighResolution);
	printer.setPageSize(QPrinter::A4);
	printer.setOrientation(QPrinter::Portrait);
	printer.setOutputFormat(QPrinter::PdfFormat);
	printer.setOutputFileName(fileName);

	QPainter painter(&printer);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);
	tempScene->render(&painter);
	painter.end();

	delete tempScene;

	return true;
}