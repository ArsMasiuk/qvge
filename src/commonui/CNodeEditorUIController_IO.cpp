/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <CNodeEditorUIController.h>
#include <CDOTExportDialog.h>
#include <CImageExportDialog.h>
#include <CCSVImportDialog.h>
#include <CExtListInputDialog.h>

#ifdef USE_OGDF
#include <ogdf/COGDFLayoutUIController.h>
#include <ogdf/COGDFNewGraphDialog.h>
#include <ogdf/COGDFLayout.h>
#endif

#include <qvge/CNode.h>
#include <qvge/CEdge.h>
#include <qvge/CImageExport.h>
#include <qvge/CPDFExport.h>
#include <qvge/CNodeEditorScene.h>
#include <qvge/CFileSerializerGEXF.h>
#include <qvge/CFileSerializerGraphML.h>
#include <qvge/CFileSerializerXGR.h>
#include <qvge/CFileSerializerDOT.h>
#include <qvge/CFileSerializerCSV.h>
#include <qvge/ISceneItemFactory.h>

#include <appbase/CMainWindow.h>

#include <QFileInfo>
#include <QFileDialog>
#include <QPageSetupDialog>
#include <QStatusBar>


bool CNodeEditorUIController::doExport(const IFileSerializer &exporter)
{
    QString fileName = CUtils::cutLastSuffix(m_parent->getCurrentFileName());
    if (fileName.isEmpty())
        fileName = m_lastExportPath;
    else
        fileName = QFileInfo(m_lastExportPath).absolutePath() + "/" + QFileInfo(fileName).fileName();

    QString path = QFileDialog::getSaveFileName(nullptr,
        QObject::tr("Export as") + " " + exporter.description(),
        fileName,
        exporter.filters()
    );

    if (path.isEmpty())
        return false;

    m_lastExportPath = path;

    if (exporter.save(path, *m_editorScene))
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


void CNodeEditorUIController::exportFile()
{
	m_imageDialog->setScene(*m_editorScene);

	auto& settings = getApplicationSettings();
	m_imageDialog->doReadSettings(settings);

	if (m_imageDialog->exec() == QDialog::Rejected)
		return;

	if (!doExport(
		CImageExport(
			m_imageDialog->cutToContent(),
			m_imageDialog->resolution()
		))) 
		return;

	m_imageDialog->doWriteSettings(settings);
}


void CNodeEditorUIController::exportDOT()
{
	if (m_dotDialog->exec() == QDialog::Rejected)
		return;

    doExport(
		CFileSerializerDOT(
			m_dotDialog->writeBackground(),
			m_dotDialog->writeAttributes()
		)
	);
}


void CNodeEditorUIController::exportPDF()
{
	QPageSetupDialog pageDialog;
	if (pageDialog.exec() == QDialog::Rejected)
		return;

	QPrinter* pagePrinter = pageDialog.printer();

	CPDFExport pdf(pagePrinter);

    doExport(pdf);
}


bool CNodeEditorUIController::importCSV(const QString &fileName, QString* lastError)
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

	return (csvLoader.load(fileName, *m_editorScene, lastError));
}


bool CNodeEditorUIController::loadFromFile(const QString &fileName, const QString &format, QString* lastError)
{
	try 
	{
		if (format == "xgr")
		{
			return (CFileSerializerXGR().load(fileName, *m_editorScene, lastError));
		}

		if (format == "graphml")
		{
			return (CFileSerializerGraphML().load(fileName, *m_editorScene, lastError));
		}

		if (format == "gexf")
		{
			return (CFileSerializerGEXF().load(fileName, *m_editorScene, lastError));
		}

		if (format == "csv")
		{
			return importCSV(fileName, lastError);
		}

		// else via ogdf
#ifdef USE_OGDF
		return (COGDFLayout::loadGraph(fileName, *m_editorScene, lastError));
#else
		return false;
#endif
	}
	catch (...)
	{
		return false;
	}
}


bool CNodeEditorUIController::saveToFile(const QString &fileName, const QString &format, QString* lastError)
{
    if (format == "xgr")
        return (CFileSerializerXGR().save(fileName, *m_editorScene, lastError));

    if (format == "dot" || format == "gv")
        return (CFileSerializerDOT().save(fileName, *m_editorScene, lastError));

    if (format == "gexf")
        return (CFileSerializerGEXF().save(fileName, *m_editorScene, lastError));

	if (format == "graphml")
		return (CFileSerializerGraphML().save(fileName, *m_editorScene, lastError));

    return false;
}
