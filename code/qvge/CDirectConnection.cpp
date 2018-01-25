/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include <QDebug>

#include "CDirectConnection.h"
#include "CNode.h"


CDirectConnection::CDirectConnection(QGraphicsItem *parent): Super(parent)
{
	m_bendFactor = 0;
}


void CDirectConnection::setBendFactor(int bf)
{
	if (bf != m_bendFactor)
	{
		m_bendFactor = bf;

		onParentGeometryChanged();
	}
}


// reimp

CConnection* CDirectConnection::clone()
{
	CDirectConnection* c = new CDirectConnection();
	c->setFirstNode(m_firstNode);
	c->setLastNode(m_lastNode);
	return c;
}


void CDirectConnection::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget* widget)
{
	//qDebug() << boundingRect() << option->exposedRect << option->rect;

	// called before draw 
    setupPainter(painter, option, widget);

    painter->setClipRect(boundingRect());

	// circled connection
	if (isCircled())
	{
		int nodeDiameter = m_firstNode->boundingRect().height();
		double nr = nodeDiameter / 2;
		double r = nr + qAbs(m_bendFactor) * nr / 2;

		painter->drawEllipse(m_controlPos, r, r);
	}
	else
		if (m_bendFactor == 0)	// straight line
		{
			painter->drawLine(line());

            // arrows
            if (m_itemFlags & CF_Start_Arrow)
                drawArrow(painter, option, true, QLineF(line().p2(), line().p1()));

            if (m_itemFlags & CF_End_Arrow)
                drawArrow(painter, option, false, line());
		}
		else // curve
		{
			QPainterPath pp;
			pp.moveTo(line().p1());
			pp.cubicTo(m_controlPoint, m_controlPoint, line().p2());

			painter->setBrush(Qt::NoBrush);
			painter->drawPath(pp);

			// arrows
            if (m_itemFlags & CF_Start_Arrow)
            {
                QLineF arrowLine = calculateArrowLine(pp, true, QLineF(m_controlPos, line().p1()));
                drawArrow(painter, option, true, arrowLine);
            }

            if (m_itemFlags & CF_End_Arrow)
            {
                QLineF arrowLine = calculateArrowLine(pp, false, QLineF(m_controlPos, line().p2()));
                drawArrow(painter, option, false, arrowLine);
            }
        }
}


void CDirectConnection::updateLabelPosition()
{
	auto r = m_labelItem->boundingRect();
	int w = r.width();
	int h = r.height();
	m_labelItem->setTransformOriginPoint(w / 2, h / 2);

	if (isCircled())
	{
		m_labelItem->setPos(m_controlPos.x() - w / 2, m_controlPos.y() - boundingRect().height() / 2 - h);

		m_labelItem->setRotation(0);
	}
	else
	{
		m_labelItem->setPos(m_controlPos.x() - w / 2, m_controlPos.y() - h / 2);

		// update label rotation
		qreal angle = 180 - line().angle();
		if (angle > 90) angle -= 180;
		else if (angle < -90) angle += 180;
		//qDebug() << angle;
		//m_labelItem->setRotation(angle);
	}
}


// callbacks 

void CDirectConnection::onParentGeometryChanged()
{
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

	// circled connection 
	if (isCircled())
	{
		int nodeDiameter = m_firstNode->boundingRect().height();
		double nr = nodeDiameter / 2;
		double r = nr + qAbs(m_bendFactor) * nr / 2;

		m_controlPos = p1 + QPointF(0, -r);
		path.addEllipse(m_controlPos, r, r);
	}
	else // not circled
	{
		path.moveTo(p1);

		// center
		m_controlPos = (p1 + p2) / 2;

		if (m_bendFactor == 0)
		{
			path.lineTo(p2);
		}
		else
		{
			QPointF t1 = m_controlPos;
			float posFactor = qAbs(m_bendFactor);

			bool bendDirection = (quint64(m_firstNode) > quint64(m_lastNode));
			if (m_bendFactor < 0)
				bendDirection = !bendDirection;

			QLineF f1(t1, p2);
			f1.setAngle(bendDirection ? f1.angle() + 90 : f1.angle() - 90);
			f1.setLength(f1.length() * 0.2 * posFactor);

			m_controlPos = f1.p2();
			m_controlPoint = m_controlPos - (t1 - m_controlPos) * 0.33;

			path.cubicTo(m_controlPoint, m_controlPoint, p2);
		}
	}

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

