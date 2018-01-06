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


bool CPDFExport::write(/*const*/ CEditorScene &scene, const QString &startPath)
{
	QFileInfo fi(startPath);
	QString fileName(startPath);
	if (int ss = fi.completeSuffix().size()) {
		fileName.chop(ss + 1); // .suffix
	}
	fileName += ".pdf";

	QString selectedFilter;

	QString path = QFileDialog::getSaveFileName(NULL,
		QObject::tr("Export as PDF"), fileName, "*.pdf", &selectedFilter);

	if (path.isEmpty())
		return false;

	QPrinter printer(QPrinter::HighResolution);
	printer.setPageSize(QPrinter::A4);
	printer.setOrientation(QPrinter::Portrait);
	printer.setOutputFormat(QPrinter::PdfFormat);
	printer.setOutputFileName(path);

	QPainter painter(&printer);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);
	scene.render(&painter);

	return true;
}