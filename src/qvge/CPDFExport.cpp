/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L.Masiuk(ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QPainter> 
#include <QPdfWriter> 
#include <QPageSize> 
#include <QPageLayout> 
#include <QMarginsF> 
#include <QDebug> 
#include <QScopedPointer>

#include "CPDFExport.h"
#include "CEditorScene.h"


CPDFExport::CPDFExport()
{
#ifdef Q_OS_WIN32
	m_printer = m_pageDialog.printer();
#else
    m_printer = new QPrinter;
#endif

    m_printer->setOutputFormat(QPrinter::NativeFormat);
}


CPDFExport::~CPDFExport()
{
#ifdef Q_OS_WIN32
#else
	delete m_printer;
#endif
}


// impl: setup interface

void CPDFExport::readSettings(QSettings& settings)
{
	settings.beginGroup("PDFExport");

	auto size = settings.value("PaperSize").toSize();
	QPageSize pageSize(size);
	m_printer->setPageSize(pageSize);

	QString paperName = settings.value("PaperName").toString();
	m_printer->setPaperName(paperName);

#ifdef Q_OS_WIN32
	int id = settings.value("WinPageSize").toInt();
	m_printer->setWinPageSize(id);
#endif

	QPrinter::Margins mm;
	mm.left = settings.value("MarginLeft").toDouble();
	mm.right = settings.value("MarginRight").toDouble();
	mm.top = settings.value("MarginTop").toDouble();
	mm.bottom = settings.value("MarginBottom").toDouble();
	m_printer->setMargins(mm);

	//QMarginsF mmf(mm.left, mm.top, mm.right, mm.bottom);
	//QPageLayout pl(pageSize, QPageLayout::Portrait, mmf);
	//m_printer->setPageLayout(pl);

	settings.endGroup();
}


void CPDFExport::writeSettings(QSettings& settings)
{
	settings.beginGroup("PDFExport");

	auto size = m_printer->pageLayout().pageSize().sizePoints();
	settings.setValue("PaperSize", size);

	QString paper = m_printer->paperName();
	settings.setValue("PaperName", paper);

#ifdef Q_OS_WIN32
	int id = m_printer->winPageSize();
	settings.setValue("WinPageSize", id);
#endif


	auto mm = m_printer->margins();
	settings.setValue("MarginLeft", mm.left);
	settings.setValue("MarginRight", mm.right);
	settings.setValue("MarginTop", mm.top);
	settings.setValue("MarginBottom", mm.bottom);
	settings.endGroup();
	settings.sync();
}


bool CPDFExport::setupDialog(CEditorScene& scene)
{
	auto bbox = scene.itemsBoundingRect();
	if (bbox.width() > bbox.height())
		m_printer->setOrientation(QPrinter::Landscape);
	else
		m_printer->setOrientation(QPrinter::Portrait);

#ifdef Q_OS_WIN32
	if (m_pageDialog.exec() == QDialog::Rejected)
		return false;
#else
    QPageSetupDialog pd(m_printer);
    if (pd.exec() == QDialog::Rejected)
        return false;
#endif

	return true;
}


// impl: IFileSerializer

bool CPDFExport::save(const QString& fileName, CEditorScene& scene, QString* /*lastError*/) const
{
	Q_ASSERT(m_printer);

	QScopedPointer<CEditorScene> tempScene(scene.clone());

	tempScene->crop();

	QPdfWriter writer(fileName);
	writer.setPageSize(m_printer->pageSize());
	writer.setPageOrientation(m_printer->orientation() == QPrinter::Landscape ? QPageLayout::Landscape : QPageLayout::Portrait);
	writer.setMargins(m_printer->margins());

	QPainter painter(&writer);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);
	tempScene->render(&painter);
	painter.end();

	return true;
}
