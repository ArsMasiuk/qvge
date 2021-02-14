/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2021 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

class CMainWindow;
class CEditorScene;
class CNodeEditorScene;
class IFileSerializer;

// think: to move?
class CGVGraphLayoutUIController;

#include <QSettings>


class CImportExportUIController: public QObject
{
	Q_OBJECT

public:
	CImportExportUIController(CMainWindow *parent);

	// think: to move?
	void setGVGraphController(CGVGraphLayoutUIController *gvController) { m_gvController = gvController; }

	void doReadSettings(QSettings& settings);
	void doWriteSettings(QSettings& settings);

	void exportImage(CEditorScene& scene);
	void exportPDF(CEditorScene& scene);
	void exportSVG(CEditorScene& scene);
	void exportDOT(CEditorScene& scene);
	bool importCSV(CEditorScene& scene, const QString &fileName, QString* lastError);

	bool loadFromFile(const QString &format, const QString &fileName, CNodeEditorScene& scene, QString* lastError);
	bool saveToFile(const QString &format, const QString &fileName, CNodeEditorScene& scene, QString* lastError);

private:
	bool doExport(CEditorScene& scene, const IFileSerializer &exporter);

private:
	CMainWindow *m_parent = nullptr;

	class CDOTExportDialog *m_dotDialog = nullptr;
	class CImageExportDialog *m_imageDialog = nullptr;

	// think: to move?
	CGVGraphLayoutUIController *m_gvController = nullptr;

	QString m_lastExportPath;
};
