/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#ifndef CEDITORSCENE_H
#define CEDITORSCENE_H

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QSet>
#include <QMenu>
#include <QByteArrayList>

#include "CAttribute.h"


class IUndoManager;
class ISceneItemFactory;
class IInteractive;
class ISceneMenuController;
class ISceneEditController;

class CItem;
class CEditorSceneActions;

struct Graph;


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

	friend class CEditorScene_p;

    CEditorScene(QObject *parent = NULL);
	virtual ~CEditorScene();

	virtual void reset();
	virtual void initialize();

	virtual bool fromGraph(const Graph&)	{ return false; }
	virtual bool toGraph(Graph&)			{ return false; }

	// properties
    void setGridSize(int newSize);
    int getGridSize() const				{ return m_gridSize; }

    bool gridEnabled() const			{ return m_gridEnabled; }
    bool gridSnapEnabled() const        { return m_gridSnap; }

    void setGridPen(const QPen& gridPen);
    const QPen& getGridPen() const      { return m_gridPen; }

	void setFontAntialiased(bool on);
	bool isFontAntialiased() const { return m_isFontAntialiased; }
	
	bool itemLabelsEnabled() const		{ return m_labelsEnabled; }
	bool itemLabelsNeedUpdate() const	{ return m_labelsUpdate; }

	enum LabelsPolicy {
		Auto, AlwaysOn, AlwaysOff
	};

	LabelsPolicy getLabelsPolicy() const;
	void setLabelsPolicy(LabelsPolicy v);

	// undo-redo
	int availableUndoCount() const;
	int availableRedoCount() const;
	// must be called after scene state changed
	void addUndoState();
	// must be called to discard recent changes without undo
	void revertUndoState();
	// sets initial scene state
	void setInitialState();

	// serialization 
	virtual bool storeTo(QDataStream& out, bool storeOptions) const;
	virtual bool restoreFrom(QDataStream& out, bool readOptions);

	// item factories
	template<class T>
	bool registerItemFactory() {
		static T f;
		return setItemFactory(&f);
	}

	template<class T>
	static T* factory() { 
		static T s_item; 
		return &s_item; 
	}

	bool setItemFactory(CItem *factoryItem, const QByteArray& typeId = "");
	CItem* getItemFactory(const QByteArray& typeId) const;

	virtual CItem* createItemOfType(const QByteArray& typeId) const;

	template<class T>
	T* createItemOfType(QPointF* at = NULL) const;

	void setItemFactoryFilter(ISceneItemFactory *filter) {
		m_itemFactoryFilter = filter;
	}

	// scene factory & copy
	virtual CEditorScene* createScene() const {
		return new CEditorScene();
	}

	virtual CEditorScene* clone();

	virtual void copyProperties(const CEditorScene& from);

	// attributes
	QByteArray getSuperClassId(const QByteArray& classId) const {
		if (m_classToSuperIds.contains(classId))
			return m_classToSuperIds[classId];

		return QByteArray();
	}
	
	const CAttribute getClassAttribute(const QByteArray& classId, const QByteArray& attrId, bool inherited) const;
	AttributesMap getClassAttributes(const QByteArray& classId, bool inherited) const;

	bool removeClassAttribute(const QByteArray& classId, const QByteArray& attrId);

	void setClassAttribute(const QByteArray& classId, const CAttribute& attr, bool vis = false);
	void setClassAttribute(const QByteArray& classId, const QByteArray& attrId, const QVariant& defaultValue);

	// convenience method to create a class attribute by single call
	CAttribute& createClassAttribute(
		const QByteArray& classId, 
		const QByteArray& attrId, 
		const QString& attrName, 
		const QVariant& defaultValue = QVariant(), 
		int attrFlags = ATTR_FIXED,
		CAttributeConstrains* constrains = NULL,
		bool vis = false);

	QSet<QByteArray> getVisibleClassAttributes(const QByteArray& classId, bool inherited) const;
	void setVisibleClassAttributes(const QByteArray& classId, const QSet<QByteArray>& vis);

	void setClassAttributeVisible(const QByteArray& classId, const QByteArray& attrId, bool vis = true);
	bool isClassAttributeVisible(const QByteArray& classId, const QByteArray& attrId) const;

	CAttributeConstrains* getClassAttributeConstrains(const QByteArray& classId, const QByteArray& attrId) const;
	void setClassAttributeConstrains(const QByteArray& classId, const QByteArray& attrId, CAttributeConstrains* cptr);

	// items
	template<class T = CItem, class L = T>
	QList<T*> getItems() const;

	template<class T = CItem>
	QList<T*> getItemsById(const QString& id) const;

	QGraphicsItem* getItemAt(const QPointF& pos) const;

	template<class T>
	T* isItemAt(const QPointF& pos) const {
		return dynamic_cast<T*>(getItemAt(pos));
	}

	// selections
	QList<QGraphicsItem*> createSelectedList(const CItemsEvaluator&) const;

	template<class T = CItem, class L = T>
	QList<T*> getSelectedItems(bool triggeredIfEmpty = false) const;

	virtual void beginSelection();
	virtual void endSelection();

	void ensureSelectionVisible();

	void moveSelectedItemsBy(double x, double y) {
		moveSelectedItemsBy(QPointF(x, y));
	}

	virtual void moveSelectedItemsBy(const QPointF& d);

	virtual QList<CItem*> cloneSelectedItems();

	virtual int getBoundingMargin() const { return 0; }

	// to reimplement
	virtual QList<QGraphicsItem*> getCopyPasteItems() const;
	virtual QList<QGraphicsItem*> getTransformableItems() const;
 
	// operations
	void startDrag(QGraphicsItem* dragItem);
	void startTransform(bool on);

	// actions
	QObject* getActions();
	CEditorSceneActions* actions();

	// edit extenders
	void setSceneEditController(ISceneEditController *controller);

	ISceneEditController* getSceneEditController() const {
		return m_editController;
	}

	// context menu
	void setContextMenuController(ISceneMenuController *controller) {
		m_menuController = controller;
	}

	ISceneMenuController* getContextMenuController() const {
		return m_menuController;
	}

	QGraphicsItem* getContextMenuTrigger() const {
		return m_menuTriggerItem;
	}

	// other
	bool checkLabelRegion(const QRectF& r);
	void layoutItemLabels();

	void needUpdate();

	virtual QPointF getSnapped(const QPointF& pos) const;

	int getInfoStatus() const {
		return m_infoStatus;
	}

	QGraphicsView* getCurrentView();

	// callbacks
	virtual void onItemDestroyed(CItem *citem);

public Q_SLOTS:
    void enableGrid(bool on = true);
    void enableGridSnap(bool on = true);
	void enableItemLabels(bool on = true);

	void undo();
	void redo();

	void selectAll();
	void deselectAll();
	void selectItems(const QList<CItem*>& items, bool exclusive = true);

	void del();
	void cut();
	void copy();

	void setPastePosition(const QPointF &anchor) { m_pastePos = anchor; }
	void pasteAt(const QPointF &anchor);
	void paste();

	void crop();

	void setSceneCursor(const QCursor& c);

Q_SIGNALS:
	void undoAvailable(bool);
	void redoAvailable(bool);

	void sceneChanged();
	void sceneDoubleClicked(QGraphicsSceneMouseEvent* mouseEvent, QGraphicsItem* clickedItem);

	void infoStatusChanged(int status);

protected:
	void setInfoStatus(int status);

	void updateCursorState();
	virtual bool doUpdateCursorState(Qt::KeyboardModifiers keys, Qt::MouseButtons buttons, QGraphicsItem *hoverItem);

	virtual QObject* createActions();

	// internal call
	void selectUnderMouse(QGraphicsSceneMouseEvent *mouseEvent);

	// reimp
	virtual void drawBackground(QPainter *painter, const QRectF &rect);
	virtual void drawForeground(QPainter *painter, const QRectF &rect);
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent);
	virtual void keyPressEvent(QKeyEvent *keyEvent);
	virtual void keyReleaseEvent(QKeyEvent *keyEvent);
	virtual void focusInEvent(QFocusEvent *focusEvent);
	virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *contextMenuEvent);

	// call from reimp
	void moveDrag(QGraphicsSceneMouseEvent *mouseEvent, QGraphicsItem* dragItem, bool performDrag);
	virtual void processDrag(QGraphicsSceneMouseEvent *mouseEvent, QGraphicsItem* dragItem);
	void finishDrag(QGraphicsSceneMouseEvent *mouseEvent, QGraphicsItem* dragItem, bool dragCancelled);

	// callbacks
	virtual void onDragging(QGraphicsItem* dragItem, const QSet<IInteractive*>& acceptedItems, const QSet<IInteractive*>& rejectedItems);
	virtual void onMoving(QGraphicsSceneMouseEvent* mouseEvent, QGraphicsItem* hoverItem);
	virtual void onDropped(QGraphicsSceneMouseEvent* mouseEvent, QGraphicsItem* dragItem);
	virtual void onLeftButtonPressed(QGraphicsSceneMouseEvent *mouseEvent);
	virtual void onRightButtonPressed(QGraphicsSceneMouseEvent *mouseEvent);
	virtual void onLeftClick(QGraphicsSceneMouseEvent* mouseEvent, QGraphicsItem* clickedItem);
	virtual void onLeftDoubleClick(QGraphicsSceneMouseEvent* mouseEvent, QGraphicsItem* clickedItem);
	// called on drag after single click; returns true if handled
	virtual bool onClickDrag(QGraphicsSceneMouseEvent *mouseEvent, const QPointF &clickPos);
	// called on drag after double click; returns true if handled
	virtual bool onDoubleClickDrag(QGraphicsSceneMouseEvent *mouseEvent, const QPointF &clickPos);

	virtual void onSceneChanged();

protected Q_SLOTS:
	virtual void onSelectionChanged();
	void onFocusItemChanged(QGraphicsItem *newFocusItem, QGraphicsItem *oldFocusItem, Qt::FocusReason reason);
	void onItemEditingFinished(CItem *item, bool cancelled);

	void onActionDelete();
	void onActionSelectAll();
	void onActionEditLabel(CItem *item);

private:
	void removeItems();
	void checkUndoState();

protected:
	QPointF m_leftClickPos;
	QPointF m_mousePos;
	bool m_doubleClick = false;
	bool m_dragInProgress = false;
	QGraphicsItem *m_startDragItem = nullptr;
	QPointF m_lastDragPos;
	QGraphicsItem *m_draggedItem = nullptr;
	QSet<IInteractive*> m_acceptedHovers, m_rejectedHovers;
	bool m_skipMenuEvent = false;
	CItem *m_editItem = nullptr;

private:
	int m_infoStatus;

	QMap<QByteArray, CItem*> m_itemFactories;
	ISceneItemFactory *m_itemFactoryFilter = nullptr;

	IUndoManager *m_undoManager = nullptr;
	bool m_inProgress = false;
	
	QGraphicsItem *m_menuTriggerItem = nullptr;
	ISceneMenuController *m_menuController = nullptr;
	
	QObject *m_actions = nullptr;

	ISceneEditController *m_editController = nullptr;

	QMap<QByteArray, QByteArray> m_classToSuperIds;
	ClassAttributesMap m_classAttributes;
    QMap<QByteArray, QSet<QByteArray>> m_classAttributesVis;
	AttributeConstrainsMap m_classAttributesConstrains;

    int m_gridSize;
    bool m_gridEnabled;
    bool m_gridSnap;
    QPen m_gridPen;

	bool m_needUpdateItems = true;

	QPointF m_pastePos;

	// labels
	QPainterPath m_usedLabelsRegion;
	bool m_labelsEnabled, m_labelsUpdate;

	bool m_isFontAntialiased = true;

	// pimpl
	class CEditorScene_p* m_pimpl = nullptr;
};


// factorization

template<class T>
T* CEditorScene::createItemOfType(QPointF* at) const
{
	if (CItem* item = createItemOfType(T::factoryId()))
	{
		if (T* titem = dynamic_cast<T*>(item))
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
