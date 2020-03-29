/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#ifndef CNODE_H
#define CNODE_H

#include "CItem.h"
#include "CNodePort.h"

#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QSet>

 
class CEdge;


enum NodeFlags
{
	NF_OrphanAllowed = 1	// allow node to have no connections
};


class CNode : public CItem, public QGraphicsRectItem
{
public:
	typedef CItem Super;
	typedef QGraphicsRectItem Shape;

	CNode(QGraphicsItem* parent = NULL);
	virtual ~CNode();

	static QByteArray factoryId()		{ return "CNode"; }
	virtual QByteArray typeId() const	{ return this->factoryId(); }
	virtual QString createNewId() const;

	int nodeFlags() const				{ return m_nodeFlags; }
	void setNodeFlags(int f)			{ m_nodeFlags = f; }
	void setNodeFlag(int f)				{ m_nodeFlags |= f; }
	void resetNodeFlag(int f)			{ m_nodeFlags &= ~f; }

	// reimp
	virtual CItem* create() const		{ return new CNode(parentItem()); }
	virtual CItem* clone();
	virtual void copyDataFrom(CItem* from);

	virtual QSizeF getSize() const		{ return rect().size(); }
	virtual void setSize(float w, float h);

	// attributes
	virtual bool hasLocalAttribute(const QByteArray& attrId) const;
	virtual bool setAttribute(const QByteArray& attrId, const QVariant& v);
	virtual bool removeAttribute(const QByteArray& attrId);
	virtual QVariant getAttribute(const QByteArray& attrId) const;
    virtual QByteArray classId() const { return "node"; }
    virtual QByteArray superClassId() const { return Super::classId(); }

	// ports
	CNodePort* addPort(const QByteArray& portId = "", int align = Qt::AlignCenter, double xoff = 0, double yoff = 0);
	bool removePort(const QByteArray& portId);
	bool movePort(const QByteArray& portId, int align = Qt::AlignCenter, double xoff = 0, double yoff = 0);
	bool renamePort(const QByteArray& portId, const QByteArray& newId);
	CNodePort* getPort(const QByteArray& portId) const;
	QByteArrayList getPortIds() const;

	// serialization 
	virtual bool storeTo(QDataStream& out, quint64 version64) const;
	virtual bool restoreFrom(QDataStream& out, quint64 version64);

	// merges node with the current one.
	// node will be deleted afterwards if no circled connection allowed.
	virtual bool merge(CNode *node, const QByteArray& portId = "");

	// splits all the connections from this node.
	// result is the list of the newly created nodes (or empty list if connections < 2).
	virtual QList<CNode*> unlink();

	// returns all nodes colliding with this one.
	QList<CNode*> getCollidingNodes() const;
	
	// returns all connections of the node.
	QSet<CEdge*> getConnections() const { return m_connections; }

    // returns all incoming connections of the node.
    QSet<CEdge*> getInConnections() const;

    // returns all outgoing connections of the node.
    QSet<CEdge*> getOutConnections() const;

	// returns true if new connection from this node is allowed.
	virtual bool allowStartConnection() const { return true; }

	// returns true if a connection from this node to itself is allowed.
	virtual bool allowCircledConnection() const { return true; }

	// calculates point on the node's outline intersecting with line.
	virtual QPointF getIntersectionPoint(const QLineF& line, const QByteArray& portId) const;

	// callbacks
	virtual void onConnectionAttach(CEdge *conn);
	virtual void onConnectionDetach(CEdge *conn);
	virtual void onConnectionDeleted(CEdge *conn);

	virtual void onPortDeleted(CNodePort *port);
	virtual void onPortRenamed(CNodePort *port, const QByteArray& oldId);

	virtual void onItemMoved(const QPointF& delta);
	virtual void onItemRestored();
	virtual void onDroppedOn(const QSet<IInteractive*>& acceptedItems, const QSet<IInteractive*>& rejectedItems);
	virtual ItemDragTestResult acceptDragFromItem(QGraphicsItem* draggedItem);

	// reimp 
	virtual QRectF boundingRect() const;

protected:
	// reimp 
	virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value);
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event); 

	virtual void updatePortsLayout();
	virtual void updateLabelPosition();
	virtual void updateCachedItems();

private:
	void recalculateShape();
	void updateConnections();

	void resize(float size)			{ setRect(-size / 2, -size / 2, size, size); }
	void resize(float w, float h)	{ setRect(-w / 2, -h / 2, w, h); }
	void resize(const QSizeF& size) { resize(size.width(), size.height()); }

protected:
	QSet<CEdge*> m_connections;
	int m_nodeFlags = 0;

	QMap<QByteArray, CNodePort*> m_ports;

	QPolygonF m_shapeCache;
	QRectF m_sizeCache;
};



#endif // CNODE_H
