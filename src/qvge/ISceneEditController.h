/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

class CEditorScene;

class QGraphicsSceneMouseEvent;
class QPainter;
class QRectF;
class QGraphicsItem;


class ISceneEditController
{
public:
	virtual void onActivated(CEditorScene& scene) = 0;
	virtual void onDeactivated(CEditorScene& scene) = 0;

	virtual void onSelectionChanged(CEditorScene& scene) = 0;
	virtual void onDragItem(CEditorScene& scene, QGraphicsSceneMouseEvent *mouseEvent, QGraphicsItem* dragItem) = 0;

	virtual bool onMousePressed(CEditorScene& scene, QGraphicsSceneMouseEvent *mouseEvent) = 0;
	virtual bool onMouseMove(CEditorScene& scene, QGraphicsSceneMouseEvent *mouseEvent) = 0;
	virtual bool onMouseReleased(CEditorScene& scene, QGraphicsSceneMouseEvent *mouseEvent) = 0;

	virtual void draw(CEditorScene& scene, QPainter *painter, const QRectF &r) = 0;
};
