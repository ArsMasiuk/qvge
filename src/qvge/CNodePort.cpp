/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CNodePort.h"
#include "CNode.h"


CNodePort::CNodePort(CNode *node, const QByteArray& portId, int align, double xoff, double yoff) :
	Shape(dynamic_cast<QGraphicsItem*>(node)),
	m_node(node),
	m_id(portId), m_align(align), m_xoff(xoff), m_yoff(yoff)
{
 	Q_ASSERT(m_node != NULL);

	setRect(-3, -3, 7, 7);
	setBrush(Qt::gray);
	setPen(QPen(Qt::black, 1));

	setToolTip(portId);

	setFlags(QGraphicsItem::ItemClipsToShape | QGraphicsItem::ItemIgnoresParentOpacity);
}


CNodePort::~CNodePort()
{
	if (m_node)
		m_node->onPortDeleted(this);
}


void CNodePort::onParentDeleted()
{
	// clear m_node if it is removed already
	m_node = NULL;
}


void CNodePort::onParentGeometryChanged()
{
	Q_ASSERT(m_node != NULL);

	QRectF nodeBox = m_node->Shape::boundingRect();

	int x = m_xoff, y = m_yoff;
	
	if (m_align & Qt::AlignLeft)
		x -= nodeBox.width() / 2;
	else if (m_align & Qt::AlignRight)
		x += nodeBox.width() / 2;

	if (m_align & Qt::AlignTop)
		y -= nodeBox.height() / 2;
	else if (m_align & Qt::AlignBottom)
		y += nodeBox.height() / 2;

	setX(x); setY(y);
}


void CNodePort::setAlign(int newAlign)
{
	m_align = newAlign;
}


void CNodePort::setOffset(double xoff, double yoff)
{
	m_xoff = xoff;
	m_yoff = yoff;
}


void CNodePort::copyDataFrom(const CNodePort &port)
{
	m_id = port.m_id;
	m_align = port.m_align;
	m_xoff = port.m_xoff;
	m_yoff = port.m_yoff;
}


ItemDragTestResult CNodePort::acceptDragFromItem(QGraphicsItem* draggedItem)
{
	if (dynamic_cast<CNode*>(draggedItem))
	{
		setOpacity(0.5);
		return Accepted;
	}
	else
	{
		setOpacity(1);
		return Ignored;
	}
}


void CNodePort::leaveDragFromItem(QGraphicsItem* draggedItem)
{
	setOpacity(1);
}


void CNodePort::onClick(QGraphicsSceneMouseEvent* /*mouseEvent*/)
{
	setSelected(true);
}
//
//
//void CNodePort::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
//{
//	
//}
//
//
//void CNodePort::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
//{
//
//}


// serialization 

bool CNodePort::storeTo(QDataStream& out, quint64 version64) const
{
	out << m_id;
	out << m_align << m_xoff << m_yoff;
	return true;
}


//bool CNodePort::restoreFrom(QDataStream& out, quint64 version64)
//{
//
//}
