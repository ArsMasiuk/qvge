/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QGraphicsSceneMouseEvent>

#include "CPolylineConnection.h"
#include "CNode.h"
#include "CControlPoint.h"


CPolylineConnection::CPolylineConnection(QGraphicsItem *parent): Super(parent)
{
}


void CPolylineConnection::setPoints(const QList<QPointF> &points)
{
	m_polyPoints = points;

	onParentGeometryChanged();
}


bool CPolylineConnection::insertPointAt(const QPointF &pos)
{
	// no points yet
	if (m_polyPoints.isEmpty())
	{
		m_polyPoints.append(pos);
		update();
		return true;
	}

	// find segment for this point
	auto points = m_polyPoints;
	points.prepend(m_firstNode->pos());
	points.append(m_lastNode->pos());

	for (int i = 0; i < points.size() - 1; ++i)
	{
		qreal l1 = QLineF(points.at(i), points.at(i + 1)).length();
		qreal l2 = QLineF(points.at(i), pos).length();
		qreal l3 = QLineF(pos, points.at(i + 1)).length();
		if (qAbs(l1 - (l2 + l3)) < 1)
		{
			m_polyPoints.insert(i, pos);
			update();
			return true;
		}
	}

	return false;
}


// reimp

CConnection* CPolylineConnection::clone()
{
	CPolylineConnection* c = new CPolylineConnection();
	c->setFirstNode(m_firstNode);
	c->setLastNode(m_lastNode);
	c->setPoints(m_polyPoints);
	return c;
}


// serialization 

bool CPolylineConnection::storeTo(QDataStream& out, quint64 version64) const
{
	Super::storeTo(out, version64);

	out << m_polyPoints;

	return true;
}


bool CPolylineConnection::restoreFrom(QDataStream& out, quint64 version64)
{
	if (Super::restoreFrom(out, version64))
	{
		dropControlPoints();

		m_polyPoints.clear();
		out >> m_polyPoints;

		return true;
	}

	return false;
}


// mousing

bool CPolylineConnection::onDoubleClickDrag(QGraphicsSceneMouseEvent* /*mouseEvent*/, const QPointF &clickPos)
{
	// create control point at click pos
	if (insertPointAt(clickPos))
	{
		createControlPoints();

		// start drag of the inserted point
		for (auto cp : m_controlPoints)
		{
			if (cp->scenePos() == clickPos)
			{
				getScene()->startDrag(cp);

				return true;
			}
		}
		 
		return false;
	}

	return false;
}


void CPolylineConnection::onControlPointMoved(CControlPoint* /*controlPoint*/, const QPointF& /*pos*/)
{
	updateShapeFromPoints();
}


void CPolylineConnection::onControlPointDelete(CControlPoint* controlPoint)
{
	int index = m_controlPoints.indexOf(controlPoint);
	Q_ASSERT(index >= 0);

	m_controlPoints.removeAt(index);
	delete controlPoint;
	
	updateShapeFromPoints();

	addUndoState();
}


// selection

void CPolylineConnection::onItemSelected(bool state)
{
	Super::onItemSelected(state);

	if (!state)
		dropControlPoints();
	else
		createControlPoints();
}


// moving

void CPolylineConnection::onItemMoved(const QPointF& delta)
{
	for (auto &p : m_polyPoints)
	{
		p += delta;
	}

	for (auto cp : m_controlPoints)
	{
		cp->moveBy(delta.x(), delta.y());
	}
}


// drawing

void CPolylineConnection::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget* widget)
{
	// straight line
	if (m_polyPoints.isEmpty())
	{
		Super::paint(painter, option, widget);
		return;
	}

	// polyline
	painter->setClipRect(option->exposedRect);

	setupPainter(painter, option, widget);

	QPointF p1 = m_firstNode->pos(), p2 = m_lastNode->pos();

	QPainterPath path;
	path.moveTo(p1);

	for (const QPointF &p : m_polyPoints)
		path.lineTo(p);

	path.lineTo(p2);

	painter->save();

	painter->setBrush(Qt::NoBrush);
	painter->drawPath(path);

	painter->restore();

	qreal r = qMax(2.0, painter->pen().widthF());
	painter->setBrush(painter->pen().brush());

	for (const QPointF &p : m_polyPoints)
		painter->drawEllipse(p, r, r);

	// arrows
	if (m_itemFlags & CF_Start_Arrow)
	{
		QLineF arrowLine(m_polyPoints.first(), p1);
		drawArrow(painter, option, true, arrowLine);
	}

	if (m_itemFlags & CF_End_Arrow)
	{
		QLineF arrowLine(m_polyPoints.last(), p2);
		drawArrow(painter, option, false, arrowLine);
	}
}


void CPolylineConnection::updateLabelPosition()
{
	// straight line
	if (m_polyPoints.isEmpty())
	{
		Super::updateLabelPosition();
		return;
	}

	// polyline
	auto r = m_labelItem->boundingRect();
	int w = r.width();
	int h = r.height();
	m_labelItem->setTransformOriginPoint(w / 2, h / 2);

	m_labelItem->setPos(m_centerPos);

	//	// update label rotation
	//	qreal angle = 180 - line().angle();
	//	if (angle > 90) angle -= 180;
	//	else if (angle < -90) angle += 180;
	//	//qDebug() << angle;
	//	//m_labelItem->setRotation(angle);
	//}
}


// callbacks 

void CPolylineConnection::onParentGeometryChanged()
{
	// straight line
	if (m_polyPoints.isEmpty())
	{
		Super::onParentGeometryChanged();
		return;
	}

	// polyline
	if (!m_firstNode || !m_lastNode)
		return;

	// optimize: no update while restoring
	if (s_duringRestore)
		return;

	prepareGeometryChange();

	// update line position
	QPointF p1 = m_firstNode->pos(), p2 = m_lastNode->pos();
	QLineF l(p1, p2);
	setLine(l);

	// update shape path
	QPainterPath path;
	path.moveTo(p1);
	
	for (const QPointF &p : m_polyPoints)
		path.lineTo(p);

	path.lineTo(p2);

	m_centerPos = path.pointAtPercent(0.5);

	QPainterPathStroker stroker;
	stroker.setWidth(6);
	m_selectionShapePath = stroker.createStroke(path);

	update();

	// update text label
	if (getScene() && getScene()->itemLabelsEnabled())
	{
		updateLabelPosition();
		updateLabelDecoration();
	}
}


// private

void CPolylineConnection::dropControlPoints()
{
	for (auto cp : m_controlPoints)
	{
		delete cp;
	}

	m_controlPoints.clear();
}


void CPolylineConnection::createControlPoints()
{
	dropControlPoints();

	for (const QPointF &point: m_polyPoints)
	{
		auto cp = new CControlPoint(this);
		
		// first set position, then flags (to avoid recursion)
		cp->setPos(point);
		cp->setFlags(ItemIsMovable | ItemSendsGeometryChanges);

		m_controlPoints.append(cp);
	}
}


void CPolylineConnection::updateShapeFromPoints()
{
	m_polyPoints.clear();

	for (auto cp : m_controlPoints)
	{
		m_polyPoints.append(cp->scenePos());
	}

	onParentGeometryChanged();
}
