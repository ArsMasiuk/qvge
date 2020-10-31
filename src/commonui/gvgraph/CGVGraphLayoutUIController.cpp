#include "CGVGraphLayoutUIController.h"

#include <appbase/CMainWindow.h>

#include <qvge/CNodeEditorScene.h>
#include <qvge/CNode.h>
#include <qvge/CEdge.h>
#include <qvge/CFileSerializerPlainDOT.h>

#include <QMenuBar>
#include <QMenu>
#include <QProcess>
#include <QTemporaryFile>
#include <QCoreApplication>
#include <QDir>


CGVGraphLayoutUIController::CGVGraphLayoutUIController(CMainWindow *parent, CNodeEditorScene *scene) :
    m_parent(parent), m_scene(scene)
{
    // add layout menu
    QMenu *layoutMenu = new QMenu(tr("&GraphViz"));
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


bool CGVGraphLayoutUIController::loadGraph(const QString &filename, CNodeEditorScene &scene, QString* lastError /*= nullptr*/)
{
	// run dot to convert filename.dot -> filename.temp.plain	
	QString pathToDotDir = QCoreApplication::applicationDirPath() + "/../tools/graphviz";
	pathToDotDir = QFileInfo(pathToDotDir).canonicalFilePath();
	QString pathToDot = pathToDotDir + "/dot.exe";

	QTemporaryFile tempFile(QDir::tempPath() + "/qvge-XXXXXX.plain");
	if (!tempFile.open())
	{
		// lastError
		return false;
	}

	QString cmd = QString("%1 -Tplain-ext \"%2\" -o\"%3\"").arg(pathToDot).arg(filename).arg(tempFile.fileName());

	QProcess process;
	process.setWorkingDirectory(pathToDotDir);
	int res = QProcess::execute(cmd);
	if (res == 0)
	{
		// import generated plain text
		return CFileSerializerPlainDOT().load(tempFile.fileName(), scene, lastError);
	}

	// lastError
	return false;
}


void CGVGraphLayoutUIController::doDotLayout()
{
    doLayout("dot", *m_scene);
}


bool CGVGraphLayoutUIController::doLayout(const QString &engine, CNodeEditorScene &scene)
{
	auto nodes = scene.getItems<CNode>();
	auto edges = scene.getItems<CEdge>();

	//GVGraph gvg("test");

	//gvg.applyLayout();

	return true;
}

