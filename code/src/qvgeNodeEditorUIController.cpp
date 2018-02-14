/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <qvgeNodeEditorUIController.h>
#include <qvgeMainWindow.h>
#include <CCommutationTable.h>
#include <CSceneOptionsDialog.h>
#include <CNodeEdgePropertiesUI.h>
#include <CClassAttributesEditorUI.h>
#include <COGDFLayoutUIController.h>
#include <COGDFNewGraphDialog.h>
#include <COGDFLayout.h>

#include <qvge/CNode.h>
#include <qvge/CConnection.h>
#include <qvge/CImageExport.h>
#include <qvge/CPDFExport.h>
#include <qvge/CNodeEditorScene.h>
#include <qvge/CEditorView.h>
#include <qvge/CFileSerializerGEXF.h>
#include <qvge/CFileSerializerGraphML.h>
#include <qvge/CFileSerializerXGR.h>
#include <qvge/CFileSerializerDOT.h>

#include <QMenuBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QMenu>
#include <QToolButton>
#include <QWidgetAction>
#include <QResizeEvent>
#include <QDebug>
#include <QPixmapCache>
#include <QFileDialog>


qvgeNodeEditorUIController::qvgeNodeEditorUIController(qvgeMainWindow *parent) :
	QObject(parent),
	m_parent(parent)
{
	// create document
	m_editorScene = new CNodeEditorScene(parent);
	m_editorView = new CEditorView(m_editorScene, parent);
	parent->setCentralWidget(m_editorView);

	// connect scene
	connect(m_editorScene, &CEditorScene::sceneChanged, parent, &CMainWindow::onDocumentChanged);
    connect(m_editorScene, &CEditorScene::sceneChanged, this, &qvgeNodeEditorUIController::onSceneChanged);
	connect(m_editorScene, &CEditorScene::selectionChanged, this, &qvgeNodeEditorUIController::onSelectionChanged);

	// connect view
	connect(m_editorView, SIGNAL(scaleChanged(double)), this, SLOT(onZoomChanged(double)));

    // slider2d
    createNavigator();

	// menus & actions
	createMenus();

	// dock panels
	createPanels();

    // status bar
    m_statusLabel = new QLabel();
    parent->statusBar()->addPermanentWidget(m_statusLabel);

    // update actions
    onSceneChanged();
    onSelectionChanged();
    onZoomChanged(1);

    // OGDF
    m_ogdfController = new COGDFLayoutUIController(parent, m_editorScene);
}


void qvgeNodeEditorUIController::createMenus()
{
	// file actions
	QAction *exportAction = m_parent->getFileExportAction();
	exportAction->setVisible(true);
	exportAction->setText(tr("Export to &Image..."));
	connect(exportAction, &QAction::triggered, this, &qvgeNodeEditorUIController::exportFile);

	QAction *exportActionPDF = new QAction(tr("Export to &PDF..."));
	m_parent->getFileMenu()->insertAction(exportAction, exportActionPDF);
	connect(exportActionPDF, &QAction::triggered, this, &qvgeNodeEditorUIController::exportPDF);

	QAction *exportActionDOT = new QAction(tr("Export to &DOT/GraphViz..."));
	m_parent->getFileMenu()->insertAction(exportActionPDF, exportActionDOT);
	connect(exportActionDOT, &QAction::triggered, this, &qvgeNodeEditorUIController::exportDOT);

	m_parent->getFileMenu()->insertSeparator(exportActionDOT);


	// add edit menu
	QMenu *editMenu = new QMenu(tr("&Edit"));
	m_parent->menuBar()->insertMenu(m_parent->getWindowMenuAction(), editMenu);

	QAction *undoAction = editMenu->addAction(QIcon(":/Icons/Undo"), tr("&Undo"));
	undoAction->setStatusTip(tr("Undo latest action"));
	undoAction->setShortcut(QKeySequence::Undo);
	connect(undoAction, &QAction::triggered, m_editorScene, &CEditorScene::undo);
	connect(m_editorScene, &CEditorScene::undoAvailable, undoAction, &QAction::setEnabled);
	undoAction->setEnabled(m_editorScene->availableUndoCount());

	QAction *redoAction = editMenu->addAction(QIcon(":/Icons/Redo"), tr("&Redo"));
	redoAction->setStatusTip(tr("Redo latest action"));
	redoAction->setShortcut(QKeySequence::Redo);
	connect(redoAction, &QAction::triggered, m_editorScene, &CEditorScene::redo);
	connect(m_editorScene, &CEditorScene::redoAvailable, redoAction, &QAction::setEnabled);
	redoAction->setEnabled(m_editorScene->availableRedoCount());

	editMenu->addSeparator();

	cutAction = editMenu->addAction(QIcon(":/Icons/Cut"), tr("Cu&t"));
	cutAction->setStatusTip(tr("Cut selection to clipboard"));
	cutAction->setShortcut(QKeySequence::Cut);
	connect(cutAction, &QAction::triggered, m_editorScene, &CEditorScene::cut);

	copyAction = editMenu->addAction(QIcon(":/Icons/Copy"), tr("&Copy"));
	copyAction->setStatusTip(tr("Copy selection to clipboard"));
	copyAction->setShortcut(QKeySequence::Copy);
	connect(copyAction, &QAction::triggered, m_editorScene, &CEditorScene::copy);

	pasteAction = editMenu->addAction(QIcon(":/Icons/Paste"), tr("&Paste"));
	pasteAction->setStatusTip(tr("Paste selection from clipboard"));
	pasteAction->setShortcut(QKeySequence::Paste);
	connect(pasteAction, &QAction::triggered, m_editorScene, &CEditorScene::paste);

	delAction = editMenu->addAction(QIcon(":/Icons/Delete"), tr("&Delete"));
	delAction->setStatusTip(tr("Delete selection"));
	delAction->setShortcut(QKeySequence::Delete);
	connect(delAction, &QAction::triggered, m_editorScene, &CEditorScene::del);

	editMenu->addSeparator();

	linkAction = editMenu->addAction(QIcon(":/Icons/Link"), tr("&Link"));
	linkAction->setStatusTip(tr("Link selected nodes together"));
	connect(linkAction, &QAction::triggered, m_editorScene, &CNodeEditorScene::onActionLink);

	unlinkAction = editMenu->addAction(QIcon(":/Icons/Unlink"), tr("&Unlink"));
	unlinkAction->setStatusTip(tr("Unlink selected nodes"));
	connect(unlinkAction, &QAction::triggered, m_editorScene, &CNodeEditorScene::onActionUnlink);

	// scene actions
	editMenu->addSeparator();

	QAction *sceneCropAction = editMenu->addAction(QIcon(":/Icons/Crop"), tr("&Crop Area"));
	sceneCropAction->setStatusTip(tr("Crop document area to contents"));
	connect(sceneCropAction, &QAction::triggered, this, &qvgeNodeEditorUIController::sceneCrop);

	// scene options
	editMenu->addSeparator();

	QAction *sceneAction = editMenu->addAction(QIcon(":/Icons/Settings"), tr("&Options..."));
	sceneAction->setStatusTip(tr("Set up the scene"));
	connect(sceneAction, &QAction::triggered, this, &qvgeNodeEditorUIController::sceneOptions);


	// add edit toolbar
	QToolBar *editToolbar = m_parent->addToolBar(tr("Edit"));
    editToolbar->setObjectName("editToolbar");
	editToolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

	editToolbar->addAction(undoAction);
	editToolbar->addAction(redoAction);

	editToolbar->addSeparator();

	editToolbar->addAction(cutAction);
	editToolbar->addAction(copyAction);
	editToolbar->addAction(pasteAction);
	editToolbar->addAction(delAction);


	// add view menu
	QMenu *viewMenu = new QMenu(tr("&View"));
	m_parent->menuBar()->insertMenu(m_parent->getWindowMenuAction(), viewMenu);

    gridAction = viewMenu->addAction(QIcon(":/Icons/Grid-Show"), tr("Show &Grid"));
	gridAction->setCheckable(true);
	gridAction->setStatusTip(tr("Show/hide background grid"));
	gridAction->setChecked(m_editorScene->gridEnabled());
	connect(gridAction, SIGNAL(toggled(bool)), m_editorScene, SLOT(enableGrid(bool)));

    gridSnapAction = viewMenu->addAction(QIcon(":/Icons/Grid-Snap"), tr("&Snap to Grid"));
	gridSnapAction->setCheckable(true);
	gridSnapAction->setStatusTip(tr("Snap to grid when dragging"));
	gridSnapAction->setChecked(m_editorScene->gridSnapEnabled());
	connect(gridSnapAction, SIGNAL(toggled(bool)), m_editorScene, SLOT(enableGridSnap(bool)));

    actionShowLabels = viewMenu->addAction(QIcon(":/Icons/Label"), tr("Show &Labels"));
	actionShowLabels->setCheckable(true);
	actionShowLabels->setStatusTip(tr("Show/hide item labels"));
	actionShowLabels->setChecked(m_editorScene->itemLabelsEnabled());
	connect(actionShowLabels, SIGNAL(toggled(bool)), m_editorScene, SLOT(enableItemLabels(bool)));

	viewMenu->addSeparator();

	zoomAction = viewMenu->addAction(QIcon(":/Icons/ZoomIn"), tr("&Zoom"));
	zoomAction->setStatusTip(tr("Zoom view in"));
	zoomAction->setShortcut(QKeySequence::ZoomIn);
	connect(zoomAction, &QAction::triggered, this, &qvgeNodeEditorUIController::zoom);

	unzoomAction = viewMenu->addAction(QIcon(":/Icons/ZoomOut"), tr("&Unzoom"));
	unzoomAction->setStatusTip(tr("Zoom view out"));
	unzoomAction->setShortcut(QKeySequence::ZoomOut);
	connect(unzoomAction, &QAction::triggered, this, &qvgeNodeEditorUIController::unzoom);

	resetZoomAction = viewMenu->addAction(QIcon(":/Icons/ZoomReset"), tr("&Reset Zoom"));
	resetZoomAction->setStatusTip(tr("Zoom view to 100%"));
	connect(resetZoomAction, &QAction::triggered, this, &qvgeNodeEditorUIController::resetZoom);

	fitZoomAction = viewMenu->addAction(QIcon(":/Icons/ZoomFit"), tr("&Fit to View"));
	fitZoomAction->setStatusTip(tr("Zoom to fit all the items to view"));
	connect(fitZoomAction, &QAction::triggered, m_editorView, &CEditorView::fitToView);


	// add view toolbar
	QToolBar *zoomToolbar = m_parent->addToolBar(tr("View"));
    zoomToolbar->setObjectName("viewToolbar");
	zoomToolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

	zoomToolbar->addAction(zoomAction);

	resetZoomAction2 = zoomToolbar->addAction(QIcon(":/Icons/Zoom"), "");
	resetZoomAction2->setStatusTip(resetZoomAction->statusTip());
	resetZoomAction2->setToolTip(resetZoomAction->statusTip());
	connect(resetZoomAction2, &QAction::triggered, this, &qvgeNodeEditorUIController::resetZoom);

	zoomToolbar->addAction(unzoomAction);
	zoomToolbar->addAction(fitZoomAction);
}


void qvgeNodeEditorUIController::createPanels()
{
	// propertis
    QDockWidget *propertyDock = new QDockWidget(tr("Item Properties"));
    propertyDock->setObjectName("propertyDock");
	m_parent->addDockWidget(Qt::RightDockWidgetArea, propertyDock);

    CNodeEdgePropertiesUI *propertiesPanel = new CNodeEdgePropertiesUI(propertyDock);
    propertiesPanel->setScene(m_editorScene);
    propertyDock->setWidget(propertiesPanel);

	// connections
    QDockWidget *connectionsDock = new QDockWidget(tr("Topology"));
    connectionsDock->setObjectName("connectionsDock");
	m_parent->addDockWidget(Qt::RightDockWidgetArea, connectionsDock);

    CCommutationTable *connectionsPanel = new CCommutationTable(connectionsDock);
	connectionsDock->setWidget(connectionsPanel);
	connectionsPanel->setScene(m_editorScene);

    // default properties
    QDockWidget *defaultsDock = new QDockWidget(tr("Default Properties"));
    defaultsDock ->setObjectName("defaultsDock");
    m_parent->addDockWidget(Qt::LeftDockWidgetArea, defaultsDock);

    CClassAttributesEditorUI *defaultsPanel = new CClassAttributesEditorUI(defaultsDock);
    defaultsPanel->setScene(m_editorScene);
    defaultsDock->setWidget(defaultsPanel);
}


void qvgeNodeEditorUIController::createNavigator()
{
    m_sliderView = new QSint::Slider2d(m_parent);
    m_sliderView->connectSource(m_editorView);

    QToolButton *sliderButton = m_sliderView->makeAsButton();
    m_editorView->setCornerWidget(sliderButton);

	sliderButton->setIcon(QIcon(":/Icons/Navigator"));
    sliderButton->setToolTip(tr("Show scene navigator"));
    connect(m_sliderView, SIGNAL(aboutToShow()), this, SLOT(onNavigatorShown()));

    m_sliderView->setFixedSize(200,200);
    m_sliderView->setSliderOpacity(0.3);
    m_sliderView->setSliderBrush(Qt::green);
}


void qvgeNodeEditorUIController::onNavigatorShown()
{
    double w = m_editorScene->sceneRect().width();
    double h = m_editorScene->sceneRect().height();
    double cw = w > h ? 200.0 : 200.0 * (w/h);
    double ch = h > w ? 200.0 : 200.0 * (h/w) ;
    m_sliderView->setFixedSize(cw, ch);

    // Qt bug: update menu size
    QResizeEvent re(m_sliderView->size(), m_sliderView->parentWidget()->size());
    qApp->sendEvent(m_sliderView->parentWidget(), &re);

    QPixmap pm(m_sliderView->size());
    QPainter p(&pm);
	bool gridOn = m_editorScene->gridEnabled();
	bool labelsOn = m_editorScene->itemLabelsEnabled();
	m_editorScene->enableGrid(false);
	m_editorScene->enableItemLabels(false);
    m_editorScene->render(&p);
	m_editorScene->enableGrid(gridOn);
	m_editorScene->enableItemLabels(labelsOn);
    m_sliderView->setBackgroundBrush(pm);
}


qvgeNodeEditorUIController::~qvgeNodeEditorUIController() 
{
}


void qvgeNodeEditorUIController::onSelectionChanged()
{
	int selectionCount = m_editorScene->selectedItems().size();

	cutAction->setEnabled(selectionCount > 0);
	copyAction->setEnabled(selectionCount > 0);
	delAction->setEnabled(selectionCount > 0);

	auto nodes = m_editorScene->getSelectedItems<CNode>();
	linkAction->setEnabled(nodes.size() > 1);
	unlinkAction->setEnabled(nodes.size() > 0);
}


void qvgeNodeEditorUIController::onSceneChanged()
{
    auto nodes = m_editorScene->getItems<CNode>();
    auto edges = m_editorScene->getItems<CConnection>();

    m_statusLabel->setText(tr("Nodes: %1 | Edges: %2").arg(nodes.size()).arg(edges.size()));
}


void qvgeNodeEditorUIController::onZoomChanged(double currentZoom)
{
	resetZoomAction2->setText(QString("%1%").arg((int)(currentZoom * 100)));
}


void qvgeNodeEditorUIController::zoom()
{
	m_editorView->zoomBy(1.3);
}


void qvgeNodeEditorUIController::unzoom()
{
	m_editorView->zoomBy(1.0 / 1.3);
}


void qvgeNodeEditorUIController::resetZoom()
{
	m_editorView->zoomTo(1.0);
}


void qvgeNodeEditorUIController::sceneCrop()
{
	QRectF itemsRect = m_editorScene->itemsBoundingRect().adjusted(-20, -20, 20, 20);
	if (itemsRect == m_editorScene->sceneRect())
		return;

	// update scene rect
	m_editorScene->setSceneRect(itemsRect);

	m_editorScene->addUndoState();
}


void qvgeNodeEditorUIController::sceneOptions()
{
    CSceneOptionsDialog dialog;
    if (dialog.exec(*m_editorScene, *m_editorView))
    {
        gridAction->setChecked(m_editorScene->gridEnabled());
        gridSnapAction->setChecked(m_editorScene->gridSnapEnabled());
        actionShowLabels->setChecked(m_editorScene->itemLabelsEnabled());

		m_parent->writeSettings();
    }
}


bool qvgeNodeEditorUIController::doExport(const IFileSerializer &exporter)
{
	QString fileName = CUtils::cutLastSuffix(m_parent->getCurrentFileName());
	if (fileName.isEmpty())
		fileName = m_lastExportPath;
	else
		fileName = QFileInfo(m_lastExportPath).absolutePath() + "/" + QFileInfo(fileName).fileName();

	QString path = QFileDialog::getSaveFileName(NULL,
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


void qvgeNodeEditorUIController::exportFile()
{
	doExport(CImageExport());
}


void qvgeNodeEditorUIController::exportDOT()
{
	doExport(CFileSerializerDOT());
}


void qvgeNodeEditorUIController::exportPDF()
{
	doExport(CPDFExport());
}


void qvgeNodeEditorUIController::doReadSettings(QSettings& settings)
{
	bool isAA = m_editorView->renderHints().testFlag(QPainter::Antialiasing);
	isAA = settings.value("antialiasing", isAA).toBool();
	m_editorView->setRenderHint(QPainter::Antialiasing, isAA);
	m_editorScene->setFontAntialiased(isAA);

	int cacheRam = QPixmapCache::cacheLimit();
	cacheRam = settings.value("cacheRam", cacheRam).toInt();
	QPixmapCache::setCacheLimit(cacheRam);


	m_lastExportPath = settings.value("lastExportPath", m_lastExportPath).toString();
}


void qvgeNodeEditorUIController::doWriteSettings(QSettings& settings)
{
	bool isAA = m_editorView->renderHints().testFlag(QPainter::Antialiasing);
	settings.setValue("antialiasing", isAA);

	int cacheRam = QPixmapCache::cacheLimit();
	settings.setValue("cacheRam", cacheRam);


	settings.setValue("lastExportPath", m_lastExportPath);
}


bool qvgeNodeEditorUIController::loadFromFile(const QString &fileName, const QString &format)
{
    if (format == "xgr")
    {
        return (CFileSerializerXGR().load(fileName, *m_editorScene));
    }

	if (format == "graphml")
	{
        return (CFileSerializerGraphML().load(fileName, *m_editorScene));
	}

	if (format == "gexf")
	{
        return (CFileSerializerGEXF().load(fileName, *m_editorScene));
	}

    // else via ogdf
    return (COGDFLayout::loadGraph(fileName.toStdString(), *m_editorScene));
}


bool qvgeNodeEditorUIController::saveToFile(const QString &fileName, const QString &format)
{
    if (format == "xgr")
        return (CFileSerializerXGR().save(fileName, *m_editorScene));

    if (format == "dot")
        return (CFileSerializerDOT().save(fileName, *m_editorScene));

	if (format == "gexf")
		return (CFileSerializerGEXF().save(fileName, *m_editorScene));

    return false;
}


void qvgeNodeEditorUIController::onNewDocumentCreated()
{
    COGDFNewGraphDialog dialog;
    if (dialog.exec(*m_editorScene))
    {
        // update scene info
        //onSceneChanged();
    }
}
