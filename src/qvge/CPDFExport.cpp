/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L.Masiuk(ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QPainter> 

#include "CPDFExport.h"
#include "CEditorScene.h"

CPDFExport::CPDFExport(QPrinter* printer): m_printer(printer)
{
	if (!m_printer)
	{
		m_printer = new QPrinter(QPrinter::HighResolution);
		m_printer->setPageSize(QPrinter::A4);
		m_printer->setOrientation(QPrinter::Portrait);
	}

	m_printer->setOutputFormat(QPrinter::PdfFormat);
}


bool CPDFExport::save(const QString& fileName, CEditorScene& scene, QString* /*lastError*/) const
{
	Q_ASSERT(m_printer);

	m_printer->setOutputFileName(fileName);

	CEditorScene* tempScene = scene.clone();
	tempScene->crop();

	QPainter painter(m_printer);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);
	tempScene->render(&painter);
	painter.end();

	delete tempScene;

	return true;
}