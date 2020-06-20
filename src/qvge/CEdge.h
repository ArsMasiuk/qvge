/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#ifndef CEdge_H
#define CEdge_H

#include <QGraphicsLineItem>
#include <QByteArray>
#include <QtGlobal>

#include "CItem.h"

class CNode;


enum ConnectionFlags	// extends ItemFlags
{
	CF_Start_Arrow		= IF_LastFlag,
	CF_End_Arrow		= IF_LastFlag << 2,
	CF_Mutual_Arrows	= CF_Start_Arrow | CF_End_Arrow		// start | end
};


class CEdge : public CItem, public QGraphicsLineItem
{
public:
	typedef CItem Super;
	typedef QGraphicsLineItem Shape;

    CEdge(QGraphicsItem *parent = Q_NULLPTR);
	virtual ~CEdge();

	// public
    void setFirstNode(CNode *node, const QByteArray& portId = "");
    void setLastNode(CNode *node, const QByteArray& portId = "");

	bool reattach(CNode *oldNode, CNode *newNode, const QByteArray& portId = "");
	bool reattach(CNode *node, const QByteArray& oldPortId, const QByteArray& newPortId);

	CNode* firstNode() const { return m_firstNode; }
	CNode* lastNode() const { return m_lastNode; }

	const QByteArray& firstPortId() const { return m_firstPortId; }
	const QByteArray& lastPortId() const { return m_lastPortId; }

	bool isValid() const	{ return m_firstNode != NULL && m_lastNode != NULL; }
	bool isCircled() const	{ return isValid() && m_firstNode == m_lastNode; }

	virtual void reverse();

	// reimp
	virtual QString createNewId() const;

	// reimp
	virtual ItemDragTestResult acceptDragFromItem(QGraphicsItem* /*draggedItem*/) { return Ignored; }

	// reimp
	virtual QRectF boundingRect() const;
	virtual QPainterPath shape() const { return m_selectionShapePath; }

	// attributes
	virtual bool hasLocalAttribute(const QByteArray& attrId) const;
	virtual bool setAttribute(const QByteArray& attrId, const QVariant& v);
	virtual bool removeAttribute(const QByteArray& attrId);

	// serialization 
	virtual bool storeTo(QDataStream& out, quint64 version64) const;
	virtual bool restoreFrom(QDataStream& out, quint64 version64);
	virtual bool linkAfterRestore(const CItemLinkMap& idToItem);
	virtual bool linkAfterPaste(const CItemLinkMap& idToItem);

    // callbacks
	virtual void onNodeMoved(CNode *node);
	virtual void onNodeDetached(CNode *node);
	virtual void onNodeDeleted(CNode *node);
	virtual void onNodePortDeleted(CNode *node, const QByteArray& portId);
	virtual void onNodePortRenamed(CNode *node, const QByteArray& portId, const QByteArray& oldId);
	virtual void onParentGeometryChanged() = 0;
	virtual void onItemRestored();

protected:
	virtual void setupPainter(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);
	virtual void drawArrow(QPainter *painter, const QStyleOptionGraphicsItem *option, bool first, const QLineF &direction) const;
	virtual void drawArrow(QPainter *painter, qreal shift, const QLineF &direction) const;
	QLineF calculateArrowLine(const QPainterPath &path, bool first, const QLineF &direction) const;

	// reimp
	virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value);
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);

	// cached attributes
	virtual void updateCachedItems();
	virtual void updateArrowFlags(const QString& direction);

	double getWeight() const;

protected:
    CNode *m_firstNode = nullptr;
    quint64 m_tempFirstNodeId = 0;

    CNode *m_lastNode = nullptr;
    quint64 m_tempLastNodeId = 0;

	QByteArray m_firstPortId, m_lastPortId;

	QPainterPath m_selectionShapePath;
	QPainterPath m_shapeCachePath;

	const int ARROW_SIZE = 6;
};


#endif // CEdge_H
