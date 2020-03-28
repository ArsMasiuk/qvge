/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CEdge.h"
#include "CNode.h"

#include <QPen>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QDebug>
#include <QGuiApplication>

#define _USE_MATH_DEFINES
#include <math.h>


CEdge::CEdge(QGraphicsItem *parent): Shape(parent)
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
	m_labelItem->setAcceptedMouseButtons(Qt::NoButton);
}


CEdge::~CEdge()
{
	if (m_firstNode)
		m_firstNode->onConnectionDeleted(this);

	if (m_lastNode && m_lastNode != m_firstNode)
		m_lastNode->onConnectionDeleted(this);
}


// attributes

bool CEdge::hasLocalAttribute(const QByteArray& attrId) const
{
	if (attrId == "direction")
		return true;
	else
		return Super::hasLocalAttribute(attrId);
}


bool CEdge::setAttribute(const QByteArray& attrId, const QVariant& v)
{
	if (attrId == "direction")
	{
		updateArrowFlags(v.toString());
	}

	bool res = Super::setAttribute(attrId, v);

	if (res) update();
	return res;
}


bool CEdge::removeAttribute(const QByteArray& attrId)
{
	bool res = Super::removeAttribute(attrId);

	if (attrId == "direction")
	{
		updateArrowFlags(getAttribute(QByteArrayLiteral("direction")).toString());
	}

	if (res) update();
	return res;
}


// cached attributes

void CEdge::updateCachedItems()
{
	Super::updateCachedItems();

	updateArrowFlags(getAttribute(QByteArrayLiteral("direction")).toString());
}


void CEdge::updateArrowFlags(const QString& direction)
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

QRectF CEdge::boundingRect() const
{
    return Shape::boundingRect().adjusted(-10,-10,10,10);
}


void CEdge::setupPainter(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget* /*widget*/)
{
	// weight
	double weight = getWeight();

	// line style
	Qt::PenStyle penStyle = (Qt::PenStyle) CUtils::textToPenStyle(getAttribute(QByteArrayLiteral("style")).toString(), Qt::SolidLine);

	// color & selection
	bool isSelected = (option->state & QStyle::State_Selected);
    if (isSelected)
    {
		QPen p(QColor(Qt::darkCyan), weight + 1.0, penStyle, Qt::FlatCap, Qt::MiterJoin);
		painter->setOpacity(0.5);
        painter->setPen(p);
    }
    else
	{
		// get color (to optimize!)
		QColor color = getAttribute(QByteArrayLiteral("color")).value<QColor>();

		QPen p(color, weight, penStyle, Qt::FlatCap, Qt::MiterJoin);

		painter->setOpacity(1.0);
		painter->setPen(p);
	}
}


double CEdge::getWeight() const
{
	bool ok = false;
	double weight = qMax(0.1, getAttribute(QByteArrayLiteral("weight")).toDouble(&ok));
	if (!ok) 
		return 1;
	else
		if (weight > 10) 
			weight = 10;	// safety
	return weight;
}


QLineF CEdge::calculateArrowLine(const QPainterPath &path, bool first, const QLineF &direction) const
{
	//// optimization: disable during drag or pan
	//Qt::MouseButtons buttons = QGuiApplication::mouseButtons();
	//if ((buttons & Qt::LeftButton) || (buttons & Qt::RightButton))
	//	return direction;

	//// optimization: disable during zoom
	//Qt::KeyboardModifiers keys = QGuiApplication::keyboardModifiers();
	//if (keys & Qt::ControlModifier)
	//	return direction;


	if (first && m_firstNode)
	{
		qreal arrowStart = path.percentAtLength(ARROW_SIZE);
		return QLineF(path.pointAtPercent(arrowStart), direction.p2());
	}
	else if (!first && m_lastNode)
	{
		qreal len = path.length();
		qreal arrowStart = path.percentAtLength(len - ARROW_SIZE);
		return QLineF(path.pointAtPercent(arrowStart), direction.p2());
	}

	return direction;
}


void CEdge::drawArrow(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, bool first, const QLineF& direction) const
{
	if (first && m_firstNode)
	{
		drawArrow(painter, 0, direction);
	}
	else if (!first && m_lastNode)
	{
		drawArrow(painter, 0, direction);
	}
}


void CEdge::drawArrow(QPainter* painter, qreal /*shift*/, const QLineF& direction) const
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
	painter->translate(QPointF(0, oldPen.widthF()));
	painter->drawPolygon(arrowHead);

	painter->restore();
}


// IO 

bool CEdge::storeTo(QDataStream &out, quint64 version64) const
{
	Super::storeTo(out, version64);

    out << quint64(m_firstNode) << quint64(m_lastNode);

	// since version 11
	out << m_firstPortId << m_lastPortId;

	return true;
}


bool CEdge::restoreFrom(QDataStream &out, quint64 version64)
{
	if (Super::restoreFrom(out, version64))
	{
		// these are TEMP ids
        out >> m_tempFirstNodeId >> m_tempLastNodeId;

		if (version64 >= 11)
			out >> m_firstPortId >> m_lastPortId;

		return true;
	}

	return false;
}


bool CEdge::linkAfterRestore(const CItemLinkMap &idToItem)
{
    //qDebug() << m_tempFirstNodeId << m_tempLastNodeId;
    auto p1 = idToItem[m_tempFirstNodeId];
    auto p2 = idToItem[m_tempLastNodeId];
    CNode *node1 = dynamic_cast<CNode*>(p1);
    CNode *node2 = dynamic_cast<CNode*>(p2);

    m_firstNode = m_lastNode = nullptr;

	setFirstNode(node1, m_firstPortId);
	setLastNode(node2, m_lastPortId);

	return true;
}


bool CEdge::linkAfterPaste(const CItemLinkMap& idToItem)
{
	bool res = linkAfterRestore(idToItem);

	return res && isValid();
}


// impl

void CEdge::setFirstNode(CNode *node, const QByteArray& portId)
{
    if (m_firstNode && m_firstNode != node)
        m_firstNode->onConnectionDetach(this);

    m_firstNode = node;

	if (m_firstPortId != portId)
		m_firstPortId = portId;

	if (m_firstNode)
        m_firstNode->onConnectionAttach(this);

	onParentGeometryChanged();
}


void CEdge::setLastNode(CNode *node, const QByteArray& portId)
{
    if (m_lastNode && m_lastNode != node)
        m_lastNode->onConnectionDetach(this);

    m_lastNode = node;

	if (m_lastPortId != portId)
		m_lastPortId = portId;

    if (m_lastNode)
        m_lastNode->onConnectionAttach(this);

	onParentGeometryChanged();
}


bool CEdge::reattach(CNode *oldNode, CNode *newNode, const QByteArray& portId)
{
	if (newNode && oldNode == newNode && !newNode->allowCircledConnection())
		return false;

	bool done = false;

	if (m_firstNode == oldNode)
		setFirstNode(newNode, portId), done = true;

	if (m_lastNode == oldNode)
		setLastNode(newNode, portId), done = true;

	return done;
}


bool CEdge::reattach(CNode *node, const QByteArray& oldPortId, const QByteArray& newPortId)
{
	if (node == nullptr)
		return false;

	if (oldPortId == newPortId && !node->allowCircledConnection())
		return false;

	bool done = false;

	if (m_firstNode == node && m_firstPortId == oldPortId)
		setFirstNode(node, newPortId), done = true;

	if (m_lastNode == node && m_lastPortId == oldPortId)
		setLastNode(node, newPortId), done = true;

	return done;
}


void CEdge::reverse()
{
	qSwap(m_firstNode, m_lastNode);
	qSwap(m_firstPortId, m_lastPortId);

	onParentGeometryChanged();
}


// reimp

QString CEdge::createNewId() const
{
	static const QString tmpl("E%1");
	return createUniqueId<CEdge>(tmpl);
}


// callbacks

void CEdge::onNodeMoved(CNode *node)
{
	Q_UNUSED(node);
	Q_ASSERT(node == m_firstNode || node == m_lastNode);
	Q_ASSERT(node != NULL);

	onParentGeometryChanged();
}


void CEdge::onNodeDetached(CNode *node)
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


void CEdge::onNodeDeleted(CNode *node)
{
	onNodeDetached(node);

	delete this;	// die as well
}


void CEdge::onNodePortDeleted(CNode *node, const QByteArray& portId)
{
	reattach(node, portId, "");
}


void CEdge::onNodePortRenamed(CNode *node, const QByteArray& portId, const QByteArray& oldPortId)
{
	if (m_firstNode == node && m_firstPortId == oldPortId)
		m_firstPortId = portId;

	if (m_lastNode == node && m_lastPortId == oldPortId)
		m_lastPortId = portId;
}


// reimp

void CEdge::onItemRestored()
{
	updateCachedItems();

	onParentGeometryChanged();
}


QVariant CEdge::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
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


void CEdge::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
	onHoverEnter(this, event);
}

