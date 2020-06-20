/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QCursor> 
#include <QPainter>
#include <QRectF>
#include <QPointF>

#include "ISceneEditController.h"

class CEditorScene;
class QGraphicsItem;


class CTransformRect: public QObject, public ISceneEditController
{
	Q_OBJECT

public:
	CTransformRect();
	~CTransformRect();

	// ISceneEditController
	virtual void onActivated(CEditorScene& scene);
	virtual void onDeactivated(CEditorScene& /*scene*/) {}
	virtual void onSelectionChanged(CEditorScene& /*scene*/) {}
	virtual void onDragItem(CEditorScene& /*scene*/, QGraphicsSceneMouseEvent* /*mouseEvent*/, QGraphicsItem* /*dragItem*/) {}
	virtual void draw(CEditorScene& scene, QPainter *painter, const QRectF &r);
	virtual bool onMousePressed(CEditorScene& scene, QGraphicsSceneMouseEvent *mouseEvent);
	virtual bool onMouseMove(CEditorScene& scene, QGraphicsSceneMouseEvent *mouseEvent);
	virtual bool onMouseReleased(CEditorScene& scene, QGraphicsSceneMouseEvent *mouseEvent);

protected:
	void doTransformBy(CEditorScene& scene, QRectF oldRect, QRectF newRect);

	struct ControlPoint
	{
		QPointF pos;
		QCursor cursor;
		QRectF sceneRect;
	};

	ControlPoint m_points[8];

	int m_dragPoint = -1;
	QPointF m_dragPos;
	QRectF m_dragRect;
	QPointF m_lastPos;
	QRectF m_lastRect;
};

