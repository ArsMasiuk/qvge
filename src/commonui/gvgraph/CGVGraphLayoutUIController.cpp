#include "CGVGraphLayoutUIController.h"

#include <gvgraph/graph_wrapper.h>

#include <appbase/CMainWindow.h>

#include <qvge/CNodeEditorScene.h>
#include <qvge/CNode.h>
#include <qvge/CEdge.h>

#include <QMenuBar>
#include <QMenu>


CGVGraphLayoutUIController::CGVGraphLayoutUIController(CMainWindow *parent, CNodeEditorScene *scene) :
    QObject(parent),
    m_parent(parent), m_scene(scene)
{
    // add layout menu
    QMenu *layoutMenu = new QMenu(tr("&GVGraph"));
    m_parent->menuBar()->insertMenu(m_parent->getWindowMenuAction(), layoutMenu);

    layoutMenu->addAction(tr("DOT Layout"), this, SLOT(doDotLayout()));
    // layoutMenu->addAction(tr("Balloon Layout"), this, SLOT(doBalloonLayout()));
    // layoutMenu->addAction(tr("Circular Layout"), this, SLOT(doCircularLayout()));
    // //layoutMenu->addAction(tr("Tree Layout"), this, SLOT(doTreeLayout()));
    // layoutMenu->addAction(tr("FMMM Layout"), this, SLOT(doFMMMLayout()));
	// layoutMenu->addAction(tr("Planar Layout"), this, SLOT(doPlanarLayout()));
	// layoutMenu->addAction(tr("Davidson-Harel Layout"), this, SLOT(doDHLayout()));
	// layoutMenu->addAction(tr("Sugiyama Layout"), this, SLOT(doSugiyamaLayout()));
}


void CGVGraphLayoutUIController::doDotLayout()
{
    doLayout("dot", *m_scene);
}


bool CGVGraphLayoutUIController::doLayout(const QString &engine, CNodeEditorScene &scene)
{
	auto nodes = scene.getItems<CNode>();
	auto edges = scene.getItems<CEdge>();

	GVGraph gvg("test");

	return true;
}

