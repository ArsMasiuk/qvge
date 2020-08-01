#include "CSceneMenuUIController.h"

#include <QMenu>
#include <QGraphicsItem>
#include <QGraphicsSceneContextMenuEvent>

#include <qvge/CEditorView.h>
#include <qvge/CNodeEditorScene.h>
#include <qvge/CNodeSceneActions.h>
#include <qvge/CNode.h>
#include <qvge/CEdge.h>


CSceneMenuUIController::CSceneMenuUIController(QObject *parent) : QObject(parent)
{
}


CSceneMenuUIController::~CSceneMenuUIController()
{
}


bool CSceneMenuUIController::exec(CEditorScene *scene, QGraphicsItem *triggerItem, QGraphicsSceneContextMenuEvent *contextMenuEvent)
{
	m_scene = dynamic_cast<CNodeEditorScene*>(scene);
	m_scene->setPastePosition(contextMenuEvent->scenePos());

	QMenu menu;
	fillMenu(menu, scene, triggerItem, contextMenuEvent);

	Q_EMIT onContextMenu(menu);

	// execute
	menu.exec(contextMenuEvent->screenPos());

	m_scene->setPastePosition(QPointF());
	return false;
}


void CSceneMenuUIController::fillMenu(QMenu &menu, CEditorScene *scene, QGraphicsItem *triggerItem, QGraphicsSceneContextMenuEvent* /*contextMenuEvent*/)
{
	auto sceneActions = scene->getActions();
	auto nodeScene = m_scene;

	int nodesCount = nodeScene->getSelectedNodes().size();
	bool nodesSelected = (nodesCount > 0);

	int edgesCount = nodeScene->getSelectedEdges().size();
	bool edgesSelected = (edgesCount > 0);

	// add default actions
	QAction *changeIdAction = menu.addAction(tr("Change Id..."), parent(), SLOT(changeItemId()));
	changeIdAction->setEnabled((nodesCount + edgesCount) == 1);

	menu.addSeparator();

	menu.addAction(scene->actions()->cutAction);
	menu.addAction(scene->actions()->copyAction);
	menu.addAction(scene->actions()->pasteAction);
	menu.addAction(scene->actions()->delAction);

	// add default node actions
	menu.addSeparator();

	QAction *linkAction = menu.addAction(tr("Link"), sceneActions, SLOT(onActionLink()));
	linkAction->setEnabled(nodesCount > 1);

	QAction *unlinkAction = menu.addAction(tr("Unlink"), sceneActions, SLOT(onActionUnlink()));
	unlinkAction->setEnabled(nodesSelected);

	QAction *nodeColorAction = menu.addAction(tr("Node(s) Color..."), sceneActions, SLOT(onActionNodeColor()));
	nodeColorAction->setEnabled(nodesSelected);

	//QAction *factorAction = menu.addAction(tr("Factor Nodes..."), parent(), SLOT(factorNodes()));
	//factorAction->setEnabled(nodesCount > 1);

	menu.addSeparator();

	QAction *addPortAction = menu.addAction(tr("Add Port..."), parent(), SLOT(addNodePort()));
	addPortAction->setEnabled(nodesCount == 1);

	QAction *editPortAction = menu.addAction(tr("Edit Port..."), parent(), SLOT(editNodePort()));
	editPortAction->setEnabled(dynamic_cast<CNodePort*>(triggerItem));

	// add default edge actions
	menu.addSeparator();

	QAction *edgeColorAction = menu.addAction(tr("Edge(s) Color..."), sceneActions, SLOT(onActionEdgeColor()));
	edgeColorAction->setEnabled(edgesSelected);

	QMenu *arrowsMenu = menu.addMenu(tr("Direction"));
	arrowsMenu->setEnabled(edgesSelected);
	arrowsMenu->addAction(tr("Directed"), sceneActions, SLOT(onActionEdgeDirected()));
	arrowsMenu->addAction(tr("Mutual"), sceneActions, SLOT(onActionEdgeMutual()));
	arrowsMenu->addAction(tr("None"), sceneActions, SLOT(onActionEdgeUndirected()));
	arrowsMenu->addSeparator();
	arrowsMenu->addAction(tr("Reverse"), sceneActions, SLOT(onActionEdgeReverse()));
}

