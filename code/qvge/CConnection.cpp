/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CConnection.h"
#include "CNode.h"

#include <QPen>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QDebug>

#define _USE_MATH_DEFINES
#include <math.h>


const int ARROW_SIZE = 6;


CConnection::CConnection(QGraphicsItem *parent): Shape(parent)
{
    m_firstNode = m_lastNode = NULL;

    setZValue(-1);

	//setBoundingRegionGranularity(1);

	// non-movable but selectable
	auto flags = ItemIsSelectable | ItemSendsGeometryChanges | ItemIsMovable | ItemUsesExtendedStyleOption;
	setFlags(flags);
	
	// no selection frame
	setItemFlag(IF_FramelessSelection);

	// accept hovers
	setAcceptHoverEvents(true);

	// cache
	//setCacheMode(DeviceCoordinateCache);

	// label
	m_labelItem = new QGraphicsSimpleTextItem(this);
	m_labelItem->setFlags(0);
	m_labelItem->setCacheMode(DeviceCoordinateCache);
	m_labelItem->setPen(Qt::NoPen);
}


CConnection::~CConnection()
{
	if (m_firstNode)
		m_firstNode->onConnectionDeleted(this);

	if (m_lastNode && m_lastNode != m_firstNode)
		m_lastNode->onConnectionDeleted(this);
}


// attributes

bool CConnection::hasLocalAttribute(const QByteArray& attrId) const
{
	if (attrId == "direction")
		return true;
	else
		return Super::hasLocalAttribute(attrId);
}


bool CConnection::setAttribute(const QByteArray& attrId, const QVariant& v)
{
	if (attrId == "direction")
	{
		updateArrowFlags(v.toString());
	}

	bool res = Super::setAttribute(attrId, v);

	if (res) update();
	return res;
}


bool CConnection::removeAttribute(const QByteArray& attrId)
{
	bool res = Super::removeAttribute(attrId);

	if (attrId == "direction")
	{
		updateArrowFlags(getAttribute("direction").toString());
	}

	if (res) update();
	return res;
}


// cached attributes

void CConnection::updateCachedItems()
{
	Super::updateCachedItems();

	updateArrowFlags(getAttribute("direction").toString());
}


void CConnection::updateArrowFlags(const QString& direction)
{
	if (direction == "directed")
	{
		setItemFlag(CF_End_Arrow);
		resetItemFlag(CF_Start_Arrow);
	}
	else if (direction == "mutual")
	{
		setItemFlag(CF_Mutual_Arrows);
	}
	else if (direction == "undirected")
	{
		resetItemFlag(CF_Mutual_Arrows);
	}
}


// reimp

QPainterPath CConnection::shape() const
{
	return m_selectionShapePath;
}


QRectF CConnection::boundingRect() const
{
    return Shape::boundingRect().adjusted(-10,-10,10,10);
}


void CConnection::setupPainter(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget* /*widget*/)
{
	// weight
	bool ok = false;
	double weight = qMax(0.1, getAttribute("weight").toDouble(&ok));
	if (!ok) weight = 1;
	if (weight > 10) weight = 10;	// safety

	// line style
	Qt::PenStyle penStyle = (Qt::PenStyle) CUtils::textToPenStyle(getAttribute("style").toString(), Qt::SolidLine);

	// color & selection
	bool isSelected = (option->state & QStyle::State_Selected);
    if (isSelected)
    {
		QPen p(QColor("orange"), weight + 1.0, penStyle, Qt::FlatCap, Qt::MiterJoin);
		//p.setCosmetic(true);

        painter->setPen(p);
    }
    else
	{
		// get color (to optimize!)
		QColor color = getAttribute("color").value<QColor>();

		QPen p(color, weight, penStyle, Qt::FlatCap, Qt::MiterJoin);
		//p.setCosmetic(true);

		painter->setPen(p);
	}
}


QLineF CConnection::calculateArrowLine(const QPainterPath &path, bool first, const QLineF &direction) const
{
	qreal len = path.length();

	if (first && m_firstNode)
	{
		qreal shift = m_firstNode->getDistanceToLineEnd(direction);
		qreal arrowStart = path.percentAtLength(shift + ARROW_SIZE);
		return QLineF(path.pointAtPercent(arrowStart), direction.p2());
	}
	else if (!first && m_lastNode)
	{
		qreal shift = m_lastNode->getDistanceToLineEnd(direction);
		qreal arrowStart = path.percentAtLength(len - shift - ARROW_SIZE);
		return QLineF(path.pointAtPercent(arrowStart), direction.p2());
	}

	return direction;
}


void CConnection::drawArrow(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, bool first, const QLineF& direction) const
{
	if (first && m_firstNode)
	{
		qreal shift = m_firstNode->getDistanceToLineEnd(direction);
		drawArrow(painter, shift, direction);
	}
	else if (!first && m_lastNode)
	{
		qreal shift = m_lastNode->getDistanceToLineEnd(direction);
		drawArrow(painter, shift, direction);
	}
}


void CConnection::drawArrow(QPainter* painter, qreal shift, const QLineF& direction) const
{
	static QPolygonF arrowHead;
	if (arrowHead.isEmpty())
		arrowHead << QPointF(0, 0) << QPointF(-ARROW_SIZE/2, ARROW_SIZE) << QPointF(ARROW_SIZE/2, ARROW_SIZE) << QPointF(0, 0);

	QPen oldPen = painter->pen();
	painter->save();

	painter->setPen(QPen(oldPen.color(), oldPen.widthF(), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
	painter->setBrush(oldPen.color());

	static QLineF hl(0, 0, 0, 100);
	qreal a = direction.angleTo(hl);

	painter->translate(direction.p2());
	painter->rotate(180 + a);
	painter->translate(QPointF(0, shift + oldPen.widthF()));
	painter->drawPolygon(arrowHead);

	painter->restore();
}


// IO 

bool CConnection::storeTo(QDataStream &out, quint64 version64) const
{
	Super::storeTo(out, version64);

    out << quint64(m_firstNode) << quint64(m_lastNode);

	return true;
}


bool CConnection::restoreFrom(QDataStream &out, quint64 version64)
{
	if (Super::restoreFrom(out, version64))
	{
		// these are TEMP ids
        out >> m_tempFirstNodeId >> m_tempLastNodeId;

		return true;
	}

	return false;
}


bool CConnection::linkAfterRestore(const CItemLinkMap &idToItem)
{
    CNode *node1 = dynamic_cast<CNode*>(idToItem.value(m_tempFirstNodeId));
    CNode *node2 = dynamic_cast<CNode*>(idToItem.value(m_tempLastNodeId));

	m_firstNode = m_lastNode = NULL;

	setFirstNode(node1);
	setLastNode(node2);

	return true;
}


bool CConnection::linkAfterPaste(const CItemLinkMap& idToItem)
{
	bool res = linkAfterRestore(idToItem);

	return res && isValid();
}


// impl

void CConnection::setFirstNode(CNode *node)
{
    if (m_firstNode)
        m_firstNode->onConnectionDetach(this);

    m_firstNode = node;

    if (m_firstNode)
        m_firstNode->onConnectionAttach(this);

	onParentGeometryChanged();
}


void CConnection::setLastNode(CNode *node)
{
    if (m_lastNode)
        m_lastNode->onConnectionDetach(this);

    m_lastNode = node;

    if (m_lastNode)
        m_lastNode->onConnectionAttach(this);

	onParentGeometryChanged();
}


void CConnection::reattach(CNode *oldNode, CNode *newNode)
{
	if (oldNode == newNode && !newNode->allowCircledConnection())
		return;

	if (m_firstNode == oldNode)
		setFirstNode(newNode);

	if (m_lastNode == oldNode)
		setLastNode(newNode);
}


void CConnection::reverse()
{
	qSwap(m_firstNode, m_lastNode);

	onParentGeometryChanged();
}


// reimp

QString CConnection::createNewId() const
{
	static int count = 0;

	return QString("C%1").arg(++count);
}


// callbacks

void CConnection::onNodeMoved(CNode *node)
{
	Q_ASSERT(node == m_firstNode || node == m_lastNode);
	Q_ASSERT(node != NULL);

	onParentGeometryChanged();
}


void CConnection::onNodeDetached(CNode *node)
{
	if (node == m_firstNode)
	{
		m_firstNode = NULL;
	}

	if (node == m_lastNode)
	{
		m_lastNode = NULL;
	}
}


void CConnection::onNodeDeleted(CNode *node)
{
	onNodeDetached(node);

	delete this;	// die as well
}


// reimp

void CConnection::onItemRestored()
{
	updateCachedItems();

	onParentGeometryChanged();
}


QVariant CConnection::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemSceneHasChanged)
	{
		// set default ID
		setDefaultId();

		onItemRestored();

		return value;
	}

	if (change == ItemPositionChange)
	{
		// discard any movement
		return QVariant();
	}

	if (change == ItemPositionHasChanged)
	{
		// discard any movement
		return QVariant();
	}

	if (change == ItemSelectedHasChanged)
	{
		onItemSelected(value.toBool());

		return value;
	}

	return value;
}


void CConnection::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
	onHoverEnter(this, event);
}

