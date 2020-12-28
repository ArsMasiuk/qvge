/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include "CEditorSceneActions.h"

class CNodeEditorScene;
class CNode;
class CEdge;


class CNodeSceneActions : public CEditorSceneActions
{
	Q_OBJECT

public:
	CNodeSceneActions(CNodeEditorScene *scene);

public Q_SLOTS:
	bool editNodeId(CNode* node);
	bool editEdgeId(CEdge* edge);

	void onActionNodeColor();
	void onActionLink();
	void onActionUnlink();

	void onActionEdgeColor();
	void onActionEdgeReverse();
	void onActionEdgeDirected();
	void onActionEdgeMutual();
	void onActionEdgeUndirected();

private:
	CNodeEditorScene &nodeScene;
};

