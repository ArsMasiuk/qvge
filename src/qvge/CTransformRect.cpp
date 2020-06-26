/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/


#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>

#include "CTransformRect.h"
#include "CItem.h"
#include "CEditorScene.h"
#include "CNode.h"


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


void CTransformRect::onActivated(CEditorScene& scene)
{
	m_dragPoint = -1;

	scene.update();
}


void CTransformRect::draw(class CEditorScene &scene, QPainter *painter, const QRectF &)
{
	auto selItems = scene.getTransformableItems();
	if (selItems.size())
	{
		QRectF r = CUtils::getBoundingRect(selItems);
		m_lastRect = r;

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

			scene.invalidate(/*r.adjusted(-5, -5, 5, 5)*/);
		}

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

			doTransformBy(scene, m_lastRect, newRect);
			m_lastRect = newRect;
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

	double xc = oldRect.width() / newRect.width();
	double yc = oldRect.height() / newRect.height();

	auto selItems = scene.getTransformableItems();
	for (auto item : selItems)
	{
		auto citem = dynamic_cast<CNode*>(item);
		if (citem)
		{
			double w = citem->getSize().width();
			double h = citem->getSize().height();
			double wc = w / xc;
			double hc = h / yc;
			citem->setSize(wc, hc);

			double x = (citem->x() - oldRect.left() - w/2) / xc + newRect.left() + wc/2;
			double y = (citem->y() - oldRect.top() - h/2) / yc + newRect.top() + hc/2;
			citem->setPos(x, y);
		}
	}
}
