#include "COGDFLayoutUIController.h"
#include "COGDFLayout.h"
#include "COGDFNewGraphDialog.h"

#include <appbase/CMainWindow.h>

#include <qvge/CNodeEditorScene.h>
#include <qvge/CEditorView.h>

#include <ogdf/planarity/PlanarizationLayout.h>
#include <ogdf/misclayout/LinearLayout.h>
#include <ogdf/misclayout/BalloonLayout.h>
#include <ogdf/misclayout/CircularLayout.h>
#include <ogdf/energybased/FMMMLayout.h>
#include <ogdf/energybased/DavidsonHarelLayout.h>
#include <ogdf/layered/SugiyamaLayout.h>

#include <ogdf/tree/TreeLayout.h>	// crashes
#include <ogdf/tree/RadialTreeLayout.h>	// crashes
#include <ogdf/planarlayout/PlanarStraightLayout.h>	// crashes
#include <ogdf/planarlayout/SchnyderLayout.h> // crashes

#include <QMenuBar>
#include <QMenu>


COGDFLayoutUIController::COGDFLayoutUIController(CMainWindow *parent, CNodeEditorScene *scene) :
    QObject(parent),
    m_parent(parent), m_scene(scene)
{
    // add layout menu
    QMenu *layoutMenu = new QMenu(tr("&OGDF"));
    m_parent->menuBar()->insertMenu(m_parent->getWindowMenuAction(), layoutMenu);

    layoutMenu->addAction(tr("Linear Layout"), this, SLOT(doLinearLayout()));
    layoutMenu->addAction(tr("Balloon Layout"), this, SLOT(doBalloonLayout()));
    layoutMenu->addAction(tr("Circular Layout"), this, SLOT(doCircularLayout()));
    //layoutMenu->addAction(tr("Tree Layout"), this, SLOT(doTreeLayout()));
    layoutMenu->addAction(tr("FMMM Layout"), this, SLOT(doFMMMLayout()));
	layoutMenu->addAction(tr("Planar Layout"), this, SLOT(doPlanarLayout()));
	layoutMenu->addAction(tr("Davidson-Harel Layout"), this, SLOT(doDHLayout()));
	layoutMenu->addAction(tr("Sugiyama Layout"), this, SLOT(doSugiyamaLayout()));

	layoutMenu->addSeparator();
	layoutMenu->addAction(tr("Create new graph..."), this, SLOT(createNewGraph()));
}


void COGDFLayoutUIController::createNewGraph()
{
	COGDFNewGraphDialog dialog;
	dialog.exec(*m_scene);
}


void COGDFLayoutUIController::doPlanarLayout()
{
    ogdf::PlanarizationLayout layout;
    COGDFLayout::doLayout(layout, *m_scene);
    Q_EMIT layoutFinished();
}


void COGDFLayoutUIController::doLinearLayout()
{
    ogdf::LinearLayout layout;
    COGDFLayout::doLayout(layout, *m_scene);
    Q_EMIT layoutFinished();
}


void COGDFLayoutUIController::doBalloonLayout()
{
    ogdf::BalloonLayout layout;
    COGDFLayout::doLayout(layout, *m_scene);
    Q_EMIT layoutFinished();
}


void COGDFLayoutUIController::doCircularLayout()
{
    ogdf::CircularLayout layout;
    COGDFLayout::doLayout(layout, *m_scene);
    Q_EMIT layoutFinished();
}


void COGDFLayoutUIController::doFMMMLayout()
{
	ogdf::FMMMLayout layout;
    COGDFLayout::doLayout(layout, *m_scene);
    Q_EMIT layoutFinished();
}


void COGDFLayoutUIController::doTreeLayout()
{
	ogdf::RadialTreeLayout layout;	// crashing
	COGDFLayout::doLayout(layout, *m_scene);
    Q_EMIT layoutFinished();
}


void COGDFLayoutUIController::doDHLayout()
{
	ogdf::DavidsonHarelLayout layout;
	//layout.setSpeed(ogdf::DavidsonHarelLayout::SpeedParameter::Fast);
	//layout.fixSettings(ogdf::DavidsonHarelLayout::SettingsParameter::Repulse);
	COGDFLayout::doLayout(layout, *m_scene);
    Q_EMIT layoutFinished();
}


void COGDFLayoutUIController::doSugiyamaLayout()
{
	ogdf::SugiyamaLayout layout;
	COGDFLayout::doLayout(layout, *m_scene);
    Q_EMIT layoutFinished();
}
