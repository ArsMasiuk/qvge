#pragma once

#include <QSet>
#include <QPointF>
#include <QGraphicsItem>


enum ItemDragTestResult
{
	Rejected,
	Accepted,
	Ignored
};


class IInteractive
{
public:
	// callbacks
	virtual void onItemMoved(const QPointF& /*delta*/) {}
	virtual void onDraggedOver(const QSet<IInteractive*>& /*acceptedItems*/, const QSet<IInteractive*>& /*rejectedItems*/) {}
	virtual void onDroppedOn(const QSet<IInteractive*>& /*acceptedItems*/, const QSet<IInteractive*>& /*rejectedItems*/) {}
	virtual void onHoverEnter(QGraphicsItem* /*sceneItem*/, QGraphicsSceneHoverEvent* /*event*/) {}
	virtual void onHoverLeave(QGraphicsItem* /*sceneItem*/, QGraphicsSceneHoverEvent* /*event*/) {}
	virtual void onClick(QGraphicsSceneMouseEvent* /*mouseEvent*/) {}
	virtual void onDoubleClick(QGraphicsSceneMouseEvent* /*mouseEvent*/) {}
	virtual bool onClickDrag(QGraphicsSceneMouseEvent* /*mouseEvent*/, const QPointF& /*clickPos*/) { return true; }
	virtual bool onDoubleClickDrag(QGraphicsSceneMouseEvent* /*mouseEvent*/, const QPointF& /*clickPos*/) { return false; }
	virtual ItemDragTestResult acceptDragFromItem(QGraphicsItem* /*draggedItem*/) {	return Ignored;	}
	virtual void leaveDragFromItem(QGraphicsItem* /*draggedItem*/) {}
};
