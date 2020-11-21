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


QString CGVGraphLayoutUIController::errorNotWritable(const QString &path) const
{
	return QObject::tr("Cannot create GraphViz output in %1. Check if the directory is writable.").arg(path);
}


QString CGVGraphLayoutUIController::errorCannotRun(const QString &path) const
{
	return QObject::tr("Cannot run %1. Check if GraphViz has been correctly installed.").arg(path);
}


bool CGVGraphLayoutUIController::doRunDOT(const QString &engine, const QString &dotFilePath, QString &plainFilePath, QString* lastError /*= nullptr*/)
{
	// run dot to convert filename.dot -> filename.temp.plain	
	QTemporaryFile tempFile(QDir::tempPath() + "/qvge-XXXXXX.plain");
	if (!tempFile.open())
	{
		if (lastError)
			*lastError = errorNotWritable(QDir::tempPath());

		return false;
	}

	plainFilePath = tempFile.fileName();
	tempFile.setAutoRemove(false);

	QString pathToDot = "dot";
	if (m_pathToGraphviz.size())
		pathToDot = m_pathToGraphviz + "/dot";

	QString cmd = QString("\"%1\" -K\"%2\" -Tplain-ext \"%3\" -o\"%4\"").arg(pathToDot, engine).arg(dotFilePath).arg(plainFilePath);

	QProcess process;
	process.setWorkingDirectory(m_pathToGraphviz);
	int res = QProcess::execute(cmd);
	if (res != 0)
	{
		if (lastError)
			*lastError = errorCannotRun(pathToDot);

		return false;
	}

	// run successful
	return true;
}


bool CGVGraphLayoutUIController::loadGraph(const QString &filename, CNodeEditorScene &scene, QString* lastError /*= nullptr*/)
{
	// run dot to convert filename.dot -> filename.temp.plain	
	QString plainFilePath;
	if (!doRunDOT(m_defaultEngine, filename, plainFilePath, lastError))
		return false;

	// import generated plain text
	bool ok = CFileSerializerPlainDOT().load(plainFilePath, scene, lastError);
	if (ok)
		Q_EMIT loadFinished();
	else
		if (lastError)
			*lastError = tr("Cannot load file content");

	QFile::remove(plainFilePath);
	return ok;
}


bool CGVGraphLayoutUIController::doLayout(const QString &engine, CNodeEditorScene &scene)
{
	QString lastError;
	QString plainFilePath;

	// export to dot
	QTemporaryFile tempFile(QDir::tempPath() + "/qvge-XXXXXX.dot");
	if (!tempFile.open())
	{
		lastError = errorNotWritable(QDir::tempPath());
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
	if (!doRunDOT(engine, tempFile.fileName(), plainFilePath, &lastError))
	{
		QMessageBox::critical(m_parent, tr("Layout failed"), lastError);
		return false;
	}

	// import layout only
	CFormatPlainDOT graphFormat;
	Graph graphModel;
	if (!graphFormat.load(plainFilePath, graphModel, &lastError))
	{
		QFile::remove(plainFilePath);

		QMessageBox::critical(m_parent, tr("Layout failed"), lastError);
		return false;
	}

	// update node positions
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

	Q_EMIT layoutFinished();

	QFile::remove(plainFilePath);

	return true;
}

