/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2017 Ars L.Masiuk(ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QImageWriter>
#include <QFileDialog> 
#include <QPainter> 

#include "CImageExport.h"
#include "CUtils.h"


bool CImageExport::write(/*const*/ CEditorScene &scene, const QString &startPath)
{
	QList<QByteArray> formats = QImageWriter::supportedImageFormats();
	if (formats.isEmpty())
		return false;

	QString filter;

	for (auto format : formats)
		filter += format + "(*." + format + ");;";

	filter.chop(2);

	QString fileName = CUtils::cutLastSuffix(startPath);
	QString selectedFilter;

	QString path = QFileDialog::getSaveFileName(NULL,
		QObject::tr("Export as Image"), fileName, filter, &selectedFilter);

	if (path.isEmpty())
		return false;

	QImage image(scene.sceneRect().size().toSize(), QImage::Format_ARGB32);
	image.fill(Qt::white);
	QPainter painter(&image);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);
	scene.render(&painter);

	return image.save(path);
}