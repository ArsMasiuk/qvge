/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2025 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QPainter> 
#include <QPageSize> 
#include <QPageLayout> 
#include <QMarginsF> 
#include <QDebug> 
#include <QScopedPointer>

#include "CPDFExport.h"
#include "CEditorScene.h"


CPDFExport::CPDFExport()
{
	m_settings.pageLayout.setMode(QPageLayout::FullPageMode);
}


CPDFExport::~CPDFExport()
{
}


// impl: setup interface

void CPDFExport::readSettings(QSettings& settings)
{
	settings.beginGroup("PDFExport");

	m_settings.resolution = settings.value("Resolution", 1200).toInt();

	QMarginsF mm;
	mm.setLeft(settings.value("MarginLeft").toDouble());
	mm.setRight(settings.value("MarginRight").toDouble());
	mm.setTop(settings.value("MarginTop").toDouble());
	mm.setBottom(settings.value("MarginBottom").toDouble());
	m_settings.pageLayout.setMargins(mm);

	m_settings.pageLayout.setOrientation(
		static_cast<QPageLayout::Orientation>(settings.value("PageLayout", QPageLayout::Portrait).toInt())
	);

	m_settings.pageLayout.setPageSize(
		QPageSize(
			static_cast<QPageSize::PageSizeId>(settings.value("PageSize", QPageSize::A4).toInt())
		)
	);

	settings.endGroup();
}


void CPDFExport::writeSettings(QSettings& settings)
{
	settings.beginGroup("PDFExport");

	settings.setValue("Resolution", m_settings.resolution);

	settings.setValue("MarginLeft", m_settings.pageLayout.margins().left());
	settings.setValue("MarginRight", m_settings.pageLayout.margins().right());
	settings.setValue("MarginTop", m_settings.pageLayout.margins().top());
	settings.setValue("MarginBottom", m_settings.pageLayout.margins().bottom());

	settings.setValue("PageLayout", m_settings.pageLayout.orientation());
	settings.setValue("PageSize", m_settings.pageLayout.pageSize().id());

	settings.endGroup();
	settings.sync();
}


bool CPDFExport::setupDialog(CEditorScene& scene)
{
//	if (!m_printer || !m_printer->isValid())
//	{
//		qWarning() << "Printer is not initialized!";
//		return false;
//	}
//
//	auto bbox = scene.itemsBoundingRect();
//	if (bbox.width() > bbox.height())
//		m_printer->setOrientation(QPrinter::Landscape);
//	else
//		m_printer->setOrientation(QPrinter::Portrait);
//
////#ifdef Q_OS_WIN32
////	if (m_pageDialog.exec() == QDialog::Rejected)
////		return false;
////#else
//    QPageSetupDialog pd(m_printer);
//    if (pd.exec() == QDialog::Rejected)
//        return false;
////#endif

	return true;
}


// impl: IFileSerializer

bool CPDFExport::save(const QString& fileName, CEditorScene& scene, QString* /*lastError*/) const
{
	//Q_ASSERT(m_printer);

	QScopedPointer<CEditorScene> tempScene(scene.clone());

	tempScene->crop();

	QPdfWriter writer(fileName);
	writer.setPageSize(m_settings.pageLayout.pageSize());
	writer.setPageOrientation(m_settings.pageLayout.orientation());
	writer.setMargins(
		{
			m_settings.pageLayout.margins().left(),
			m_settings.pageLayout.margins().right(),
			m_settings.pageLayout.margins().top(),
			m_settings.pageLayout.margins().bottom()
		}
	);
	writer.setResolution(m_settings.resolution);

	QPainter painter(&writer);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);
	tempScene->render(&painter);
	painter.end();

	return true;
}
