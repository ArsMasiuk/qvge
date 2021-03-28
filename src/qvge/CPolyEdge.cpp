/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QGraphicsSceneMouseEvent>

#include "CPolyEdge.h"
#include "CNode.h"
#include "CControlPoint.h"


CPolyEdge::CPolyEdge(QGraphicsItem *parent): Super(parent)
{
}


void CPolyEdge::setPoints(const QList<QPointF> &points)
{
	m_polyPoints = points;

	onParentGeometryChanged();
}


bool CPolyEdge::insertPointAt(const QPointF &pos)
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

CEdge* CPolyEdge::clone()
{
	CPolyEdge* c = new CPolyEdge(parentItem());

	// assign directly!
	c->m_firstNode = m_firstNode;
	c->m_firstPortId = m_firstPortId;
	c->m_lastNode = m_lastNode;
	c->m_lastPortId = m_lastPortId;
	c->m_polyPoints = m_polyPoints;

	if (scene())
		scene()->addItem(c);

	c->copyDataFrom(this);

	return c;
}


void CPolyEdge::reverse()
{
	std::reverse(m_polyPoints.begin(), m_polyPoints.end());
	std::reverse(m_controlPoints.begin(), m_controlPoints.end());

	Super::reverse();
}


void CPolyEdge::transform(const QRectF & oldRect, const QRectF & newRect, 
	double xc, double yc, 
	bool changeSize, bool changePos)
{
	Super::transform(oldRect, newRect, xc, yc, changeSize, changePos);

	// snap
	//auto scene = getScene();

	// transform subpoints as well
	for (auto &point : m_polyPoints)
	{
		double xp = (point.x() - oldRect.left()) * xc + newRect.left();
		double yp = (point.y() - oldRect.top()) * yc + newRect.top();

		//if (scene)
		//{
		//	QPointF psnap = scene->getSnapped(QPointF(xp,yp));
		//	point.setX(psnap.x());
		//	point.setY(psnap.y());
		//}
		//else
		{
			point.setX(xp);
			point.setY(yp);
		}
	}

	createControlPoints();
	updateShapeFromPoints();
}


// attributes

bool CPolyEdge::hasLocalAttribute(const QByteArray& attrId) const
{
	if (attrId == "points")
		return true;
	else
		return Super::hasLocalAttribute(attrId);
}


bool CPolyEdge::setAttribute(const QByteArray& attrId, const QVariant& v)
{
	if (attrId == "points")
	{
		QString pointStr = v.toString();
		setPoints(CUtils::pointsFromString(pointStr));
		return true;
	}

	return Super::setAttribute(attrId, v);
}


bool CPolyEdge::removeAttribute(const QByteArray& attrId)
{
	if (attrId == "points")
	{
		setPoints({});
		return true;
	}

	return Super::removeAttribute(attrId);
}


// serialization 

bool CPolyEdge::storeTo(QDataStream& out, quint64 version64) const
{
	Super::storeTo(out, version64);

	out << m_polyPoints;

	return true;
}


bool CPolyEdge::restoreFrom(QDataStream& out, quint64 version64)
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

bool CPolyEdge::onDoubleClickDrag(QGraphicsSceneMouseEvent* /*mouseEvent*/, const QPointF &clickPos)
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


void CPolyEdge::onControlPointMoved(CControlPoint* /*controlPoint*/, const QPointF& /*pos*/)
{
	updateShapeFromPoints();
}


void CPolyEdge::onControlPointDelete(CControlPoint* controlPoint)
{
	int index = m_controlPoints.indexOf(controlPoint);
	Q_ASSERT(index >= 0);

	m_controlPoints.removeAt(index);
	delete controlPoint;
	
	updateShapeFromPoints();

	addUndoState();
}


// selection

void CPolyEdge::onItemSelected(bool state)
{
	Super::onItemSelected(state);

	if (!state)
		dropControlPoints();
	else
		createControlPoints();
}


// moving

void CPolyEdge::onItemMoved(const QPointF& delta)
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

void CPolyEdge::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	// straight line
	if (m_polyPoints.isEmpty())
	{
		Super::paint(painter, option, widget);
		return;
	}

	// selection
	drawSelection(painter, option);

	// polyline
	setupPainter(painter, option, widget);

	painter->setClipRect(boundingRect());

	painter->save();

	painter->setBrush(Qt::NoBrush);
	painter->drawPath(m_shapeCachePath);

	painter->restore();

	qreal r = qMax(3.0, painter->pen().widthF());
	painter->setBrush(painter->pen().brush());

	for (const QPointF &p : m_polyPoints)
		painter->drawEllipse(p, r, r);

	// arrows
	if (m_itemFlags & CF_Start_Arrow)
	{
		QLineF arrowLine(m_polyPoints.first(), line().p1());
		if (arrowLine.length() > ARROW_SIZE * 2)
			drawArrow(painter, option, true, arrowLine);
	}

	if (m_itemFlags & CF_End_Arrow)
	{
		QLineF arrowLine(m_polyPoints.last(), line().p2());
		if (arrowLine.length() > ARROW_SIZE * 2)
			drawArrow(painter, option, true, arrowLine);
	}
}


// callbacks 

void CPolyEdge::onParentGeometryChanged()
{
	// straight line
	if (m_polyPoints.isEmpty())
	{
		Super::onParentGeometryChanged();
		return;
	}

	// optimize: no update while restoring
	if (s_duringRestore)
		return;

	// polyline
	if (!m_firstNode || !m_lastNode)
		return;

	prepareGeometryChange();

	// update line position
	QPointF p1c = m_firstNode->pos();
	if (m_firstPortId.size() && m_firstNode->getPort(m_firstPortId))
		p1c = m_firstNode->getPort(m_firstPortId)->scenePos();

	QPointF p2c = m_lastNode->pos();
	if (m_lastPortId.size() && m_lastNode->getPort(m_lastPortId))
		p2c = m_lastNode->getPort(m_lastPortId)->scenePos();

	QLineF l1(p1c, m_polyPoints.first());
	QPointF p1 = m_firstNode->getIntersectionPoint(l1, m_firstPortId);
	QLineF l2(p2c, m_polyPoints.last());
	QPointF p2 = m_lastNode->getIntersectionPoint(l2, m_lastPortId);

	QLineF l(p1, p2);
	setLine(l);

	// shift line by arrows
	double arrowSize = getVisibleWeight() + ARROW_SIZE;

	l1 = QLineF(p1, m_polyPoints.first());
	if (l1.length() < 5)
		p1 = m_polyPoints.first();
	else if (m_itemFlags & CF_Start_Arrow)
	{
		l1 = CUtils::extendLine(l1, -arrowSize, 0);
		p1 = l1.p1();
	}

	l2 = QLineF(m_polyPoints.last(), p2);
	if (l2.length() < 5)
		p2 = m_polyPoints.last();
	else if (m_itemFlags & CF_End_Arrow)
	{
		l2 = CUtils::extendLine(l2, 0, arrowSize);
		p2 = l2.p2();
	}


	// update shape path
	m_shapeCachePath = QPainterPath();
	m_shapeCachePath.moveTo(p1);
	
	for (const QPointF &p : m_polyPoints)
		m_shapeCachePath.lineTo(p);

	m_shapeCachePath.lineTo(p2);

	m_controlPoint = m_shapeCachePath.pointAtPercent(0.5);

	QPainterPathStroker stroker;
	stroker.setWidth(6);
	m_selectionShapePath = stroker.createStroke(m_shapeCachePath);

	update();

	// update text label
	if (getScene() && getScene()->itemLabelsEnabled())
	{
		updateLabelPosition();
		updateLabelDecoration();
	}
}


// private

void CPolyEdge::dropControlPoints()
{
	qDeleteAll(m_controlPoints);
	m_controlPoints.clear();
}


void CPolyEdge::createControlPoints()
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


void CPolyEdge::updateShapeFromPoints()
{
	m_polyPoints.clear();

	for (auto cp : m_controlPoints)
	{
		m_polyPoints.append(cp->scenePos());
	}

	onParentGeometryChanged();
}
