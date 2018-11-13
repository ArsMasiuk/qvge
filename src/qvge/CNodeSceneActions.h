/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QObject>

class CNodeEditorScene;


class CNodeSceneActions : public QObject 
{
	Q_OBJECT

public:
	CNodeSceneActions(CNodeEditorScene *scene);
	~CNodeSceneActions();

public Q_SLOTS:
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

