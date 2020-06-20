/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L.Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QImageWriter>
#include <QImage>
#include <QPainter>
#include <QMap>
#include <QByteArray>
#include <QSet>

#include "CImageExport.h"
#include "CEditorScene.h"


QString CImageExport::filters() const
{
	static QList<QByteArray> formats = QImageWriter::supportedImageFormats();
	if (formats.isEmpty())
		return QString();

	static QMap<QByteArray, QString> formatNames = {
		{ "bmp", "Windows Bitmap (*.bmp)" },
        { "ico", "Windows Icon (*.ico *.cur)" },
		{ "gif", "Graphic Interchange Format (*.gif)" },
        { "jpg", "Joint Photographic Experts Group (*.jpg *.jpeg)" },
		{ "png", "Portable Network Graphics (*.png)" },
		{ "pbm", "Portable Bitmap (*.pbm)" },
		{ "pgm", "Portable Graymap (*.pgm)" },
		{ "ppm", "Portable Pixmap (*.ppm)" },
		{ "svg", "Scalable Vector Graphics (*.svg)" },
        { "tif", "Tagged Image File Format (*.tif *.tiff)" },
		{ "xbm", "X11 Bitmap (*.xbm)" },
		{ "xpm", "X11 Pixmap (*.xpm)" },
		{ "wbmp", "Wireless Bitmap (*.wbmp)" },
		{ "webp", "WebP (*.webp)" },
		{ "icns", "Apple Icon Image (*.icns)" }
	};

	static QMap<QByteArray, QByteArray> recodeMap = {
		{ "jpeg", "jpg" },
		{ "tiff", "tif" },
		{ "cur", "ico" }
	};

	static QString filter;
	if (filter.isEmpty())
	{
		QSet<QByteArray> usedFormats;

		for (auto format : formats)
		{
			auto suffix = format.toLower();
			if (recodeMap.contains(suffix))
				usedFormats << recodeMap[suffix];
			else
				usedFormats << suffix;
		}

		// add known formats
		for (auto it = formatNames.constBegin(); it != formatNames.constEnd(); ++it)
		{
			if (usedFormats.contains(it.key()))
			{
				usedFormats.remove(it.key());
				filter += it.value() + ";;";
			}
		}

		// add evtl. unlisted ones
		for (auto format : usedFormats)
		{
			filter += format + " (*." + format + ");;";
		}

		filter.chop(2);
	}

	return filter;
}


bool CImageExport::save(const QString& fileName, CEditorScene& scene, QString* /*lastError*/) const
{
	CEditorScene* tempScene = scene.clone();

	if (m_cutContent)
		tempScene->crop();

	QImage image(tempScene->sceneRect().size().toSize(), QImage::Format_ARGB32);
	QRect targetRect;	// empty by default

	// resolution
	int old_dpi = image.physicalDpiX();
	if (old_dpi <= 0)
		old_dpi = 96;

	if (m_resolution > 0 && old_dpi != m_resolution)
	{
		double coeff = (double)m_resolution / (double)old_dpi;
		int dpm = m_resolution / 0.0254;
		image.setDotsPerMeterX(dpm);
		image.setDotsPerMeterY(dpm);

		QSize newSize = image.size() * coeff;
		image = image.scaled(newSize);

		targetRect = QRect(0,0, newSize.width(), newSize.height());
	}

	image.fill(Qt::white);
	QPainter painter(&image);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);
	tempScene->render(&painter, targetRect);
	painter.end();

	delete tempScene;

	return image.save(fileName);
}
