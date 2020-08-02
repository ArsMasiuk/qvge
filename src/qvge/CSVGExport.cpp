/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L.Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QSvgGenerator>
#include <QPainter>

#include "CSVGExport.h"
#include "CEditorScene.h"


bool CSVGExport::save(const QString& fileName, CEditorScene& scene, QString* /*lastError*/) const
{
	CEditorScene* tempScene = scene.clone();

	if (m_cutContent)
		tempScene->crop();

	QSvgGenerator svgWriter;
	QRect targetRect;	// empty by default
	svgWriter.setFileName(fileName);

	// resolution
	if (m_resolution > 0)
	{
		//svgWriter.setResolution(m_resolution);
		double coeff = m_resolution / 96.0;
		auto size = tempScene->sceneRect().size() * coeff;
		svgWriter.setSize(size.toSize());
	}
	else
		svgWriter.setSize(tempScene->sceneRect().size().toSize());

	// export
	QPainter painter(&svgWriter);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);
	tempScene->render(&painter);
	painter.end();

	delete tempScene;

	return true;
}
