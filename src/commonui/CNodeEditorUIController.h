/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QAction>
#include <QLabel>
#include <QSettings>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>
#include <QTimer>

#include <slider2d.h>

#include <commonui/CSceneOptionsDialog.h>


class CMainWindow;

class CNodeEditorScene;
class CNodePort;
class CEditorView;
class IFileSerializer;


class CNodeEditorUIController : public QObject
{
	Q_OBJECT

public:
    CNodeEditorUIController(CMainWindow *parent);
	virtual ~CNodeEditorUIController();

	QSettings& getApplicationSettings() const;
	void doReadSettings(QSettings& settings);
	void doWriteSettings(QSettings& settings);

	bool loadFromFile(const QString &fileName, const QString &format, QString* lastError);
	bool saveToFile(const QString &fileName, const QString &format, QString* lastError);

    // callbacks
    void onNewDocumentCreated();
	void onDocumentLoaded(const QString &fileName);

// protected API
protected:
    CNodeEditorScene* scene() {
        return m_editorScene;
    }

private Q_SLOTS:
	bool doExport(const IFileSerializer &exporter);
	void exportFile();
	void exportPDF();
	void exportSVG();
	void exportDOT();
	bool importCSV(const QString &fileName, QString* lastError);

	void doBackup();

	void onNavigatorShown();

	void onSelectionChanged();
    void onSceneChanged();
	void onSceneHint(const QString& text);
	void onSceneStatusChanged(int status);
	void onSceneDoubleClicked(QGraphicsSceneMouseEvent* mouseEvent, QGraphicsItem* clickedItem);

	void sceneEditMode(QAction*);
	void onEditModeChanged(int mode);

	void onZoomChanged(double currentZoom);
	void zoom();
	void unzoom();
	void resetZoom();

    void sceneOptions();

	void showNodeIds(bool on);
	void showEdgeIds(bool on);

	void undo();
	void redo();

	void changeItemId();

	void addNodePort();
	void editNodePort();
	
	void factorNodes();

	void find();

private:
	void createMenus();
	void createPanels();
    void createNavigator();

	void readDefaultSceneSettings();
	void writeDefaultSceneSettings();

	void updateSceneOptions();

	void updateActions();
	void updateFromActions();

	void editNodePort(CNodePort &port);

private:
    CMainWindow *m_parent = nullptr;
	CNodeEditorScene *m_editorScene = nullptr;
	CEditorView *m_editorView = nullptr;

    class QSint::Slider2d *m_sliderView = nullptr;

    QLabel *m_statusLabel = nullptr;

	QMenu *m_viewMenu = nullptr;

	QAction *findAction = nullptr;

	QActionGroup *m_editModesGroup;
	QAction *modeDefaultAction;
	QAction *modeNodesAction;
	QAction *modeTransformAction;
	QAction *modeFactorAction;

	QAction *zoomAction;
	QAction *unzoomAction;
	QAction *resetZoomAction;
	QAction *resetZoomAction2;
	QAction *fitZoomAction;
	QAction *fitZoomSelectedAction;
	QAction *fitZoomBackAction;

    QAction *gridAction;
    QAction *gridSnapAction;
    QAction *actionShowLabels;
	QAction *m_actionShowNodeIds;
	QAction *m_actionShowEdgeIds;

	OptionsData m_optionsData;

	QTimer m_backupTimer;

#ifdef USE_OGDF
	class COGDFLayoutUIController *m_ogdfController = nullptr;
#endif

#ifdef USE_GVGRAPH
	class CGVGraphLayoutUIController *m_gvController = nullptr;
#endif

	class CColorSchemesUIController *m_schemesController = nullptr;

	class CNodeEdgePropertiesUI *m_propertiesPanel = nullptr;
	class CCommutationTable *m_connectionsPanel = nullptr;
	class CClassAttributesEditorUI *m_defaultsPanel = nullptr;
	class CQuickHelpUI *m_quickHelpPanel = nullptr;

	class CSearchDialog *m_searchDialog = nullptr;


	// IO
	class CDOTExportDialog *m_dotDialog = nullptr;
	class CImageExportDialog *m_imageDialog = nullptr;

	QString m_lastExportPath;
};
