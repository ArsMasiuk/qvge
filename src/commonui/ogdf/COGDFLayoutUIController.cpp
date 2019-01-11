#include "COGDFLayoutUIController.h"
#include "COGDFLayout.h"

#include <appbase/CMainWindow.h>

#include <qvge/CNodeEditorScene.h>
#include <qvge/CEditorView.h>

#include <ogdf/planarity/PlanarizationLayout.h>
#include <ogdf/misclayout/LinearLayout.h>
#include <ogdf/misclayout/BalloonLayout.h>
#include <ogdf/misclayout/CircularLayout.h>
//#include <ogdf/tree/TreeLayout.h>
//#include <ogdf/tree/RadialTreeLayout.h>
#include <ogdf/energybased/FMMMLayout.h>
#include <ogdf/planarlayout/PlanarStraightLayout.h>
#include <ogdf/layered/SugiyamaLayout.h>

#include <QMenuBar>
#include <QMenu>


COGDFLayoutUIController::COGDFLayoutUIController(CMainWindow *parent, CNodeEditorScene *scene) :
    QObject(parent),
    m_parent(parent), m_scene(scene)
{
    // add layout menu
    QMenu *layoutMenu = new QMenu(tr("&Layout"));
    m_parent->menuBar()->insertMenu(m_parent->getWindowMenuAction(), layoutMenu);

    layoutMenu->addAction(tr("Linear Layout"), this, SLOT(doLinearLayout()));
    layoutMenu->addAction(tr("Balloon Layout"), this, SLOT(doBalloonLayout()));
    layoutMenu->addAction(tr("Circular Layout"), this, SLOT(doCircularLayout()));
    layoutMenu->addAction(tr("FMMM Layout"), this, SLOT(doFMMMLayout()));
	layoutMenu->addAction(tr("Planar Layout"), this, SLOT(doPlanarLayout()));
	//layoutMenu->addAction(tr("PSL Layout"), this, SLOT(doPSLLayout()));
	layoutMenu->addAction(tr("Sugiyama Layout"), this, SLOT(doSugiyamaLayout()));
}


void COGDFLayoutUIController::doPlanarLayout()
{
    ogdf::PlanarizationLayout layout;
    COGDFLayout::doLayout(layout, *m_scene);
}


void COGDFLayoutUIController::doLinearLayout()
{
    ogdf::LinearLayout layout;
	//layout.setCustomOrder(true);
    COGDFLayout::doLayout(layout, *m_scene);
}


void COGDFLayoutUIController::doBalloonLayout()
{
    ogdf::BalloonLayout layout;
    COGDFLayout::doLayout(layout, *m_scene);
}


void COGDFLayoutUIController::doCircularLayout()
{
    ogdf::CircularLayout layout;
    COGDFLayout::doLayout(layout, *m_scene);
}


void COGDFLayoutUIController::doFMMMLayout()
{
    //ogdf::TreeLayout layout;	// crashing

	ogdf::FMMMLayout layout;
    COGDFLayout::doLayout(layout, *m_scene);
}


void COGDFLayoutUIController::doPSLLayout()
{
	ogdf::PlanarStraightLayout layout;	// freezing
	COGDFLayout::doLayout(layout, *m_scene);
}


void COGDFLayoutUIController::doSugiyamaLayout()
{
	ogdf::SugiyamaLayout layout;
	COGDFLayout::doLayout(layout, *m_scene);
}
