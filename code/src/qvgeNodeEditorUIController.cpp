/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "qvgeNodeEditorUIController.h"

#include <QMenuBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QMenu>
#include <QToolButton>
#include <QWidgetAction>
#include <QResizeEvent>
#include <QDebug>

#include <qvge/CNode.h>
#include <qvge/CConnection.h>
#include <qvge/CImageExport.h>
#include <qvge/CPDFExport.h>

#include <CCommutationTable.h>
#include <CSceneOptionsDialog.h>
#include <CNodeEdgePropertiesUI.h>


qvgeNodeEditorUIController::qvgeNodeEditorUIController(CMainWindow *parent, CNodeEditorScene *scene, CEditorView *view) : 
	QObject(parent),
	m_parent(parent), m_scene(scene), m_editorView(view)
{
	// connect scene
	connect(scene, &CEditorScene::sceneChanged, parent, &CMainWindow::onDocumentChanged);
    connect(scene, &CEditorScene::sceneChanged, this, &qvgeNodeEditorUIController::onSceneChanged);
	connect(scene, &CEditorScene::selectionChanged, this, &qvgeNodeEditorUIController::onSelectionChanged);

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

	m_parent->getFileMenu()->insertSeparator(exportActionPDF);


	// add edit menu
	QMenu *editMenu = new QMenu(tr("&Edit"));
	m_parent->menuBar()->insertMenu(m_parent->getWindowMenuAction(), editMenu);

	QAction *undoAction = editMenu->addAction(QIcon(":/Icons/Undo"), tr("&Undo"));
	undoAction->setStatusTip(tr("Undo latest action"));
	undoAction->setShortcut(QKeySequence::Undo);
	connect(undoAction, &QAction::triggered, m_scene, &CEditorScene::undo);
	connect(m_scene, &CEditorScene::undoAvailable, undoAction, &QAction::setEnabled);
	undoAction->setEnabled(m_scene->availableUndoCount());

	QAction *redoAction = editMenu->addAction(QIcon(":/Icons/Redo"), tr("&Redo"));
	redoAction->setStatusTip(tr("Redo latest action"));
	redoAction->setShortcut(QKeySequence::Redo);
	connect(redoAction, &QAction::triggered, m_scene, &CEditorScene::redo);
	connect(m_scene, &CEditorScene::redoAvailable, redoAction, &QAction::setEnabled);
	redoAction->setEnabled(m_scene->availableRedoCount());

	editMenu->addSeparator();

	cutAction = editMenu->addAction(QIcon(":/Icons/Cut"), tr("Cu&t"));
	cutAction->setStatusTip(tr("Cut selection to clipboard"));
	cutAction->setShortcut(QKeySequence::Cut);
	connect(cutAction, &QAction::triggered, m_scene, &CEditorScene::cut);

	copyAction = editMenu->addAction(QIcon(":/Icons/Copy"), tr("&Copy"));
	copyAction->setStatusTip(tr("Copy selection to clipboard"));
	copyAction->setShortcut(QKeySequence::Copy);
	connect(copyAction, &QAction::triggered, m_scene, &CEditorScene::copy);

	pasteAction = editMenu->addAction(QIcon(":/Icons/Paste"), tr("&Paste"));
	pasteAction->setStatusTip(tr("Paste selection from clipboard"));
	pasteAction->setShortcut(QKeySequence::Paste);
	connect(pasteAction, &QAction::triggered, m_scene, &CEditorScene::paste);

	delAction = editMenu->addAction(QIcon(":/Icons/Delete"), tr("&Delete"));
	delAction->setStatusTip(tr("Delete selection"));
	delAction->setShortcut(QKeySequence::Delete);
	connect(delAction, &QAction::triggered, m_scene, &CEditorScene::del);

	editMenu->addSeparator();

	unlinkAction = editMenu->addAction(QIcon(":/Icons/Unlink"), tr("&Unlink"));
	unlinkAction->setStatusTip(tr("Unlink selected nodes"));
	connect(unlinkAction, &QAction::triggered, m_scene, &CNodeEditorScene::onActionUnlink);

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
	gridAction->setChecked(m_scene->gridEnabled());
	connect(gridAction, SIGNAL(toggled(bool)), m_scene, SLOT(enableGrid(bool)));

    gridSnapAction = viewMenu->addAction(QIcon(":/Icons/Grid-Snap"), tr("&Snap to Grid"));
	gridSnapAction->setCheckable(true);
	gridSnapAction->setStatusTip(tr("Snap to grid when dragging"));
	gridSnapAction->setChecked(m_scene->gridSnapEnabled());
	connect(gridSnapAction, SIGNAL(toggled(bool)), m_scene, SLOT(enableGridSnap(bool)));

    actionShowLabels = viewMenu->addAction(QIcon(":/Icons/Label"), tr("Show &Labels"));
	actionShowLabels->setCheckable(true);
	actionShowLabels->setStatusTip(tr("Show/hide item labels"));
	actionShowLabels->setChecked(m_scene->itemLabelsEnabled());
	connect(actionShowLabels, SIGNAL(toggled(bool)), m_scene, SLOT(enableItemLabels(bool)));

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

	viewMenu->addSeparator();

	// scene options
	QAction *sceneAction = viewMenu->addAction(tr("&Options..."));
	sceneAction->setStatusTip(tr("Set up the scene"));
	connect(sceneAction, &QAction::triggered, this, &qvgeNodeEditorUIController::sceneOptions);


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
	QDockWidget *propertyDock = new QDockWidget(tr("Properties"));
    propertyDock->setObjectName("propertyDock");
	m_parent->addDockWidget(Qt::RightDockWidgetArea, propertyDock);

    CNodeEdgePropertiesUI *propertiesPanel = new CNodeEdgePropertiesUI(propertyDock);
    propertiesPanel->setScene(m_scene);
    propertyDock->setWidget(propertiesPanel);

	// connections
    QDockWidget *connectionsDock = new QDockWidget(tr("Topology"));
    connectionsDock->setObjectName("connectionsDock");
	m_parent->addDockWidget(Qt::RightDockWidgetArea, connectionsDock);

    CCommutationTable *connectionsPanel = new CCommutationTable(connectionsDock);
	connectionsDock->setWidget(connectionsPanel);
	connectionsPanel->setScene(m_scene);
}


void qvgeNodeEditorUIController::createNavigator()
{
    m_sliderView = new QSint::Slider2d(m_parent);
    m_sliderView->connectSource(m_editorView);

    QToolButton *sliderButton = m_sliderView->makeAsButton();
    m_editorView->setCornerWidget(sliderButton);

    sliderButton->setToolTip(tr("Show scene navigator"));
    connect(m_sliderView, SIGNAL(aboutToShow()), this, SLOT(onNavigatorShown()));

    m_sliderView->setFixedSize(200,200);
    m_sliderView->setSliderOpacity(0.3);
    m_sliderView->setSliderBrush(Qt::green);
}


void qvgeNodeEditorUIController::onNavigatorShown()
{
    double w = m_scene->sceneRect().width();
    double h = m_scene->sceneRect().height();
    double cw = w > h ? 200.0 : 200.0 * (w/h);
    double ch = h > w ? 200.0 : 200.0 * (h/w) ;
    m_sliderView->setFixedSize(cw, ch);

    // Qt bug: update menu size
    QResizeEvent re(m_sliderView->size(), m_sliderView->parentWidget()->size());
    qApp->sendEvent(m_sliderView->parentWidget(), &re);

    QPixmap pm(m_sliderView->size());
    QPainter p(&pm);
    m_scene->render(&p);
    m_sliderView->setBackgroundBrush(pm);
}


qvgeNodeEditorUIController::~qvgeNodeEditorUIController() 
{
}


void qvgeNodeEditorUIController::onSelectionChanged()
{
	int selectionCount = m_scene->selectedItems().size();

	cutAction->setEnabled(selectionCount > 0);
	copyAction->setEnabled(selectionCount > 0);
	delAction->setEnabled(selectionCount > 0);

	auto nodes = m_scene->getSelectedItems<CNode>();
	unlinkAction->setEnabled(nodes.size() > 0);
}


void qvgeNodeEditorUIController::onSceneChanged()
{
    auto nodes = m_scene->getItems<CNode>();
    auto edges = m_scene->getItems<CConnection>();

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


void qvgeNodeEditorUIController::sceneOptions()
{
    CSceneOptionsDialog dialog;
    if (dialog.exec(*m_scene))
    {
        gridAction->setChecked(m_scene->gridEnabled());
        gridSnapAction->setChecked(m_scene->gridSnapEnabled());
        actionShowLabels->setChecked(m_scene->itemLabelsEnabled());
    }
}


void qvgeNodeEditorUIController::exportFile()
{
	if (CImageExport::write(*m_scene, m_parent->getCurrentFileName()))
	{
		m_parent->statusBar()->showMessage(tr("Export successful"));
	}
	else
	{
		m_parent->statusBar()->showMessage(tr("Export failed"));
	}
}


void qvgeNodeEditorUIController::exportPDF()
{
	if (CPDFExport::write(*m_scene, m_parent->getCurrentFileName()))
	{
		m_parent->statusBar()->showMessage(tr("Export successful"));
	}
	else
	{
		m_parent->statusBar()->showMessage(tr("Export failed"));
	}
}
