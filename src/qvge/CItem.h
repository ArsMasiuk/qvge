/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

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
#include "IInteractive.h"


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


class CControlPoint;


class Stub
{
public:
	typedef Stub Super;
	static QByteArray factoryId() { return ""; }
};


class CItem: public IInteractive
{
public:
	typedef Stub Super;

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

	template<class C>
	QString createUniqueId(const QString& Tmpl) const;

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
	virtual QPointF getLabelCenter() const;

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
	virtual void onItemRestored();
	virtual void onItemSelected(bool state);
	virtual void onHoverEnter(QGraphicsItem* sceneItem, QGraphicsSceneHoverEvent* event);

	// call from control points
	virtual void onControlPointMoved(CControlPoint* /*controlPoint*/, const QPointF& /*pos*/) {}
	virtual void onControlPointDelete(CControlPoint* /*controlPoint*/) {}

	// call from drag event
	virtual ItemDragTestResult acceptDragFromItem(QGraphicsItem* /*draggedItem*/) { return Accepted; }

	// called after restoring data (reimplement to update cached attribute values)
	virtual void updateCachedItems();

protected:
	int m_itemFlags;
	int m_internalStateFlags;
	QMap<QByteArray, QVariant> m_attributes;
	QString m_id;
	QGraphicsSimpleTextItem *m_labelItem;

	// restore optimization
	static bool s_duringRestore;
};


template<class C>
QString CItem::createUniqueId(const QString& tmpl) const 
{
	auto editorScene = getScene();
	if (editorScene == NULL)
	{
		static int count = 0;
		return tmpl.arg(++count);
	}

	auto citems = editorScene->getItems<C>();
	QSet<QString> ids;
	for (const auto &citem : citems)
		ids << citem->getId();

	int count = 0;
	QString newId;
	do
		newId = tmpl.arg(++count);
	while (ids.contains(newId));

	return newId;
};


#endif // CITEM_H
