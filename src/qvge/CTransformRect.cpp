/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/


#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>

#include "CTransformRect.h"
#include "CEditorScene.h"
#include "CItem.h"
#include "CNode.h"
#include "CEdge.h"


const double MIN_RECT_SIZE = 15.0;


CTransformRect::CTransformRect()
{
	m_points[0].cursor = Qt::SizeFDiagCursor;
	m_points[1].cursor = Qt::SizeVerCursor;
	m_points[2].cursor = Qt::SizeBDiagCursor;
	m_points[3].cursor = Qt::SizeHorCursor;
	m_points[4].cursor = Qt::SizeHorCursor;
	m_points[5].cursor = Qt::SizeBDiagCursor;
	m_points[6].cursor = Qt::SizeVerCursor;
	m_points[7].cursor = Qt::SizeFDiagCursor;
}


CTransformRect::~CTransformRect()
{
}


void CTransformRect::setMoveOnly(bool on)
{
	m_moveOnlyMode = on;
}


void CTransformRect::onActivated(CEditorScene& scene)
{
	m_dragPoint = -1;

	onSelectionChanged(scene);
}


void CTransformRect::onSelectionChanged(CEditorScene& scene)
{
	auto selItems = scene.getTransformableItems();
	if (selItems.size())
	{
		QRectF r = CUtils::getBoundingRect(selItems);
		m_lastRect = r;
	}
	else
	{
		m_lastRect = QRectF();
	}

	scene.update();
}


void CTransformRect::onSceneChanged(CEditorScene& scene)
{
	onSelectionChanged(scene);
}


void CTransformRect::onDragItem(CEditorScene& scene, QGraphicsSceneMouseEvent* /*mouseEvent*/, QGraphicsItem* /*dragItem*/)
{
	onSelectionChanged(scene);
}


void CTransformRect::draw(class CEditorScene &scene, QPainter *painter, const QRectF &)
{
	QRectF r = m_lastRect;
	if (r.isEmpty() || r.isNull() || !r.isValid())
		return;

	// update points
	m_points[0].pos = r.topLeft();
	m_points[1].pos = QPointF(r.center().x(), r.top());
	m_points[2].pos = r.topRight();
	m_points[3].pos = QPointF(r.left(), r.center().y());
	m_points[4].pos = QPointF(r.right(), r.center().y());
	m_points[5].pos = r.bottomLeft();
	m_points[6].pos = QPointF(r.center().x(), r.bottom());
	m_points[7].pos = r.bottomRight();

	const QPen rectPen(QColor(0x333333), 0, Qt::SolidLine);
	painter->setPen(rectPen);
	painter->drawRect(r);

	auto view = scene.getCurrentView();
	if (view)
	{
		for (int i = 0; i < 8; ++i)
		{
			// zoom-independend control points
			QPoint viewPos = view->mapFromScene(m_points[i].pos);
			QPointF topLeft = view->mapToScene(QPoint(viewPos.x() - 4, viewPos.y() - 4));
			QPointF bottomRight = view->mapToScene(QPoint(viewPos.x() + 4, viewPos.y() + 4));

			m_points[i].sceneRect = QRectF(topLeft, bottomRight);

			painter->fillRect(m_points[i].sceneRect, Qt::SolidPattern);
		}

		scene.invalidate();
	}
}


bool CTransformRect::onMousePressed(CEditorScene& /*scene*/, QGraphicsSceneMouseEvent* mouseEvent)
{
	bool isDragging = (mouseEvent->button() == Qt::LeftButton);
	if (!isDragging)
		return false;

	auto pos = mouseEvent->scenePos();

	for (int i = 0; i < 8; ++i)
	{
		if (m_points[i].sceneRect.contains(pos))
		{
			m_dragPos = m_lastPos = pos;
			m_dragPoint = i;
			return true;
		}
	}

	m_dragPos = m_lastPos = QPointF();
	m_dragPoint = -1;
	return false;
}


bool CTransformRect::onMouseReleased(CEditorScene& scene, QGraphicsSceneMouseEvent *mouseEvent)
{
	bool isDragging = (mouseEvent->button() == Qt::LeftButton);
	if (!isDragging)
		return false;

	// nothing was dragged
	if (m_dragPoint == -1)
		return false;

	// else finish the drag
	if (m_lastPos != m_dragPos)
		scene.addUndoState();

	scene.setSceneCursor(Qt::ArrowCursor);

	m_dragPos = m_lastPos = QPointF();
	m_dragPoint = -1;
	return true;
}


bool CTransformRect::onMouseMove(CEditorScene& scene, QGraphicsSceneMouseEvent *mouseEvent)
{
	auto pos = mouseEvent->scenePos();

	bool isDragging = (mouseEvent->buttons() & Qt::LeftButton);
	if (isDragging && m_dragPoint >= 0)
	{
		// drag
		QPointF deltaPos = pos - m_lastPos;
		if (!deltaPos.isNull())
		{
			QRectF newRect = m_lastRect;

			bool isShift = (mouseEvent->modifiers() & Qt::ShiftModifier);

			// default transform
			switch (m_dragPoint)
			{
			case 0:		newRect.setTopLeft(pos);		break;
			case 1:		newRect.setTop(pos.y());		break;
			case 2:		newRect.setTopRight(pos);		break;
			case 3:		newRect.setLeft(pos.x());		break;
			case 4:		newRect.setRight(pos.x());		break;
			case 5:		newRect.setBottomLeft(pos);		break;
			case 6:		newRect.setBottom(pos.y());		break;
			case 7:		newRect.setBottomRight(pos);	break;
			}

			// if shift pressed: mirror around center
			if (isShift && newRect.isValid())
			{
				qreal dx_r = newRect.right() - m_lastRect.right();
				qreal dx_l = newRect.left() - m_lastRect.left();
				qreal dy_t = newRect.top() - m_lastRect.top();
				qreal dy_b = newRect.bottom() - m_lastRect.bottom();

				switch (m_dragPoint)
				{
				case 0:		newRect.setBottomRight(m_lastRect.bottomRight() - QPointF(dx_l, dy_t));		break;
				case 1:		newRect.setBottom(m_lastRect.bottom() - dy_t);								break;
				case 2:		newRect.setBottomLeft(m_lastRect.bottomLeft() - QPointF(dx_r, dy_t));		break;
				case 3:		newRect.setRight(m_lastRect.right() - dx_l);								break;
				case 4:		newRect.setLeft(m_lastRect.left() - dx_r);									break;
				case 5:		newRect.setTopRight(m_lastRect.topRight() - QPointF(dx_l, dy_b));			break;
				case 6:		newRect.setTop(m_lastRect.top() - dy_b);									break;
				case 7:		newRect.setTopLeft(m_lastRect.topLeft() - QPointF(dx_r, dy_b));				break;
				}
			}

			// do transform if rect is valid
			if (newRect.isValid() && newRect.width() >= MIN_RECT_SIZE && newRect.height() >= MIN_RECT_SIZE)
			{
				doTransformBy(scene, m_lastRect, newRect);
				m_lastRect = newRect;
			}
			else
				return false;
		}

		m_lastPos = pos;
		return true;
	}

	// no drag - check hover
	for (int i = 0; i < 8; ++i)
	{
		if (m_points[i].sceneRect.contains(pos))
		{
			scene.setSceneCursor(m_points[i].cursor);
			return true;
		}
	}

	// no hit
	return false;
}


void CTransformRect::doTransformBy(CEditorScene& scene, QRectF oldRect, QRectF newRect)
{
	if (oldRect == newRect)
		return;

	// normalize borders
	const int margin = scene.getBoundingMargin();
	oldRect.adjust(margin, margin, -margin, -margin);
	newRect.adjust(margin, margin, -margin, -margin);

	if (!oldRect.isValid() || !newRect.isValid())
		return;

	double xc = newRect.width() / oldRect.width();
	double yc = newRect.height() / oldRect.height();

	// prepare transform lists
	QList<CNode*> nodesTransform, nodesMove;
	QList<CItem*> others;

	auto selItems = scene.getTransformableItems();

	// go for nodes
	for (auto item : selItems)
	{
		auto cnode = dynamic_cast<CNode*>(item);
		if (cnode)
		{
			nodesTransform << cnode;
			continue;
		}
	}

	// go for edges & rest
	for (auto item : selItems)
	{
		auto cnode = dynamic_cast<CNode*>(item);
		if (cnode)
			continue;

		auto cedge = dynamic_cast<CEdge*>(item);
		if (cedge)
		{
			others << cedge;
			auto n1 = cedge->firstNode();
			auto n2 = cedge->lastNode();
			if (!nodesTransform.contains(n1) && !nodesMove.contains(n1))
				nodesMove << n1;
			if (!nodesTransform.contains(n2) && !nodesMove.contains(n2))
				nodesMove << n2;
			continue;
		}

		auto citem = dynamic_cast<CItem*>(item);
		if (citem)
		{
			others << citem;
			continue;
		}
	}

	// run transformation
	bool changeSize = !m_moveOnlyMode;

	for (auto node : nodesTransform)
	{
		node->transform(oldRect, newRect, xc, yc, selItems, changeSize, true);
	}

	for (auto node : nodesMove)
	{
		node->transform(oldRect, newRect, xc, yc, selItems, false, true);
	}

	for (auto item : others)
	{
		item->transform(oldRect, newRect, xc, yc, selItems, changeSize, true);
	}

	// snap after transform
	// TO DO...
}
