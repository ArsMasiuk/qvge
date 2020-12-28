/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QColorDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "CNodeSceneActions.h"
#include "CNodeEditorScene.h"
#include "CNode.h"
#include "CEdge.h"


CNodeSceneActions::CNodeSceneActions(CNodeEditorScene *scene) : 
	CEditorSceneActions(scene),
	nodeScene(*scene)
{
}


bool CNodeSceneActions::editNodeId(CNode* editNode)
{
	if (editNode == nullptr)
		return false;

	QString id = editNode->getId();
	QString editId = id;

_again:

	QString newId = QInputDialog::getText(0, 
		tr("Change node Id"),
		tr("Specify new node Id:"), 
		QLineEdit::Normal, 
		editId);

	if (newId.isEmpty() || newId == id)
		return false;

	auto items = nodeScene.getItemsById(newId);
	for (auto item : items)
	{
		CNode* node = dynamic_cast<CNode*>(item);
		if (node == NULL || node == editNode)
			continue;

		if (node->getId() == newId)
		{
			int count = 0;
			QString nextFreeId = newId + QString::number(count++);
			while (nodeScene.getItemsById(nextFreeId).count())
			{
				nextFreeId = newId + QString::number(count++);
			}

			QString autoId = QString(tr("Suggested Id: %1").arg(nextFreeId));

			int r = QMessageBox::warning(0, 
				tr("Warning: Id is in use"),
				tr("Id %1 is already used by another node.").arg(newId),
				autoId,
				tr("Swap node Ids"),
				tr("Continue editing"), 0, 2);

			if (r == 2)
			{
				editId = newId;
				goto _again;
			}

			if (r == 1)
			{
				editNode->setId(newId);
				node->setId(id);
				nodeScene.addUndoState();
				return true;
			}

			// r = 0
			editId = nextFreeId;
			goto _again;
		}
	}

	editNode->setId(newId);
	nodeScene.addUndoState();
	return true;
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


bool CNodeSceneActions::editEdgeId(CEdge* editEdge)
{
	if (editEdge == nullptr)
		return false;

	QString id = editEdge->getId();
	QString editId = id;

_again:

	QString newId = QInputDialog::getText(0, 
		tr("Change edge Id"),
		tr("Specify new edge Id:"), 
		QLineEdit::Normal, 
		editId);

	if (newId.isEmpty() || newId == id)
		return false;

	auto items = nodeScene.getItemsById(newId);
	for (auto item : items)
	{
		CEdge* edge = dynamic_cast<CEdge*>(item);
		if (edge == NULL || edge == editEdge)
			continue;

		if (edge->getId() == newId)
		{
			int count = 0;
			QString nextFreeId = newId + QString::number(count++);
			while (nodeScene.getItemsById(nextFreeId).count())
			{
				nextFreeId = newId + QString::number(count++);
			}

			QString autoId = QString(tr("Suggested Id: %1").arg(nextFreeId));

			int r = QMessageBox::warning(0, 
				tr("Warning: Id is in use"),
				tr("Id %1 is already used by another edge.").arg(newId),
				autoId,
				tr("Swap edge Ids"),
				tr("Continue editing"), 0, 2);

			if (r == 2)
			{
				editId = newId;
				goto _again;
			}

			if (r == 1)
			{
				editEdge->setId(newId);
				edge->setId(id);
				nodeScene.addUndoState();
				return true;
			}

			// r = 0
			editId = nextFreeId;
			goto _again;
		}
	}

	editEdge->setId(newId);
	nodeScene.addUndoState();
	return true;
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

