/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QColorDialog>

#include "CNodeSceneActions.h"
#include "CNodeEditorScene.h"
#include "CNode.h"
#include "CEdge.h"


CNodeSceneActions::CNodeSceneActions(CNodeEditorScene *scene) : 
	QObject(scene),
	nodeScene(*scene)
{
}


CNodeSceneActions::~CNodeSceneActions() 
{	
}


void CNodeSceneActions::onActionNodeColor()
{
	auto nodes = nodeScene.getSelectedNodes();
	if (nodes.isEmpty())
		return;

	QColor color = QColorDialog::getColor(nodes.first()->getAttribute("color").value<QColor>());
	if (!color.isValid())
		return;

	for (auto node : nodes)
	{
		node->setAttribute("color", color);
	}

	nodeScene.addUndoState();
}


void CNodeSceneActions::onActionLink()
{
	auto nodes = nodeScene.getSelectedNodes();
	if (nodes.count() < 2)
		return;

	auto baseNode = nodes.takeFirst();
	for (auto node : nodes)
	{
		baseNode->merge(node);
	}

	nodeScene.addUndoState();
}


void CNodeSceneActions::onActionUnlink()
{
	auto nodes = nodeScene.getSelectedNodes();
	if (nodes.isEmpty())
		return;

	for (auto node : nodes)
	{
		node->unlink();
	}

	nodeScene.addUndoState();
}


void CNodeSceneActions::onActionEdgeColor()
{
	auto edges = nodeScene.getSelectedEdges();
	if (edges.isEmpty())
		return;

	QColor color = QColorDialog::getColor(edges.first()->getAttribute("color").value<QColor>());
	if (!color.isValid())
		return;

	for (auto edge : edges)
	{
		edge->setAttribute("color", color);
	}

	nodeScene.addUndoState();
}


void CNodeSceneActions::onActionEdgeReverse()
{
	auto edges = nodeScene.getSelectedEdges();
	if (edges.isEmpty())
		return;

	for (auto edge : edges)
	{
		edge->reverse();
	}

	nodeScene.addUndoState();
}


void CNodeSceneActions::onActionEdgeDirected()
{
	auto edges = nodeScene.getSelectedEdges();
	if (edges.isEmpty())
		return;

	for (auto edge : edges)
	{
		edge->setAttribute("direction", "directed");
		edge->update();
	}

	nodeScene.addUndoState();
}


void CNodeSceneActions::onActionEdgeMutual()
{
	auto edges = nodeScene.getSelectedEdges();
	if (edges.isEmpty())
		return;

	for (auto edge : edges)
	{
		edge->setAttribute("direction", "mutual");
		edge->update();
	}

	nodeScene.addUndoState();
}


void CNodeSceneActions::onActionEdgeUndirected()
{
	auto edges = nodeScene.getSelectedEdges();
	if (edges.isEmpty())
		return;

	for (auto edge : edges)
	{
		edge->setAttribute("direction", "undirected");
		edge->update();
	}

	nodeScene.addUndoState();
}

