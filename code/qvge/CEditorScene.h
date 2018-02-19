/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#ifndef CEDITORSCENE_H
#define CEDITORSCENE_H

#include <QGraphicsScene>
#include <QSet>
#include <QMenu>
#include <QByteArrayList>

#include "CAttribute.h"


class CItem;
class IUndoManager;
class ISceneItemFactory;


class CItemsEvaluator
{
public:
	virtual bool evaluate(const QGraphicsItem& item) const = 0;
};


class CDeletableItems: public CItemsEvaluator
{
public:
	virtual bool evaluate(const QGraphicsItem& item) const;
};


class CEditorScene : public QGraphicsScene
{
    Q_OBJECT

public:
	typedef QGraphicsScene Super;

    CEditorScene(QObject *parent);
	virtual ~CEditorScene();

	virtual void reset();
	virtual void initialize();
	virtual void initializeOnce() {}	// static initialization (called once)

	// properties
    void setGridSize(int newSize);
    int getGridSize() const				{ return m_gridSize; }

    bool gridEnabled() const			{ return m_gridEnabled; }
    bool gridSnapEnabled() const        { return m_gridSnap; }

    void setGridPen(const QPen& gridPen);
    const QPen& getGridPen() const      { return m_gridPen; }

	void setSceneCursor(const QCursor& c);

	bool itemLabelsEnabled() const		{ return m_labelsEnabled; }
	bool itemLabelsNeedUpdate() const	{ return m_labelsUpdate; }

	void setFontAntialiased(bool on);
	bool isFontAntialiased() const		{ return m_isFontAntialiased;  }

	// undo-redo
	int availableUndoCount() const;
	int availableRedoCount() const;
	// must be called after scene state changed
	void addUndoState();

	// serialization 
	virtual bool storeTo(QDataStream& out, bool storeOptions) const;
	virtual bool restoreFrom(QDataStream& out, bool readOptions);

	// item factories
	template<class T>
	bool registerItemFactory() {
		static T f;
		return addItemFactory(&f);
	}

	bool addItemFactory(CItem *factoryItem);

	void setItemFactoryFilter(ISceneItemFactory *filter) {
		m_itemFactoryFilter = filter;
	}

	CItem* activateItemFactory(const QByteArray& factoryId);

	virtual CItem* createItemOfType(const QByteArray& typeId) const;

	template<class T>
	T* createItemOfType(QPointF* at = NULL) const;

	CItem* getActiveItemFactory() const {
		return m_activeItemFactory;
	}

	// attributes
	QByteArray getSuperClassId(const QByteArray& classId) const {
		if (m_classToSuperIds.contains(classId))
			return m_classToSuperIds[classId];

		return QByteArray();
	}


	//template<class Class>
	//const CAttribute getClassAttribute(const QByteArray& attrId, bool inherited) const
	//{
	//	if (!Class::factoryId().size())
	//		// fail
	//		return CAttribute();

	//	CAttribute attr = m_classAttributes[Class::classId()][attrId];
	//	if (attr.id.size() || !inherited)
	//		return attr;

	//	// else inherited
	//	return getClassAttribute<Class::Super>(attrId, inherited);
	//}


	const CAttribute getClassAttribute(const QByteArray& classId, const QByteArray& attrId, bool inherited) const;
	AttributesMap getClassAttributes(const QByteArray& classId, bool inherited) const;

	bool removeClassAttribute(const QByteArray& classId, const QByteArray& attrId);

	void setClassAttribute(const QByteArray& classId, const CAttribute& attr, bool vis = false);
	void setClassAttribute(const QByteArray& classId, const QByteArray& attrId, const QVariant& defaultValue);

	// convenience method to create a class attribute by single call
	bool createClassAttribute(const QByteArray& classId, 
		const QByteArray& attrId, const QString& attrName, const QVariant& defaultValue, 
		CAttributeConstrains* constrains = NULL,
		bool vis = false);

	QSet<QByteArray> getVisibleClassAttributes(const QByteArray& classId, bool inherited) const;
	void setClassAttributeVisible(const QByteArray& classId, const QByteArray& attrId, bool vis = true);

	CAttributeConstrains* getClassAttributeConstrains(const QByteArray& classId, const QByteArray& attrId) const;
	void setClassAttributeConstrains(const QByteArray& classId, const QByteArray& attrId, CAttributeConstrains* cptr);

	// selections
	QList<QGraphicsItem*> createSelectedList(const CItemsEvaluator&) const;

	template<class T = CItem, class L = T>
	QList<T*> getSelectedItems(bool triggeredIfEmpty = false) const;

	template<class T = CItem, class L = T>
	QList<T*> getItems() const;

	template<class T = CItem>
	QList<T*> getItemsById(const QString& id) const;

	virtual void beginSelection();
	virtual void endSelection();

	void moveSelectedItemsBy(double x, double y) {
		moveSelectedItemsBy(QPointF(x, y));
	}

	virtual void moveSelectedItemsBy(const QPointF& d);
 
	// callbacks
	virtual void onItemDestroyed(CItem *citem);

	// operations
	void startDrag(QGraphicsItem* dragItem);

	// other
	bool checkLabelRegion(const QRectF& r);
	void layoutItemLabels();

	void needUpdate();

	virtual QPointF getSnapped(const QPointF& pos) const;

public Q_SLOTS:
    void enableGrid(bool on = true);
    void enableGridSnap(bool on = true);
	void enableItemLabels(bool on = true);

	void undo();
	void redo();

	void onActionDelete();
	void onActionSelectAll();
	void onActionEditLabel(CItem *item);

	void selectAll();
	void deselectAll();

	// copy-paste
	void cut();
	void copy();
	void paste();
	void del();

Q_SIGNALS:
	void undoAvailable(bool);
	void redoAvailable(bool);

	void sceneChanged();

protected:
	// reimp
	virtual void drawBackground(QPainter *painter, const QRectF &rect);
	virtual void drawForeground(QPainter *painter, const QRectF &rect);
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent);
	virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *contextMenuEvent);
	virtual void keyPressEvent(QKeyEvent *keyEvent);
	virtual void focusInEvent(QFocusEvent *focusEvent);

	// to reimplement
	virtual bool populateMenu(QMenu& menu, QGraphicsItem* item, const QList<QGraphicsItem*>& selectedItems);
	virtual QList<QGraphicsItem*> copyPasteItems() const;

	// call from reimp
	void moveDrag(QGraphicsSceneMouseEvent *mouseEvent, QGraphicsItem* dragItem, bool performDrag);
	void finishDrag(QGraphicsSceneMouseEvent *mouseEvent, QGraphicsItem* dragItem, bool dragCancelled);
	virtual void updateMovedCursor(QGraphicsSceneMouseEvent *mouseEvent, QGraphicsItem* hoverItem);

	// callbacks
	virtual void onDragging(QGraphicsItem* dragItem, const QSet<CItem*>& acceptedItems, const QSet<CItem*>& rejectedItems);
	virtual void onMoving(QGraphicsSceneMouseEvent* mouseEvent, QGraphicsItem* hoverItem);
	virtual void onDropped(QGraphicsSceneMouseEvent* mouseEvent, QGraphicsItem* dragItem);
	virtual void onLeftClick(QGraphicsSceneMouseEvent* mouseEvent, QGraphicsItem* clickedItem);
	virtual void onLeftDoubleClick(QGraphicsSceneMouseEvent* mouseEvent, QGraphicsItem* clickedItem);
	virtual void onSceneChanged();

private:
	void removeItems();
	void checkUndoState();

protected:
	QPointF m_leftClickPos;
	bool m_doubleClick;
	bool m_dragInProgress;
	QGraphicsItem *m_startDragItem;

	QMap<QByteArray, CItem*> m_itemFactories;
	CItem *m_activeItemFactory;
	ISceneItemFactory *m_itemFactoryFilter = NULL;

	QMap<QByteArray, QByteArray> m_classToSuperIds;

	IUndoManager *m_undoManager;

	ClassAttributesMap m_classAttributes;
    QMap<QByteArray, QSet<QByteArray>> m_classAttributesVis;
	AttributeConstrainsMap m_classAttributesConstrains;

private:
    int m_gridSize;
    bool m_gridEnabled;
    bool m_gridSnap;
    QPen m_gridPen;

	QSet<CItem*> m_acceptedHovers, m_rejectedHovers;

	QGraphicsItem *m_menuTriggerItem;
	QGraphicsItem *m_draggedItem;

	bool m_needUpdateItems;

	// labels
	QPainterPath m_usedLabelsRegion;
	bool m_labelsEnabled, m_labelsUpdate;

	bool m_isFontAntialiased = true;
};


// factorization

template<class T>
T* CEditorScene::createItemOfType(QPointF* at) const
{
	CItem* item = createItemOfType(T::factoryId());
	if (item)
	{
		T* titem = dynamic_cast<T*>(item);
		if (titem)
		{
			if (at)
			{
				(const_cast<CEditorScene*>(this))->addItem(titem);
				titem->setPos(*at);
			}

			return titem;
		}

		delete item;
		return NULL;
	}

	return NULL;
}


// selections

template<class T, class L>
QList<T*> CEditorScene::getSelectedItems(bool triggeredIfEmpty) const
{
	QList<T*> result;

	auto selItems = selectedItems();
	if (selItems.isEmpty() && triggeredIfEmpty && m_menuTriggerItem)
		selItems.append(m_menuTriggerItem);

	for (auto* item : selItems)
	{
		T* titem = dynamic_cast<L*>(item);
		if (titem)
			result.append(titem);
	}

	return result;
}

 
template<class T, class L>
QList<T*> CEditorScene::getItems() const
{
	QList<T*> result;

	auto allItems = items();
	for (auto item : allItems)
	{
		T* titem = dynamic_cast<L*>(item);
		if (titem)
			result.append(titem);
	}

	return result;
}


template<class T>
QList<T*> CEditorScene::getItemsById(const QString& id) const
{
	QList<T*> res;

	auto allItems = items();
	for (auto item : allItems)
	{
		T* titem = dynamic_cast<T*>(item);

		if (titem && titem->getId() == id)
			res << titem;
	}

	return res;
}


#endif // CEDITORSCENE_H
