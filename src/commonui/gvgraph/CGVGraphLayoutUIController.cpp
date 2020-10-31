#include "CGVGraphLayoutUIController.h"

#include <appbase/CMainWindow.h>

#include <qvge/CNodeEditorScene.h>
#include <qvge/CNode.h>
#include <qvge/CEdge.h>
#include <qvge/CFileSerializerDOT.h>
#include <qvge/CFileSerializerPlainDOT.h>
#include <qvgeio/CFormatPlainDOT.h>

#include <QMenuBar>
#include <QMenu>
#include <QProcess>
#include <QTemporaryFile>
#include <QCoreApplication>
#include <QDir>
#include <QMessageBox>


CGVGraphLayoutUIController::CGVGraphLayoutUIController(CMainWindow *parent, CNodeEditorScene *scene) :
    m_parent(parent), m_scene(scene),
	m_defaultEngine("dot")
{
    // add layout menu
    QMenu *layoutMenu = new QMenu(tr("&GraphViz"));
    m_parent->menuBar()->insertMenu(m_parent->getWindowMenuAction(), layoutMenu);

    layoutMenu->addAction(tr("Hierarchical Layout (dot default)"), this, SLOT(doDotLayout()));
    layoutMenu->addAction(tr("Spring Energy Layout (neato)"), this, SLOT(doNeatoLayout()));
    layoutMenu->addAction(tr("Spring Force Layout (fdp)"), this, SLOT(doFDPLayout()));
    layoutMenu->addAction(tr("Multiscaled Spring Force Layout (sfdp)"), this, SLOT(doSFDPLayout()));
    layoutMenu->addAction(tr("Radial Layout (twopi)"), this, SLOT(doTwopiLayout()));
    layoutMenu->addAction(tr("Circular Layout (circo)"), this, SLOT(doCircularLayout()));
}


void CGVGraphLayoutUIController::setPathToGraphviz(const QString &pathToGraphviz)
{
	m_pathToGraphviz = pathToGraphviz;
}


void CGVGraphLayoutUIController::setDefaultEngine(const QString &engine)
{
	if (!engine.isEmpty())
		m_defaultEngine = engine;
}


bool CGVGraphLayoutUIController::loadGraph(const QString &filename, CNodeEditorScene &scene, QString* lastError /*= nullptr*/)
{
	// run dot to convert filename.dot -> filename.temp.plain	
	QTemporaryFile tempFile(QDir::tempPath() + "/qvge-XXXXXX.plain");
	if (!tempFile.open())
	{
		if (lastError)
			*lastError = QObject::tr("Cannot create GraphViz output in %1. Check if the directory is writable.").arg(QDir::tempPath());

		return false;
	}

	QString pathToDot = m_defaultEngine;
	if (m_pathToGraphviz.size())
		pathToDot = m_pathToGraphviz + "/" + m_defaultEngine;

	QString cmd = QString("\"%1\" -Tplain-ext \"%2\" -o\"%3\"").arg(pathToDot).arg(filename).arg(tempFile.fileName());

	QProcess process;
	process.setWorkingDirectory(m_pathToGraphviz);
	int res = QProcess::execute(cmd);
	if (res == 0)
	{
		// import generated plain text
		return CFileSerializerPlainDOT().load(tempFile.fileName(), scene, lastError);
	}

	if (lastError)
		*lastError = QObject::tr("Cannot run %1. Check if GraphViz has been correctly installed.").arg(pathToDot);

	return false;
}


bool CGVGraphLayoutUIController::doLayout(const QString &engine, CNodeEditorScene &scene)
{
	QString lastError;

	// export to dot
	QTemporaryFile tempFile(QDir::tempPath() + "/qvge-XXXXXX.dot");
	if (!tempFile.open())
	{
		lastError = QObject::tr("Cannot create GraphViz output in %1. Check if the directory is writable.").arg(QDir::tempPath());
		QMessageBox::critical(m_parent, tr("Layout failed"), lastError);
		return false;
	}

	bool ok = CFileSerializerDOT().save(tempFile.fileName(), scene, &lastError);
	if (!ok)
	{
		QMessageBox::critical(m_parent, tr("Layout failed"), lastError);
		return false;
	}


	// convert dot -> plain
	QTemporaryFile tempFilePlain(QDir::tempPath() + "/qvge-XXXXXX.plain");
	if (!tempFilePlain.open())
	{
		lastError = QObject::tr("Cannot create GraphViz output in %1. Check if the directory is writable.").arg(QDir::tempPath());
		QMessageBox::critical(m_parent, tr("Layout failed"), lastError);
		return false;
	}

	QString pathToDot = engine;
	if (m_pathToGraphviz.size())
		pathToDot = m_pathToGraphviz + "/" + engine;

	QString cmd = QString("\"%1\" -Tplain-ext \"%2\" -o\"%3\"").arg(pathToDot).arg(tempFile.fileName()).arg(tempFilePlain.fileName());

	QProcess process;
	process.setWorkingDirectory(m_pathToGraphviz);
	int res = QProcess::execute(cmd);
	if (res != 0)
	{
		lastError = QObject::tr("Cannot run %1. Check if GraphViz has been correctly installed.").arg(pathToDot);
		QMessageBox::critical(m_parent, tr("Layout failed"), lastError);
		return false;
	}


	// import layout only
	CFormatPlainDOT graphFormat;
	Graph graphModel;
	if (!graphFormat.load(tempFilePlain.fileName(), graphModel, &lastError))
	{
		QMessageBox::critical(m_parent, tr("Layout failed"), lastError);
		return false;
	}

	auto nodes = scene.getItems<CNode>();
	for (auto & node : nodes)
	{
		int nodeIndex = graphModel.findNodeIndex(node->getId());
		if (nodeIndex >= 0)
		{
			const auto & attrs = graphModel.nodes.at(nodeIndex).attrs;
			node->setX(attrs["x"].toDouble());
			node->setY(attrs["y"].toDouble());
		}
	}

	scene.addUndoState();

	return true;
}

