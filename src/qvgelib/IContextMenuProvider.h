/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2021 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QMenu>
#include <QList>
#include <QGraphicsItem>

class CEditorScene;
class QGraphicsSceneContextMenuEvent;


class IContextMenuProvider
{
public:
	virtual bool showMenu(QGraphicsSceneContextMenuEvent* contextMenuEvent, CEditorScene* scene, const QList<QGraphicsItem*>& selectedItems) 
	{ return false; }

	// tbd: deprecate
	virtual bool populateMenu(QMenu& menu, const QList<QGraphicsItem*>& selectedItems) 
	{ return false; }
};
