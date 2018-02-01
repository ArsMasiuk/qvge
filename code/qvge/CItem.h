/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#ifndef CITEM_H
#define CITEM_H

#include <QGraphicsItem>
#include <QGraphicsSimpleTextItem> 
#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include "CEditorScene.h"
#include "Properties.h"
#include "CUtils.h"


enum ItemFlags
{
	IF_FramelessSelection = 1,
	IF_DeleteAllowed = 2,
	IF_LastFlag = 4
};


enum ItemStateFlags
{
	IS_Normal = 0,
	IS_Selected = 1,
	IS_Hover = 2,
	IS_Drag_Accepted = 4,
	IS_Drag_Rejected = 8,
	IS_Attribute_Changed = 16,
	IS_Need_Update = 32
};


enum ItemDragTestResult
{
	Rejected,
	Accepted,
	Ignored
};


class CControlPoint;


class CItem
{
public:
	CItem();
	virtual ~CItem();

	int itemFlags() const { return m_itemFlags; }
	void setItemFlags(int f) { m_itemFlags = f; }
	void setItemFlag(int f) { m_itemFlags |= f; }
	void resetItemFlag(int f) { m_itemFlags &= ~f; }

	int itemStateFlags() const { return m_internalStateFlags; }
	void setItemStateFlag(int f) { m_internalStateFlags |= f; }
	void resetItemStateFlag(int f) { m_internalStateFlags &= ~f; }

	// to be reimplemented
	static QByteArray factoryId() { return "CItem"; }
	virtual QByteArray typeId() const { return this->factoryId(); }
	virtual QString createNewId() const { return QString::number((quint64)this); }
	virtual bool setDefaultId();

	// attributes
	virtual bool hasLocalAttribute(const QByteArray& attrId) const;
	const QMap<QByteArray, QVariant>& getLocalAttributes() const { return m_attributes; }

	virtual bool setAttribute(const QByteArray& attrId, const QVariant& v);
	virtual bool removeAttribute(const QByteArray& attrId);
	virtual QVariant getAttribute(const QByteArray& attrId) const;

	virtual QByteArray classId() const { return "item"; }
	virtual QByteArray superClassId() const { return QByteArray(); }

	QString getId() const { return m_id; }
	void setId(const QString& id) { setAttribute("id", id); }

	enum VisibleFlags { VF_ANY = 0, VF_LABEL = 1, VF_TOOLTIP = 2 };
	virtual QSet<QByteArray> getVisibleAttributeIds(int flags) const;

	// scene access
	QGraphicsItem* getSceneItem() const {
		return dynamic_cast<QGraphicsItem*>((CItem*)this);
	}

	CEditorScene* getScene() const;

	void addUndoState();

	// labels
	virtual void updateLabelContent();
	virtual void updateLabelDecoration();
	virtual void updateLabelPosition() {}
	void setLabelText(const QString& text);
	void showLabel(bool on);
	QRectF getSceneLabelRect() const;

	// serialization 
	virtual bool storeTo(QDataStream& out, quint64 version64) const;
	virtual bool restoreFrom(QDataStream& out, quint64 version64);

	typedef QMap<quint64, CItem*> CItemLinkMap;
	virtual bool linkAfterRestore(const CItemLinkMap& /*idToItem*/) { return true; }
	virtual bool linkAfterPaste(const CItemLinkMap& idToItem) { return linkAfterRestore(idToItem); }	// default the same
	static void beginRestore() { s_duringRestore = true; }
	static void endRestore() { s_duringRestore = false; }

	// returns new item of this class
	virtual CItem* clone() = 0;
	virtual CItem* create() const = 0;

	// copy data from item
	virtual void copyDataFrom(CItem* from);

	// callbacks
	virtual void onItemMoved(const QPointF& /*delta*/) {}
	virtual void onItemRestored();
	virtual void onItemSelected(bool state);
	virtual void onHoverEnter(QGraphicsItem* sceneItem, QGraphicsSceneHoverEvent* event);
	virtual void onHoverLeave(QGraphicsItem* /*sceneItem*/, QGraphicsSceneHoverEvent* /*event*/) {}
	virtual void onDraggedOver(const QSet<CItem*>& /*acceptedItems*/, const QSet<CItem*>& /*rejectedItems*/) {}
	virtual void onDroppedOn(const QSet<CItem*>& /*acceptedItems*/, const QSet<CItem*>& /*rejectedItems*/) {}
	virtual void onClick(QGraphicsSceneMouseEvent* /*mouseEvent*/) {}
	virtual bool onClickDrag(QGraphicsSceneMouseEvent* /*mouseEvent*/, const QPointF& /*clickPos*/) { return false; }
	virtual bool onDoubleClickDrag(QGraphicsSceneMouseEvent* /*mouseEvent*/, const QPointF& /*clickPos*/) { return false; }

	// call from control points
	virtual void onControlPointMoved(CControlPoint* /*controlPoint*/, const QPointF& /*pos*/) {}
	virtual void onControlPointDelete(CControlPoint* /*controlPoint*/) {}

	// call from drag event
	virtual ItemDragTestResult acceptDragFromItem(QGraphicsItem* /*draggedItem*/) { return Accepted; }
	virtual void leaveDragFromItem(QGraphicsItem* /*draggedItem*/) {}

	// called after restoring data (reimplement to update cached attribute values)
	virtual void updateCachedItems() {}

protected:
	int m_itemFlags;
	int m_internalStateFlags;
	QMap<QByteArray, QVariant> m_attributes;
	QString m_id;
	QGraphicsSimpleTextItem *m_labelItem;

	// restore optimization
	static bool s_duringRestore;
};


#endif // CITEM_H
