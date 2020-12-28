#pragma once

class CEditorScene;

class QGraphicsItem;
class QGraphicsSceneContextMenuEvent;


class ISceneMenuController
{
public:
	virtual bool exec(CEditorScene *scene, QGraphicsItem *triggerItem, QGraphicsSceneContextMenuEvent *contextMenuEvent) = 0;
};

