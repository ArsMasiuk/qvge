/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016 Ars L.Masiuk(ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CEditorScene.h"
#include "CItem.h"
#include "CSimpleUndoManager.h"
#include "CDiffUndoManager.h"
#include "IContextMenuProvider.h"
#include "ISceneItemFactory.h"

#include <QPainter>
#include <QPaintEngine>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QMessageBox>
#include <QApplication>
#include <QKeyEvent>
#include <QInputDialog>
#include <QMimeData>
#include <QClipboard>
#include <QDebug>
#include <QElapsedTimer>
#include <QPixmapCache> 

#include <qopengl.h>


const quint64 version64 = 9;	// build
const char* versionId = "VersionId";


CEditorScene::CEditorScene(QObject *parent): QGraphicsScene(parent), 
    m_doubleClick(false),
    m_startDragItem(NULL),
    m_activeItemFactory(NULL),
    //m_undoManager(new CSimpleUndoManager(*this)),
	m_undoManager(new CDiffUndoManager(*this)),
	m_menuTriggerItem(NULL),
    m_draggedItem(NULL),
	m_dragInProgress(false),
    m_needUpdateItems(true)
{
    m_gridSize = 25;
    m_gridEnabled = true;
    m_gridSnap = false;
    m_gridPen = QPen(Qt::gray, 0, Qt::DotLine);

	m_labelsEnabled = true;
	m_labelsUpdate = false;

	setBackgroundBrush(QColor("#f3ffe1"));

    setSceneRect(-500, -500, 1000, 1000);

	// optimizations
    setItemIndexMethod(QGraphicsScene::NoIndex);
    
	setMinimumRenderSize(5);

	QPixmapCache::setCacheLimit(200000);
}

CEditorScene::~CEditorScene()
{
	disconnect();
	clear();
}

void CEditorScene::reset()
{
	initialize();

	m_undoManager->reset();

	setSceneRect(QRectF(-500,-500,1000,1000));
}

void CEditorScene::initialize()
{
	removeItems();

	m_classAttributes.clear();
	m_classAttributesVis.clear();
	//qDeleteAll(m_classAttributesConstrains);
	m_classAttributesConstrains.clear();

	// default item attrs
    CAttribute labelAttr("label", "Label", "");
	labelAttr.noDefault = true;
	setClassAttribute("item", labelAttr, true);

	CAttribute labelColorAttr("label.color", "Label Color", QColor(Qt::black));
	setClassAttribute("item", labelColorAttr);

	QFont labelFont;
	CAttribute labelFontAttr("label.font", "Label Font", labelFont);
	setClassAttribute("item", labelFontAttr);

    CAttribute idAttr("id", "ID", "");
	idAttr.noDefault = true;
	setClassAttribute("item", idAttr, true);

	// static init
	static bool s_init = false;
	if (!s_init)
	{
		s_init = true;
		initializeOnce();
	}
}

void CEditorScene::removeItems()
{
	CItem::beginRestore();

	deselectAll();

	while (!items().isEmpty())
		delete items().first();

	clear();

	CItem::endRestore();
}


// properties

void CEditorScene::setGridSize(int newSize)
{
    if (newSize <= 0)
        return;

    m_gridSize = newSize;

    update();
}

void CEditorScene::enableGrid(bool on)
{
    m_gridEnabled = on;

    update();
}

void CEditorScene::enableGridSnap(bool on)
{
    m_gridSnap = on;
}

void CEditorScene::setGridPen(const QPen &gridPen)
{
    m_gridPen = gridPen;

    update();
}

void CEditorScene::enableItemLabels(bool on)
{
	m_labelsEnabled = on;

	layoutItemLabels();
}


void CEditorScene::setFontAntialiased(bool on)
{
	m_isFontAntialiased = on;

	layoutItemLabels();

	update();
}


// undo-redo

void CEditorScene::undo()
{
	if (m_undoManager)
	{
		m_undoManager->undo();

		checkUndoState();

		onSceneChanged();
	}
}

void CEditorScene::redo()
{
	if (m_undoManager)
	{
		m_undoManager->redo();

		checkUndoState();

		onSceneChanged();
	}
}

void CEditorScene::addUndoState()
{
	// canvas size
    QRectF minRect(sceneRect());
    minRect |= itemsBoundingRect().adjusted(-20, -20, 20, 20);
    setSceneRect(minRect);

	// undo-redo
	if (m_undoManager)
	{
		m_undoManager->addState();

		checkUndoState();
	}

	// notification
	onSceneChanged();
}


int CEditorScene::availableUndoCount() const
{ 
	return m_undoManager ? m_undoManager->availableUndoCount() : 0; 
}

int CEditorScene::availableRedoCount() const 
{ 
	return m_undoManager ? m_undoManager->availableRedoCount() : 0; 
}

void CEditorScene::checkUndoState()
{
	Q_EMIT undoAvailable(m_undoManager->availableUndoCount() > 0);
	Q_EMIT redoAvailable(m_undoManager->availableRedoCount() > 0);
}


// io

bool CEditorScene::storeTo(QDataStream& out, bool storeOptions) const
{
    out << versionId << version64;

	// items
	QMap<CItem*, uint> sortedMap;

	QList<QGraphicsItem*> allItems = items();

	for (QGraphicsItem* item : allItems)
	{
		CItem* citem = dynamic_cast<CItem*>(item);
		if (citem)
		{
            sortedMap[citem] = quint64(citem);
		}
	}

	for (CItem* citem : sortedMap.keys())
	{
        out << citem->typeId() << quint64(citem);

		citem->storeTo(out, version64);
	}

	// attributes
	out << QByteArray("_attr_");
	out << (quint64)0x12345678;

	out << m_classAttributes.size();
	for (auto classAttrsIt = m_classAttributes.constBegin(); classAttrsIt != m_classAttributes.constEnd(); ++classAttrsIt)
	{
		out << classAttrsIt.key();
		out << classAttrsIt.value().size();
		for (auto attr : classAttrsIt.value())
		{
			attr.storeTo(out, version64);
		}
	}

	out << m_classToSuperIds;

	// visible attributes
	out << m_classAttributesVis;

	// options
	if (storeOptions)
	{
		out << backgroundBrush();
		out << m_gridPen;
		out << m_gridSize;
		out << m_gridEnabled << m_gridSnap;
	}

	// 9+: scene rect
	out << sceneRect();

	return true;
}

bool CEditorScene::restoreFrom(QDataStream& out, bool readOptions)
{
	initialize();

	// version
	quint64 storedVersion = 0;

	// read
	CItem::CItemLinkMap idToItem;

	while (!out.atEnd())
	{
		QByteArray id; out >> id;
        quint64 ptrId; out >> ptrId;

		if (storedVersion == 0 && strcmp(id.data(), versionId) == 0)
		{
			storedVersion = ptrId;
			out >> id >> ptrId;
		}

		// started attr section
		if (storedVersion >= 3 && id == "_attr_" && ptrId == 0x12345678)
			break;

		CItem* item = createItemOfType(id);
		if (item)
		{
			if (item->restoreFrom(out, storedVersion))
			{
                idToItem[ptrId] = item;
				continue;
			}
		}

		// failed: cleanup
		qDeleteAll(idToItem.values());

		return false;
	}

	// link items
	CItem::beginRestore();

	for (CItem* item : idToItem.values())
	{
		if (item->linkAfterRestore(idToItem))
		{
			addItem(dynamic_cast<QGraphicsItem*>(item));
		}
		else
		{
			// failed: cleanup
			qDeleteAll(idToItem.values());

			clear();

			CItem::endRestore();

			return false;
		}
	}

	// attributes
	if (storedVersion >= 3)
	{
		int classAttrSize = 0;
		out >> classAttrSize;

		for (int i = 0; i < classAttrSize; ++i)
		{
			QByteArray classId;
			if (storedVersion >= 6)
				out >> classId;

			int attrSize = 0;
			out >> attrSize;

			for (int j = 0; j < attrSize; ++j)
			{
				CAttribute attr;
				if (attr.restoreFrom(out, storedVersion))
				{
					if (storedVersion < 6)
						classId = attr.classId;		// deprecated for now

					setClassAttribute(classId, attr);
				}
				else
				{
					CItem::endRestore();

					return false;
				}
			}
		}
	}

	// visible attributes
	if (storedVersion >= 5)
	{
		out >> m_classToSuperIds;
		out >> m_classAttributesVis;
	}

	// options
	if (readOptions && storedVersion >= 8)
	{
		QBrush b;
		out >> b; setBackgroundBrush(b);
		out >> m_gridPen;
		out >> m_gridSize;
		out >> m_gridEnabled >> m_gridSnap;
	}

	// scene rect
	if (storedVersion >= 9)
	{
		QRectF sr; 
		out >> sr;
		setSceneRect(sr);
	}

	// finish
	CItem::endRestore();

	for (CItem* item : idToItem.values())
	{
		item->onItemRestored();
	}

	return true;
}


// factorization

bool CEditorScene::addItemFactory(CItem *factoryItem)
{
	if (factoryItem)
	{
		// register class inheritance
		QByteArray classId = factoryItem->classId();
		QByteArray superClassId = factoryItem->superClassId();
		m_classToSuperIds[classId] = superClassId;

		QByteArray id = factoryItem->typeId();

		// already registered?
		if (m_itemFactories.contains(id))
		{
			return (m_itemFactories[id] == factoryItem);
		}

		m_itemFactories[id] = factoryItem;
		return true;
	}

	return false;
}


CItem* CEditorScene::activateItemFactory(const QByteArray &factoryId)
{
	if (factoryId.isEmpty() || !m_itemFactories.contains(factoryId))
	{
		m_activeItemFactory = NULL;
	}
	else
	{
		m_activeItemFactory = m_itemFactories[factoryId];
	}

	return NULL;
}


CItem* CEditorScene::createItemOfType(const QByteArray &id) const
{
	// check for filter
	if (m_itemFactoryFilter)
	{
		if (CItem* item = m_itemFactoryFilter->createItemOfType(id, *this))
			return item;
	}

	// else default creation
	if (m_itemFactories.contains(id))
	{
		return m_itemFactories[id]->create();
	}

	return NULL;
}


// attributes

bool CEditorScene::createClassAttribute(const QByteArray& classId,
	const QByteArray& attrId, const QString& attrName, const QVariant& defaultValue,
	CAttributeConstrains* constrains,
	bool vis) 
{
	if (m_classAttributes[classId].contains(attrId))
		return false;

	CAttribute attr(attrId, attrName, defaultValue);

	m_classAttributes[classId][attrId] = attr;

	setClassAttributeVisible(classId, attrId, vis);

	if (constrains)
		setClassAttributeConstrains(classId, attrId, constrains);

	return true;
}


void CEditorScene::setClassAttribute(const QByteArray& classId, const CAttribute& attr, bool vis)
{
	// only update value if exists 
	if (m_classAttributes[classId].contains(attr.id))
		m_classAttributes[classId][attr.id].defaultValue = attr.defaultValue;
	else 
		// else insert
		m_classAttributes[classId][attr.id] = attr;

	setClassAttributeVisible(classId, attr.id, vis);

	needUpdate();
}


void CEditorScene::setClassAttribute(const QByteArray& classId, const QByteArray& attrId, const QVariant& defaultValue)
{
	// clone from super if not found
	if (!m_classAttributes[classId].contains(attrId))
	{
		auto superId = getSuperClassId(classId);
		while (!superId.isEmpty() && !m_classAttributes[superId].contains(attrId))
		{
			superId = getSuperClassId(superId);
		}

		if (!superId.isEmpty())
		{
			auto attr = m_classAttributes[superId][attrId];
			attr.defaultValue = defaultValue;
			m_classAttributes[classId][attrId] = attr;
			
			needUpdate();
		}

		return;
	}

	// else just update the value
	m_classAttributes[classId][attrId].defaultValue = defaultValue;

	needUpdate();
}


bool CEditorScene::removeClassAttribute(const QByteArray& classId, const QByteArray& attrId)
{
	auto it = m_classAttributes.find(classId);
	if (it == m_classAttributes.end())
		return false;

	needUpdate();

	return (*it).remove(attrId);
}


void CEditorScene::setClassAttributeVisible(const QByteArray& classId, const QByteArray& attrId, bool vis)
{
	if (vis)
		m_classAttributesVis[classId].insert(attrId);
	else
		m_classAttributesVis[classId].remove(attrId);

	// set label update flag
	m_labelsUpdate = true;

	// schedule update
	invalidate();
}


QSet<QByteArray> CEditorScene::getVisibleClassAttributes(const QByteArray& classId, bool inherited) const
{
	QSet<QByteArray> result = m_classAttributesVis[classId];

	if (inherited)
	{
		QByteArray superId = getSuperClassId(classId);
		while (!superId.isEmpty())
		{
			result = result.unite(m_classAttributesVis[superId]);
			superId = getSuperClassId(superId);
		}
	}

	return result;
}


const CAttribute CEditorScene::getClassAttribute(const QByteArray& classId, const QByteArray& attrId, bool inherited) const 
{
	if (classId.isEmpty())
		// fail
		return CAttribute();

	CAttribute attr = m_classAttributes[classId][attrId];
	if (attr.id.size() || !inherited)
		return attr;

	// else inherited
	QByteArray superId = getSuperClassId(classId);
	return getClassAttribute(superId, attrId, true);
}


AttributesMap CEditorScene::getClassAttributes(const QByteArray& classId, bool inherited) const
{
	AttributesMap result = m_classAttributes[classId];

	if (inherited)
	{
		QByteArray superId = getSuperClassId(classId);
		while (!superId.isEmpty())
		{
			//result = result.unite(m_classAttributes[superId]);
			// unite does not check for existing elements :(
			// there must be insertUnique
            CUtils::insertUnique(result, m_classAttributes[superId]);

			superId = getSuperClassId(superId);
		}
	}

	return result;
}


CAttributeConstrains* CEditorScene::getClassAttributeConstrains(const QByteArray& classId, const QByteArray& attrId) const
{
	ClassAttrIndex index(classId, attrId);

	if (m_classAttributesConstrains.contains(index))
		return m_classAttributesConstrains[index];
	else
		return NULL;
}


void CEditorScene::setClassAttributeConstrains(const QByteArray& classId, const QByteArray& attrId, CAttributeConstrains* cptr)
{
	ClassAttrIndex index(classId, attrId);

	// do we need to clean up?
	//if (m_classAttributesConstrains.contains(index))
	//	delete m_classAttributesConstrains[index];

	if (cptr)
		m_classAttributesConstrains[index] = cptr;
	else
		m_classAttributesConstrains.remove(index);
}


// copy-paste

QList<QGraphicsItem*> CEditorScene::copyPasteItems() const
{
	return selectedItems();
}


void CEditorScene::cut()
{
	copy();
	del();
}


void CEditorScene::del()
{
	QList<QGraphicsItem*> itemList = createSelectedList(CDeletableItems());
	if (itemList.isEmpty())
		return;

	for (QGraphicsItem* item : itemList)
	{
		if (items().contains(item))
			delete item;
	}

	addUndoState();
}


void CEditorScene::copy()
{
	// store selected items only
	QMap<CItem*, uint> sortedMap;

	QList<QGraphicsItem*> allItems = copyPasteItems();

	for (QGraphicsItem* item : allItems)
	{
		CItem* citem = dynamic_cast<CItem*>(item);
		if (citem)
		{
			sortedMap[citem] = quint64(citem);
		}
	}

	if (sortedMap.isEmpty())
	{
		QApplication::clipboard()->clear();
		return;
	}

	// write version and items
	QByteArray buffer;
	QDataStream out(&buffer, QIODevice::WriteOnly);

	out << version64;

	for (CItem* citem : sortedMap.keys())
	{
		out << citem->typeId() << quint64(citem);

		citem->storeTo(out, version64);
	}

	// create mime object
	QMimeData* mimeData = new QMimeData;
	mimeData->setData("qvge/selection", buffer);
	QApplication::clipboard()->setMimeData(mimeData);
}


void CEditorScene::paste()
{
	deselectAll();

	const QClipboard *clipboard = QApplication::clipboard();
	const QMimeData *mimeData = clipboard->mimeData();
	if (mimeData == NULL)
		return;
	if (!mimeData->hasFormat("qvge/selection"))
		return;

	// read items from the buffer
	QByteArray buffer = mimeData->data("qvge/selection");
	QDataStream out(buffer);

	// version
	quint64 storedVersion = 0;
	out >> storedVersion;

	CItem::CItemLinkMap idToItem;
	QList<CItem*> deathList, lifeList;

	while (!out.atEnd())
	{
		QByteArray typeId; out >> typeId;
		quint64 ptrId; out >> ptrId;

		CItem* item = createItemOfType(typeId);
		if (item)
		{
			if (item->restoreFrom(out, storedVersion))
			{
				idToItem[ptrId] = item;
			}
			else
				deathList << item;
		}
	}

	// link items
	QSignalBlocker blocker(this);

	for (CItem* item : idToItem.values())
	{
		if (item->linkAfterPaste(idToItem))
		{
			auto sceneItem = dynamic_cast<QGraphicsItem*>(item);
			addItem(sceneItem);
			sceneItem->setSelected(true);
			
			lifeList << item;
		}
		else
			deathList << item;
	}

	// cleanup
	qDeleteAll(deathList);

	if (lifeList.isEmpty())
		return;

	// shift & rename pasted items which were not removed
	QMap<QString, int> ids;
	auto allItems = getItems<CItem>();
	for (auto item : allItems)
		ids[item->getId()]++;

	// shift
	moveSelectedItemsBy(100, 0);

	auto selItems = selectedItems();
	for (auto sceneItem : selItems)
	{
		//sceneItem->moveBy(100, 0);

		CItem* item = dynamic_cast<CItem*>(sceneItem);
		if (item)
		{
			QString id = item->getId();
			if (ids[id] > 1)
			{
				int counter = 1;
				QString newId = id;

				while (ids.contains(newId))
					newId = QString("Copy%1 of %2").arg(counter++).arg(id);

				item->setId(newId);
			}
		}
	}

	for (CItem* item : idToItem.values())
	{
		item->onItemRestored();
	}

	blocker.unblock();
	Q_EMIT selectionChanged();

	// finish
	addUndoState();
}


// callbacks

void CEditorScene::onItemDestroyed(CItem *citem)
{
	Q_ASSERT(citem);
}


void CEditorScene::onSceneChanged()
{
	Q_EMIT sceneChanged();

	layoutItemLabels();
}


// drawing

void CEditorScene::drawBackground(QPainter *painter, const QRectF &)
{
	// invalidate items if needed
	if (m_needUpdateItems)
	{
		m_needUpdateItems = false;
		auto citems = getItems<CItem>();
		for (auto citem : citems)
		{
			citem->updateCachedItems();
			citem->getSceneItem()->update();
		}
	}

	// update layout if needed
	if (m_labelsUpdate)
	{
		layoutItemLabels();
	}

	// fill background
	if (painter->paintEngine()->type() == QPaintEngine::OpenGL || painter->paintEngine()->type() == QPaintEngine::OpenGL2)
	{
		glClearColor(1, 1, 1, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	painter->setPen(QPen(Qt::darkGray, 2, Qt::SolidLine));
	painter->setBrush(backgroundBrush());
	painter->drawRect(sceneRect());

	// draw grid if needed
	if (m_gridSize < 0 || !m_gridEnabled)
		return;

	painter->setPen(m_gridPen);

	QRectF rect = sceneRect();

	qreal left = int(rect.left()) - (int(rect.left()) % m_gridSize);
	qreal top = int(rect.top()) - (int(rect.top()) % m_gridSize);

	QVarLengthArray<QLineF, 100> lines;

	for (qreal x = left; x < rect.right(); x += m_gridSize)
		lines.append(QLineF(x, rect.top(), x, rect.bottom()));
	for (qreal y = top; y < rect.bottom(); y += m_gridSize)
		lines.append(QLineF(rect.left(), y, rect.right(), y));

	//qDebug() << lines.size();

	painter->drawLines(lines.data(), lines.size());
}


void CEditorScene::drawForeground(QPainter *painter, const QRectF &r)
{
    Super::drawForeground(painter, r);

	// drop label update flag
	m_labelsUpdate = false;
}


bool CEditorScene::checkLabelRegion(const QRectF &r)
{
	if (!r.isValid())
		return false;

	if (m_usedLabelsRegion.intersects(r))
		return false;

	m_usedLabelsRegion.addRect(r);
	return true;
}


void CEditorScene::layoutItemLabels()
{
	// reset region
	m_usedLabelsRegion = QPainterPath();

	QList<CItem*> allItems = getItems<CItem>();

	// hide all if disabled
	if (!m_labelsEnabled)
	{
		for (auto citem : allItems)
		{
			citem->showLabel(false);
		}

		return;
	}

	QElapsedTimer tm;
	tm.start();

	// else layout texts
	for (auto citem : allItems)
	{
		citem->updateLabelContent();

		citem->updateLabelPosition();

		QRectF labelRect = citem->getSceneLabelRect();
		QRectF reducedRect(labelRect.topLeft() / 10, labelRect.size() / 10);
		
		citem->showLabel(checkLabelRegion(reducedRect));
	}

	//qDebug() << "layout labels: " << tm.elapsed();
}


void CEditorScene::needUpdate()
{
	//m_labelsUpdate = true;
	m_needUpdateItems = true;

	update();
}


// mousing

void CEditorScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
	// workaround: do not deselect selected items by RMB
	if (mouseEvent->button() != Qt::LeftButton)
	{
		mouseEvent->accept();
		return;
	}

	Super::mousePressEvent(mouseEvent);

	if (mouseEvent->button() == Qt::LeftButton)
	{
		m_draggedItem = NULL;
		m_dragInProgress = false;

		m_leftClickPos = mouseEvent->scenePos();
	}
}


void CEditorScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
	Super::mouseMoveEvent(mouseEvent);

	m_draggedItem = mouseGrabberItem();

	moveDrag(mouseEvent, m_draggedItem, false);
}


void CEditorScene::startDrag(QGraphicsItem* dragItem)
{
	m_startDragItem = dragItem;
	m_dragInProgress = true;
}


void CEditorScene::moveDrag(QGraphicsSceneMouseEvent *mouseEvent, QGraphicsItem* dragItem, bool performDrag)
{
	//m_leftClickPos = QPointF();
	//m_doubleClick = false;
	m_dragInProgress = true;

	if (dragItem)
	{
		if (dragItem->flags() & dragItem->ItemIsMovable)
		{
			if (performDrag)
			{
				dragItem->setPos(mouseEvent->scenePos());
			}

			QSet<CItem*> oldHovers = m_acceptedHovers + m_rejectedHovers;

			QList<QGraphicsItem*> hoveredItems = dragItem->collidingItems();

			for (int i = 0; i < hoveredItems.size(); ++i)
			{
				// dont drop on disabled
				if (!hoveredItems.at(i)->isEnabled())
					continue;

				CItem* item = dynamic_cast<CItem*>(hoveredItems.at(i));
				if (item)
				{
					oldHovers.remove(item);
					if (m_acceptedHovers.contains(item) || m_rejectedHovers.contains(item))
						continue;

					ItemDragTestResult result = item->acceptDragFromItem(dragItem);

					if (result == Accepted)
					{
						m_acceptedHovers.insert(item);

						item->setItemStateFlag(IS_Drag_Accepted);
						item->resetItemStateFlag(IS_Drag_Rejected);
					}
					else if (result == Rejected)
					{
						m_rejectedHovers.insert(item);

						item->resetItemStateFlag(IS_Drag_Accepted);
						item->setItemStateFlag(IS_Drag_Rejected);
					}
				}
			}

			// deactivate left hovers
			for (CItem* item : oldHovers)
			{
				item->leaveDragFromItem(dragItem);

				m_acceptedHovers.remove(item);
				m_rejectedHovers.remove(item);

				item->resetItemStateFlag(IS_Drag_Accepted);
				item->resetItemStateFlag(IS_Drag_Rejected);
			}

			// inform the dragger
			CItem* draggedCItem = dynamic_cast<CItem*>(dragItem);
			if (draggedCItem)
			{
				draggedCItem->onDraggedOver(m_acceptedHovers, m_rejectedHovers);
			}

			// inform the scene
			onDragging(dragItem, m_acceptedHovers, m_rejectedHovers);
		}
	}
	else	// no drag, just hover
	{	
		QGraphicsItem *hoverItem = itemAt(mouseEvent->scenePos(), QTransform());

		// inform the scene
		onMoving(mouseEvent, hoverItem);
	}
}


void CEditorScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
	Super::mouseDoubleClickEvent(mouseEvent);

	if (mouseEvent->button() == Qt::LeftButton)
	{
		m_doubleClick = true;
	}
}


void CEditorScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
	QGraphicsItem* prevGrabber = m_draggedItem;

	Super::mouseReleaseEvent(mouseEvent);

	m_draggedItem = mouseGrabberItem();

	if (mouseEvent->button() == Qt::LeftButton)
	{
		if (m_dragInProgress)
		{
			finishDrag(mouseEvent, prevGrabber, false);
		}
		else if (m_leftClickPos == mouseEvent->scenePos())
		{
			QGraphicsItem *hoverItem = itemAt(mouseEvent->scenePos(), QTransform());

			if (m_doubleClick)
				onLeftDoubleClick(mouseEvent, hoverItem);
			else
				onLeftClick(mouseEvent, hoverItem);
		}
	}

	m_doubleClick = false;
	m_dragInProgress = false;

	// update curson on release
	QGraphicsItem *hoverItem = itemAt(mouseEvent->scenePos(), QTransform());
	updateMovedCursor(mouseEvent, hoverItem);
}


void CEditorScene::finishDrag(QGraphicsSceneMouseEvent* mouseEvent, QGraphicsItem* dragItem, bool dragCancelled)
{
	if (dragItem)
	{
		// deactivate left hovers
		for (CItem* item : m_acceptedHovers)
		{
			item->leaveDragFromItem(dragItem);

			item->resetItemStateFlag(IS_Drag_Accepted);
			item->resetItemStateFlag(IS_Drag_Rejected);
		}

		for (CItem* item : m_rejectedHovers)
		{
			item->leaveDragFromItem(dragItem);

			item->resetItemStateFlag(IS_Drag_Accepted);
			item->resetItemStateFlag(IS_Drag_Rejected);
		}

		// inform the dragger
		CItem* draggedItem = dynamic_cast<CItem*>(dragItem);
		if (draggedItem && !dragCancelled)
		{
			draggedItem->onDroppedOn(m_acceptedHovers, m_rejectedHovers);
		}

		// drag finish
		m_acceptedHovers.clear();
 		m_rejectedHovers.clear();

		if (!dragCancelled)
		{
			// snap after drop if dragItem still alive (can die after onDroppedOn??)
			if (items().contains(dragItem))
				onDropped(mouseEvent, dragItem);

			// update undo manager
			addUndoState();
		}
	}

	m_startDragItem = NULL;

	if (mouseEvent)
	{
		QGraphicsItem *hoverItem = itemAt(mouseEvent->scenePos(), QTransform());
		updateMovedCursor(mouseEvent, hoverItem);
	}
}


void CEditorScene::updateMovedCursor(QGraphicsSceneMouseEvent *mouseEvent, QGraphicsItem* hoverItem)
{
	if (hoverItem)
	{
		if (hoverItem->isEnabled() && (hoverItem->flags() & hoverItem->ItemIsMovable))
		{
			if (mouseEvent->buttons() == Qt::NoButton)
			{
				setSceneCursor(Qt::SizeAllCursor);
				return;
			}
		}
	}

	setSceneCursor(Qt::ArrowCursor);
}


void CEditorScene::onMoving(QGraphicsSceneMouseEvent *mouseEvent, QGraphicsItem* hoverItem)
{
	updateMovedCursor(mouseEvent, hoverItem);
}


void CEditorScene::onDragging(QGraphicsItem* /*dragItem*/, const QSet<CItem*>& acceptedItems, const QSet<CItem*>& rejectedItems)
{
	if (acceptedItems.size())
	{
		setSceneCursor(Qt::CrossCursor);
		return;
	}

	if (rejectedItems.size())
	{
		setSceneCursor(Qt::ForbiddenCursor);
		return;
	}

	setSceneCursor(Qt::SizeAllCursor);
}


void CEditorScene::onDropped(QGraphicsSceneMouseEvent* /*mouseEvent*/, QGraphicsItem* dragItem)
{
	if (m_gridSnap)
	{
		auto pos = getSnapped(dragItem->pos());
		auto d = pos - dragItem->pos();
		dragItem->setPos(pos);

		for (auto item : selectedItems())
		{
			if (item != dragItem)
				item->moveBy(d.x(), d.y());
		}
	}
}


void CEditorScene::onLeftClick(QGraphicsSceneMouseEvent* mouseEvent, QGraphicsItem* clickedItem)
{
	if (CItem *item = dynamic_cast<CItem*>(clickedItem))
	{
		item->onClick(mouseEvent);
	}
}


void CEditorScene::onLeftDoubleClick(QGraphicsSceneMouseEvent* /*mouseEvent*/, QGraphicsItem* clickedItem)
{
	// clicked on empty space?
	if (!clickedItem)
	{
		return;
	}

	// else check clicked item...
	CItem *item = dynamic_cast<CItem*>(clickedItem);
	if (!item) 
	{
		item = dynamic_cast<CItem*>(clickedItem->parentItem());	// if clicked on label
	}

	if (item)
	{
		onActionEditLabel(item);
	}
}


QPointF CEditorScene::getSnapped(const QPointF& pos) const
{
	if (m_gridSnap)
	{
		QPointF newPos(pos);

		if (newPos.x() < 0)
			newPos.setX(newPos.x() - m_gridSize / 2);
		else
			newPos.setX(newPos.x() + m_gridSize / 2);

		if (newPos.y() < 0)
			newPos.setY(newPos.y() - m_gridSize / 2);
		else
			newPos.setY(newPos.y() + m_gridSize / 2);

		newPos.setX((int)newPos.x() - (int)newPos.x() % m_gridSize);
		newPos.setY((int)newPos.y() - (int)newPos.y() % m_gridSize);

		return newPos;
	}
	else
		return pos;
}


// private

void CEditorScene::setSceneCursor(const QCursor& c)
{
	for (QGraphicsView* v : views())
	{
		v->setCursor(c);
	}
}


// keys

void CEditorScene::keyPressEvent(QKeyEvent *keyEvent)
{
	Super::keyPressEvent(keyEvent);
	if (keyEvent->isAccepted())
		return;

	if (keyEvent->key() == Qt::Key_Delete)
	{
		onActionDelete();

		keyEvent->accept();
		return;
	}

	if (keyEvent->key() == Qt::Key_A && keyEvent->modifiers() == Qt::ControlModifier)
	{
		onActionSelectAll();

		keyEvent->accept();
		return;
	}

	if (keyEvent->key() == Qt::Key_Right && keyEvent->modifiers() == Qt::ControlModifier)
	{
		moveSelectedItemsBy(1, 0);
		addUndoState();

		keyEvent->accept();
		return;
	}

	if (keyEvent->key() == Qt::Key_Left && keyEvent->modifiers() == Qt::ControlModifier)
	{
		moveSelectedItemsBy(-1, 0);
		addUndoState();

		keyEvent->accept();
		return;
	}

	if (keyEvent->key() == Qt::Key_Up && keyEvent->modifiers() == Qt::ControlModifier)
	{
		moveSelectedItemsBy(0, -1);
		addUndoState();

		keyEvent->accept();
		return;
	}

	if (keyEvent->key() == Qt::Key_Down && keyEvent->modifiers() == Qt::ControlModifier)
	{
		moveSelectedItemsBy(0, 1);
		addUndoState();

		keyEvent->accept();
		return;
	}
}


// general events

void CEditorScene::focusInEvent(QFocusEvent *focusEvent)
{
	Super::focusInEvent(focusEvent);

	static bool s_firstRun = true;
	if (s_firstRun) 
	{
		s_firstRun = false;

		// init scene
		addUndoState();
	}
}


// menu stuff 

void CEditorScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *contextMenuEvent)
{
	QMenu sceneMenu;

	m_menuTriggerItem = itemAt(contextMenuEvent->scenePos(), QTransform()); //Get the item at the position

	// check if item provides own menu
	auto menuItem = dynamic_cast<IContextMenuProvider*>(m_menuTriggerItem);
	if (menuItem)
	{
		if (menuItem->populateMenu(sceneMenu, selectedItems()))
		{
			sceneMenu.exec(contextMenuEvent->screenPos());
			return;
		}
	}

	// else custom menu
	if (populateMenu(sceneMenu, m_menuTriggerItem, selectedItems()))
	{
		sceneMenu.exec(contextMenuEvent->screenPos());
	}
}


bool CEditorScene::populateMenu(QMenu& menu, QGraphicsItem* item, const QList<QGraphicsItem*>& selectedItems)
{
	if (!item && selectedItems.isEmpty())
		return false;

	// add default actions
	QAction *deleteAction = menu.addAction(tr("Delete"), this, SLOT(onActionDelete()));
	deleteAction->setEnabled(createSelectedList(CDeletableItems()).size());

	return true;
}


void CEditorScene::onActionDelete()
{
	QList<QGraphicsItem*> itemList = createSelectedList(CDeletableItems());
	if (itemList.isEmpty())
		return;

	if (QMessageBox::question(NULL, tr("Delete Items"), tr("You are about to delete %1 item(s). Sure?").arg(itemList.size())) == QMessageBox::No)
		return;

	del();
}


void CEditorScene::onActionSelectAll()
{
	selectAll();
}


void CEditorScene::onActionEditLabel(CItem *item)
{
	bool ok = false;

	QString text = QInputDialog::getMultiLineText(NULL,
		tr("Item Label"), tr("New label text:"),
		item->getAttribute("label").toString(),
		&ok);

	if (ok)
	{
		item->setAttribute("label", text);

		addUndoState();
	}
}


// selections

void CEditorScene::selectAll() 
{
	QPainterPath path;
	path.addRect(sceneRect());
	setSelectionArea(path, QTransform());
}


void CEditorScene::deselectAll()
{
	QPainterPath path;
	setSelectionArea(path, QTransform());
}


void CEditorScene::beginSelection()
{
	blockSignals(true);
}


void CEditorScene::endSelection()
{
	blockSignals(false);

	Q_EMIT selectionChanged();
}


void CEditorScene::moveSelectedItemsBy(const QPointF& d)
{
	for (auto sceneItem : selectedItems())
	{
		sceneItem->moveBy(d.x(), d.y());
	}
}


// evaluators

QList<QGraphicsItem*> CEditorScene::createSelectedList(const CItemsEvaluator& eval) const
{
	QList<QGraphicsItem*> result;
	QList<QGraphicsItem*> itemList = getSelectedItems<QGraphicsItem>(true);

	for (int i = 0; i < itemList.size(); ++i)
	{
		if (eval.evaluate(*itemList.at(i)))
			result << itemList.at(i);
	}

	return result;
}


bool CDeletableItems::evaluate(const QGraphicsItem& item) const
{
	const CItem* citem = dynamic_cast<const CItem*>(&item);
	if (citem)
	{
		return citem->itemFlags() & IF_DeleteAllowed;
	}

	// can delete QGraphicsItem
	return true;
}
