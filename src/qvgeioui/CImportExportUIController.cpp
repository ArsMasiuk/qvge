/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <CImportExportUIController.h>
#include <CDOTExportDialog.h>
#include <CImageExportDialog.h>
#include <CCSVImportDialog.h>

#ifdef USE_OGDF
#include <commonui/ogdf/COGDFLayoutUIController.h>
#include <commonui/ogdf/COGDFLayout.h>
#endif

#ifdef USE_GVGRAPH
#include <commonui/gvgraph/CGVGraphLayoutUIController.h>
#endif

#include <commonui/CExtListInputDialog.h>
#include <appbase/CMainWindow.h>

#include <qvge/CNode.h>
#include <qvge/CEdge.h>
#include <qvge/CImageExport.h>
#include <qvge/CPDFExport.h>
#include <qvge/CSVGExport.h>
#include <qvge/CNodeEditorScene.h>
#include <qvge/CFileSerializerGEXF.h>
#include <qvge/CFileSerializerGraphML.h>
#include <qvge/CFileSerializerXGR.h>
#include <qvge/CFileSerializerDOT.h>
#include <qvge/CFileSerializerPlainDOT.h>
#include <qvge/CFileSerializerCSV.h>
#include <qvge/ISceneItemFactory.h>

#include <QFileInfo>
#include <QFileDialog>
#include <QStatusBar>
#include <QDebug>


CImportExportUIController::CImportExportUIController(CMainWindow *parent): QObject(parent)
{
	m_parent = parent;

	// export dialogs
	m_dotDialog = new CDOTExportDialog(parent);
	m_imageDialog = new CImageExportDialog(parent);
}


// Settings

void CImportExportUIController::doReadSettings(QSettings& settings)
{
	m_lastExportPath = settings.value("lastExportPath", m_lastExportPath).toString();

	settings.beginGroup("ImageExport");
	m_imageDialog->doReadSettings(settings);
	settings.endGroup();
}


void CImportExportUIController::doWriteSettings(QSettings& settings)
{
	settings.setValue("lastExportPath", m_lastExportPath);

	settings.beginGroup("ImageExport");
	m_imageDialog->doWriteSettings(settings);
	settings.endGroup();
}


// IO

bool CImportExportUIController::doExport(CEditorScene& scene, const IFileSerializer &exporter)
{
    QString fileName = CUtils::cutLastSuffix(m_parent->getCurrentFileName());
    if (fileName.isEmpty())
        fileName = m_lastExportPath;
    else
        fileName = QFileInfo(m_lastExportPath).absolutePath() + "/" + QFileInfo(fileName).fileName() + "." + exporter.defaultFileExtension();

    //qDebug() << "doExport()" << fileName;

    QString path = QFileDialog::getSaveFileName(nullptr,
        QObject::tr("Export as") + " " + exporter.description(),
        fileName,
        exporter.filters()
    );

    if (path.isEmpty())
        return false;

    m_lastExportPath = path;

    if (exporter.save(path, scene))
    {
        m_parent->statusBar()->showMessage(tr("Export successful (%1)").arg(path));
        return true;
    }
    else
    {
        m_parent->statusBar()->showMessage(tr("Export failed (%1)").arg(path));
        return false;
    }
}


void CImportExportUIController::exportImage(CEditorScene& scene)
{
	m_imageDialog->setScene(scene);

	auto& settings = m_parent->getApplicationSettings();
	m_imageDialog->doReadSettings(settings);

	if (m_imageDialog->exec() == QDialog::Rejected)
		return;

	if (!doExport(scene, CImageExport(
			m_imageDialog->cutToContent(),
			m_imageDialog->resolution()
		))) 
		return;

	m_imageDialog->doWriteSettings(settings);
}


void CImportExportUIController::exportDOT(CEditorScene& scene)
{
	if (m_dotDialog->exec() == QDialog::Rejected)
		return;

    doExport(scene,	CFileSerializerDOT(
			m_dotDialog->writeBackground(),
			m_dotDialog->writeAttributes()
		)
	);
}


void CImportExportUIController::exportPDF(CEditorScene& scene)
{
	auto& settings = m_parent->getApplicationSettings();

	CPDFExport pdf;
	pdf.readSettings(settings);

	if (pdf.setupDialog(scene))
	{
		pdf.writeSettings(settings);
		doExport(scene, pdf);
	}
}


void CImportExportUIController::exportSVG(CEditorScene& scene)
{
	m_imageDialog->setScene(scene);

	auto& settings = m_parent->getApplicationSettings();
	m_imageDialog->doReadSettings(settings);

	if (m_imageDialog->exec() == QDialog::Rejected)
		return;

	if (!doExport(scene, CSVGExport(
			m_imageDialog->cutToContent(),
			m_imageDialog->resolution()
		)))
		return;

	m_imageDialog->doWriteSettings(settings);
}


bool CImportExportUIController::importCSV(CEditorScene& scene, const QString &fileName, QString* lastError)
{
	CCSVImportDialog csvDialog;
	csvDialog.setFileName(fileName);
	if (csvDialog.exec() == QDialog::Rejected)
	{
		if (lastError) *lastError = csvDialog.getLastErrorText();
		return false;
	}

	QStringList csvList;
	csvList << ";" << "," << "Tab";

	int index = CExtListInputDialog::getItemIndex(
		tr("Separator"),
		tr("Choose a separator of columns:"),
		csvList);
	if (index < 0)
		return false;

	CFileSerializerCSV csvLoader;
	switch (index)
	{
	case 0:     csvLoader.setDelimiter(';');    break;
	case 1:     csvLoader.setDelimiter(',');    break;
	default:    csvLoader.setDelimiter('\t');   break;
	}

	return (csvLoader.load(fileName, scene, lastError));
}


bool CImportExportUIController::loadFromFile(const QString &format, const QString &fileName, CNodeEditorScene& scene, QString* lastError)
{
	try 
	{
		if (format == "xgr")
		{
			return (CFileSerializerXGR().load(fileName, scene, lastError));
		}

		if (format == "graphml")
		{
			return (CFileSerializerGraphML().load(fileName, scene, lastError));
		}

		if (format == "gexf")
		{
			return (CFileSerializerGEXF().load(fileName, scene, lastError));
		}

		if (format == "dot" || format == "gv")
		{
			// try to load via graphviz
#ifdef USE_GVGRAPH
			if (m_gvController) 
			{
				bool ok = m_gvController->loadGraph(fileName, scene, lastError);
				if (ok)
					return true;
			}
#endif
			// try to load via boost
#ifdef USE_BOOST
			return (CFileSerializerDOT().load(fileName, scene, lastError));
#endif
		}

		if (format == "plain" || format == "txt")
		{
			return (CFileSerializerPlainDOT().load(fileName, scene, lastError));
		}

		if (format == "csv")
		{
			return importCSV(scene, fileName, lastError);
		}

		// else via ogdf
#ifdef USE_OGDF
        return (COGDFLayout::loadGraph(fileName, scene, lastError));
#else
		return false;
#endif
	}
	catch (...)
	{
		return false;
	}
}


bool CImportExportUIController::saveToFile(const QString &format, const QString &fileName, CNodeEditorScene& scene, QString* lastError)
{
    if (format == "xgr")
        return (CFileSerializerXGR().save(fileName, scene, lastError));

    if (format == "dot" || format == "gv")
        return (CFileSerializerDOT().save(fileName, scene, lastError));

    if (format == "gexf")
        return (CFileSerializerGEXF().save(fileName, scene, lastError));

	if (format == "graphml")
		return (CFileSerializerGraphML().save(fileName, scene, lastError));

    return false;
}
