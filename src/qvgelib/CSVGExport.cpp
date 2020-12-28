/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L.Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QSvgGenerator>
#include <QPainter>
#include <QApplication>
#include <QScopedPointer>

#include "CSVGExport.h"
#include "CEditorScene.h"


bool CSVGExport::save(const QString& fileName, CEditorScene& scene, QString* /*lastError*/) const
{
	QScopedPointer<CEditorScene> tempScene(scene.clone());

	if (m_cutContent)
		tempScene->crop();

	QSvgGenerator svgWriter;
	svgWriter.setFileName(fileName);

	QString comment = scene.getClassAttribute("", "comment", false).defaultValue.toString();
	if (comment.size())
		svgWriter.setTitle(comment);

	QString desc = QString("Created with: ") + QApplication::applicationDisplayName();
	svgWriter.setDescription(desc);

	// resolution
	if (m_resolution > 0)
	{
		int res = svgWriter.resolution();
		double coeff = m_resolution / (double)res;
		auto size = tempScene->sceneRect().size();
		auto sizeInch = size * coeff;
		//auto sizeMM = sizeInch * 25.4;
		//svgWriter.setSize(sizeMM.toSize());
		//svgWriter.setResolution(m_resolution);
		svgWriter.setSize(sizeInch.toSize());
	}
	else
		svgWriter.setSize(tempScene->sceneRect().size().toSize());

	// export
	QPainter painter(&svgWriter);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);
	tempScene->render(&painter);
	painter.end();

	return true;
}
