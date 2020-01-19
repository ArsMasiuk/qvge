/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016 Ars L.Masiuk(ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CEditorScene.h"
#include "CEditorScene_p.h"
#include "CEditorSceneDefines.h"
#include "CEditorSceneActions.h"
#include "CItem.h"
#include "CControlPoint.h"
#include "CSimpleUndoManager.h"
#include "CDiffUndoManager.h"
#include "IContextMenuProvider.h"
#include "ISceneItemFactory.h"
#include "ISceneMenuController.h"

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


const quint64 version64 = 12;	// build
const char* versionId = "VersionId";


CEditorScene::CEditorScene(QObject *parent): QGraphicsScene(parent), 
    m_doubleClick(false),
	m_dragInProgress(false),
    m_startDragItem(NULL),
	m_infoStatus(-1),
    //m_undoManager(new CSimpleUndoManager(*this)),
	m_undoManager(new CDiffUndoManager(*this)),
	m_menuTriggerItem(NULL),
    m_draggedItem(NULL),
    m_needUpdateItems(true),
	m_isFontAntialiased(true),
	m_labelsEnabled(true),
	m_labelsUpdate(false),
	m_pimpl(new CEditorScene_p(this))
{
    m_gridSize = 25;
    m_gridEnabled = true;
    m_gridSnap = true;
    m_gridPen = QPen(Qt::gray, 0, Qt::DotLine);

	setBackgroundBrush(Qt::white);

    setSceneRect(-500, -500, 1000, 1000);

	// optimizations
    setItemIndexMethod(QGraphicsScene::NoIndex);
    
	setMinimumRenderSize(5);

	QPixmapCache::setCacheLimit(200000);

	// connections
	connect(this, &CEditorScene::selectionChanged, this, &CEditorScene::onSelectionChanged, Qt::DirectConnection);
	connect(this, &CEditorScene::focusItemChanged, this, &CEditorScene::onFocusItemChanged);
}


CEditorScene::~CEditorScene()
{
	disconnect();
	clear();

	delete m_pimpl;
}


void CEditorScene::reset()
{
	initialize();

	if (m_undoManager)
		m_undoManager->reset();

	setSceneRect(QRectF(-500,-500,1000,1000));
}


void CEditorScene::initialize()
{
	removeItems();

	m_classAttributes.clear();
	m_classAttributesVis.clear();
	m_classAttributesConstrains.clear();

	// default item attrs
	createClassAttribute(class_item, "label", "Label", "", ATTR_NODEFAULT | ATTR_FIXED, 0, true);
	createClassAttribute(class_item, "label.color", "Label Color", QColor(Qt::black));

	QFont labelFont;
	CAttribute labelFontAttr("label.font", "Label Font", labelFont, ATTR_FIXED);
	setClassAttribute(class_item, labelFontAttr);

	createClassAttribute(class_item, "id", "ID", "", ATTR_NODEFAULT | ATTR_FIXED, 0, true);


	// labels policy
	static CAttributeConstrainsEnum *labelsPolicy = new CAttributeConstrainsEnum();
	if (labelsPolicy->ids.isEmpty()) {
		labelsPolicy->names << tr("Auto") << tr("Always On") << tr("Always Off");
		labelsPolicy->ids << Auto << AlwaysOn << AlwaysOff;
	}

	createClassAttribute(class_scene, attr_labels_policy, "Labels Policy", Auto, ATTR_FIXED, labelsPolicy);
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

void CEditorScene::copyProperties(const CEditorScene& from)
{
	m_classAttributes = from.m_classAttributes;
	m_classToSuperIds = from.m_classToSuperIds;
	m_classAttributesVis = from.m_classAttributesVis;
}

CEditorScene* CEditorScene::clone()
{
	QByteArray buffer;
	QDataStream out(&buffer, QIODevice::WriteOnly);
	if (!storeTo(out, true))
		return nullptr;

	CEditorScene* tempScene = createScene();
	QDataStream in(buffer);
	if (tempScene->restoreFrom(in, true))
		return tempScene;

	delete tempScene;
	return nullptr;	
}


// undo-redo

void CEditorScene::undo()
{
	if (m_inProgress)
		return;

	m_inProgress = true;

	if (m_undoManager)
	{
		m_undoManager->undo();

		checkUndoState();

		onSceneChanged();
	}

	m_inProgress = false;
}


void CEditorScene::redo()
{
	if (m_inProgress)
		return;

	m_inProgress = true;

	if (m_undoManager)
	{
		m_undoManager->redo();

		checkUndoState();

		onSceneChanged();
	}

	m_inProgress = false;
}


void CEditorScene::addUndoState()
{
	if (m_inProgress)
		return;

	m_inProgress = true;

	onSceneChanged();

	// canvas size
    QRectF minRect(sceneRect());
    minRect |= itemsBoundingRect().adjusted(-10, -10, 10, 10);
    setSceneRect(minRect);

	// undo-redo
	if (m_undoManager)
	{
		m_undoManager->addState();

		checkUndoState();
	}

	m_inProgress = false;
}


void CEditorScene::revertUndoState()
{
	if (m_inProgress)
		return;

	m_inProgress = true;

	if (m_undoManager)
	{
		m_undoManager->revertState();

		checkUndoState();
	}

	onSceneChanged();

	m_inProgress = false;
}


void CEditorScene::setInitialState()
{
	m_inProgress = false;

	if (m_undoManager)
	{
		m_undoManager->reset();
	}

	addUndoState();
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
		for (const auto &attr : classAttrsIt.value())
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

bool CEditorScene::setItemFactory(CItem *factoryItem, const QByteArray& typeId)
{
	if (factoryItem)
	{
		// register class inheritance
		QByteArray classId = factoryItem->classId();
		QByteArray superClassId = factoryItem->superClassId();
		m_classToSuperIds[classId] = superClassId;

		QByteArray id = typeId.isEmpty() ? factoryItem->typeId() : typeId;
		m_itemFactories[id] = factoryItem;
		return true;
	}

	return false;
}


CItem* CEditorScene::getItemFactory(const QByteArray& typeId) const 
{
	return m_itemFactories.contains(typeId) ? m_itemFactories[typeId] : NULL;
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

CAttribute& CEditorScene::createClassAttribute(
	const QByteArray& classId,
	const QByteArray& attrId, 
	const QString& attrName, 
	const QVariant& defaultValue,
	int attrFlags,
	CAttributeConstrains* constrains,
	bool vis) 
{
	if (m_classAttributes[classId].contains(attrId))
	{
		// just update the value
		m_classAttributes[classId][attrId].defaultValue = defaultValue;
	}
	else
	{
		m_classAttributes[classId][attrId] = CAttribute(attrId, attrName, defaultValue, attrFlags);

		setClassAttributeVisible(classId, attrId, vis);

		if (constrains)
			setClassAttributeConstrains(classId, attrId, constrains);
	}

	return m_classAttributes[classId][attrId];
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
	if (m_classAttributes[classId].contains(attrId))
	{
		// just update the value
		m_classAttributes[classId][attrId].defaultValue = defaultValue;
		needUpdate();
		return;
	}

	// clone from super if not found
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
		return;
	}

	// else create new attribute with name = id
	CAttribute attr(attrId, attrId, defaultValue);
	m_classAttributes[classId][attrId] = attr;
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
	if (vis == m_classAttributesVis[classId].contains(attrId))
		return;

	if (vis)
		m_classAttributesVis[classId].insert(attrId);
	else
		m_classAttributesVis[classId].remove(attrId);

	// set label update flag
	m_labelsUpdate = true;

	// schedule update
	invalidate();
}


bool CEditorScene::isClassAttributeVisible(const QByteArray& classId, const QByteArray& attrId) const
{
	return m_classAttributesVis[classId].contains(attrId);
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


void CEditorScene::setVisibleClassAttributes(const QByteArray& classId, const QSet<QByteArray>& vis)
{
	m_classAttributesVis[classId] = vis;

	// set label update flag
	m_labelsUpdate = true;

	// schedule update
	invalidate();
}


const CAttribute CEditorScene::getClassAttribute(const QByteArray& classId, const QByteArray& attrId, bool inherited) const 
{
	CAttribute attr = m_classAttributes[classId][attrId];
	if (attr.id.size() || !inherited)
		return attr;

	// else inherited
	QByteArray superId = getSuperClassId(classId);
	if (superId.isEmpty())
		// fail
		return CAttribute();

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
        return nullptr;
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

	beginSelection();

	for (QGraphicsItem* item : itemList)
	{
		if (items().contains(item))
			delete item;
	}

	endSelection();

	addUndoState();
}


void CEditorScene::copy()
{
	// store selected items only
	QMap<CItem*, quint64> sortedMap;

	QList<QGraphicsItem*> allItems = getCopyPasteItems();

	for (QGraphicsItem* item : allItems)
	{
		if (CItem* citem = dynamic_cast<CItem*>(item))
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

	// paste it to a temp scene & render into mime image
	CEditorScene* tempScene = createScene();
	tempScene->copyProperties(*this);
	tempScene->enableGrid(false);
	tempScene->paste();
	tempScene->deselectAll();
	tempScene->crop();

	QImage image(tempScene->sceneRect().size().toSize(), QImage::Format_ARGB32);
	image.fill(Qt::white);

	QPainter painter(&image);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);
	tempScene->render(&painter);
	painter.end();

	mimeData->setImageData(QVariant(image));
	QApplication::clipboard()->setMimeData(mimeData);

	delete tempScene;
}


void CEditorScene::paste()
{
	if (!m_pastePos.isNull())
	{
		pasteAt(m_pastePos);
		return;
	}

	auto *view = getCurrentView();
	if (view)
	{
		if (view->underMouse())
		{
			auto p = view->mapFromGlobal(QCursor::pos());
			pasteAt(view->mapToScene(p));
		}
		else
		{
			QRectF vp = view->mapToScene(view->viewport()->geometry()).boundingRect();
			auto center = vp.center();
			pasteAt(center);
		}
	}
	else
	{
		pasteAt(QPointF());
	}
}


void CEditorScene::pasteAt(const QPointF &anchor)
{
	const QClipboard *clipboard = QApplication::clipboard();
	const QMimeData *mimeData = clipboard->mimeData();
    if (!mimeData)
		return;
	if (!mimeData->hasFormat("qvge/selection"))
		return;

	deselectAll();

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
		ids[item->getId() + item->typeId()]++;

	// shift if not in-place
	auto selItems = selectedItems();

	if (!anchor.isNull())
	{
		QRectF r = CUtils::getBoundingRect(selItems);
		QPointF d = anchor - r.center();
		moveSelectedItemsBy(d);
	}

	// rename pasted items
	for (auto sceneItem : selItems)
	{
		CItem* item = dynamic_cast<CItem*>(sceneItem);
		if (item)
		{
			QString id = item->getId();
			QString typeId = item->typeId();
			if (ids[id + typeId] > 1)
			{
				int counter = 1;
				QString newId = id;

				while (ids.contains(newId + typeId))
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


QList<QGraphicsItem*> CEditorScene::getCopyPasteItems() const
{
	return selectedItems();
}


QList<CItem*> CEditorScene::cloneSelectedItems()
{
	QList<CItem*> clonedList;

	// store selected items only
	QMap<CItem*, quint64> sortedMap;

	QList<QGraphicsItem*> allItems = getCopyPasteItems();

	for (QGraphicsItem* item : allItems)
	{
		if (CItem* citem = dynamic_cast<CItem*>(item))
		{
			sortedMap[citem] = quint64(citem);
		}
	}

	if (sortedMap.isEmpty())
		return clonedList;

	// write version and items
	QByteArray buffer;
	{
		QDataStream out(&buffer, QIODevice::WriteOnly);

		for (CItem* citem : sortedMap.keys())
		{
			out << citem->typeId() << quint64(citem);

			citem->storeTo(out, version64);
		}
	}

	// read cloned items from the buffer
	deselectAll();

	CItem::CItemLinkMap idToItem;
	QList<CItem*> deathList;

	{
		QDataStream in(buffer);

		while (!in.atEnd())
		{
			QByteArray typeId; in >> typeId;
			quint64 ptrId; in >> ptrId;

			CItem* item = createItemOfType(typeId);
			if (item)
			{
				if (item->restoreFrom(in, version64))
				{
					idToItem[ptrId] = item;
				}
				else
					deathList << item;
			}
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

			clonedList << item;
		}
		else
			deathList << item;
	}

	// cleanup
	qDeleteAll(deathList);

	if (clonedList.isEmpty())
		return clonedList;

	// shift & rename pasted items which were not removed
	QMap<QString, int> ids;
	auto allCItems = getItems<CItem>();
	for (auto item : allCItems)
		ids[item->getId() + item->typeId()]++;

	// shift if not in-place
	auto selItems = selectedItems();

	// rename pasted items
	for (auto sceneItem : selItems)
	{
		CItem* item = dynamic_cast<CItem*>(sceneItem);
		if (item)
		{
			QString id = item->getId();
			QString typeId = item->typeId();
			if (ids[id + typeId] > 1)
			{
				int counter = 1;
				QString newId = id;

				while (ids.contains(newId + typeId))
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

	return clonedList;
}


// crop

void CEditorScene::crop()
{
	QRectF itemsRect = itemsBoundingRect().adjusted(-20, -20, 20, 20);
	if (itemsRect == sceneRect())
		return;

	// update scene rect
	setSceneRect(itemsRect);

	addUndoState();
}


// transform

QList<QGraphicsItem*> CEditorScene::getTransformableItems() const
{
	return selectedItems();
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


void CEditorScene::onSelectionChanged()
{
	int selectionCount = selectedItems().size();
	actions()->cutAction->setEnabled(selectionCount > 0);
	actions()->copyAction->setEnabled(selectionCount > 0);
	actions()->delAction->setEnabled(selectionCount > 0);

	if (m_editController)
	{
		m_editController->onSelectionChanged(*this);
	}
}


void CEditorScene::onFocusItemChanged(QGraphicsItem *newFocusItem, QGraphicsItem *oldFocusItem, Qt::FocusReason reason)
{
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
	if (m_gridSize <= 0 || !m_gridEnabled)
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

	// draw transformer etc
	if (m_editController)
		m_editController->draw(*this, painter, r);
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


CEditorScene::LabelsPolicy CEditorScene::getLabelsPolicy() const
{
	int labelPolicy = getClassAttribute(class_scene, attr_labels_policy, false).defaultValue.toInt();
	return (CEditorScene::LabelsPolicy) labelPolicy;
}


void CEditorScene::setLabelsPolicy(CEditorScene::LabelsPolicy v)
{
	setClassAttribute(class_scene, attr_labels_policy, (QVariant) v);
}


void CEditorScene::layoutItemLabels()
{
	// reset region
	m_usedLabelsRegion = QPainterPath();

	QList<CItem*> allItems = getItems<CItem>();

	// get labeling policy
	auto labelPolicy = getLabelsPolicy();

	// hide all if disabled
	if (!m_labelsEnabled || labelPolicy == AlwaysOff)
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

		if (labelPolicy == AlwaysOn)
			citem->showLabel(true);
		else
		{
			QRectF labelRect = citem->getSceneLabelRect();
			QRectF reducedRect(labelRect.topLeft() / 10, labelRect.size() / 10);

			citem->showLabel(checkLabelRegion(reducedRect));
		}
	}

	//qDebug() << "layout labels: " << tm.elapsed();
}


void CEditorScene::needUpdate()
{
	m_labelsUpdate = true;
	m_needUpdateItems = true;

	update();
}


// mousing

void CEditorScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
	if (m_editController)
	{
		if (m_editItem)
		{
			m_pimpl->m_labelEditor.finishEdit();
		}

		if (m_editController->onMousePressed(*this, mouseEvent))
		{
			mouseEvent->setAccepted(true);
			return;
		}
		else
			mouseEvent->setAccepted(false);
	}

	if (m_editItem)
	{
		// call super
		Super::mousePressEvent(mouseEvent);
		return;
	}

	// check RMB
	if (mouseEvent->button() == Qt::RightButton)
	{	
		onRightButtonPressed(mouseEvent);
	}

	// check LMB
	if (mouseEvent->button() == Qt::LeftButton)
	{
		onLeftButtonPressed(mouseEvent);
	}

	// call super is allowed
	if (!mouseEvent->isAccepted())
	{
		Super::mousePressEvent(mouseEvent);
	}
}


void CEditorScene::selectUnderMouse(QGraphicsSceneMouseEvent *mouseEvent)
{
	auto item = getItemAt(mouseEvent->scenePos());
	if (item)
	{
		if (!item->isSelected())
		{
			deselectAll();
			item->setSelected(true);
		}
	}
}


void CEditorScene::onLeftButtonPressed(QGraphicsSceneMouseEvent *mouseEvent)
{
	m_draggedItem = NULL;
	m_dragInProgress = false;

	m_leftClickPos = mouseEvent->scenePos();
}


void CEditorScene::onRightButtonPressed(QGraphicsSceneMouseEvent *mouseEvent)
{
	auto item = getItemAt(mouseEvent->scenePos());
	if (!item)
		return;

	// bypass control points
	if (dynamic_cast<CControlPoint*>(item))
		return;

	// do not deselect selected items by RMB, but select exclusive if not selected
	if (!item->isSelected())
	{
		deselectAll();
		item->setSelected(true);
	}

	mouseEvent->accept();
	return;
}


void CEditorScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
	if (m_editController)
	{
		if (m_editController->onMouseMove(*this, mouseEvent))
		{
			mouseEvent->setAccepted(true);
			return;
		}
		else
			mouseEvent->setAccepted(false);
	}

	if (m_editItem)
	{
		// call super
		Super::mouseMoveEvent(mouseEvent);
		return;
	}

	// store the last position
	m_mousePos = mouseEvent->scenePos();

	bool isDragging = (mouseEvent->buttons() & Qt::LeftButton);


	if (m_doubleClick)
	{
		m_doubleClick = false;

		// moved after double click?
		if (isDragging && !onDoubleClickDrag(mouseEvent, m_leftClickPos))
		{
			return;
		}
	}

	// no double click and no drag
	if (m_startDragItem == NULL)
	{
		// moved after single click?
		if (isDragging && onClickDrag(mouseEvent, m_leftClickPos))
		{
			moveDrag(mouseEvent, m_startDragItem, true);
			return;
		}

		// call super
		Super::mouseMoveEvent(mouseEvent);

		m_draggedItem = mouseGrabberItem();

		moveDrag(mouseEvent, m_draggedItem, false);

		updateCursorState();

		return;
	}

	// custom dragging
	moveDrag(mouseEvent, m_startDragItem, isDragging);
}


void CEditorScene::startDrag(QGraphicsItem* dragItem)
{
	m_startDragItem = dragItem;
	m_dragInProgress = true;
	m_lastDragPos = m_leftClickPos;
}


void CEditorScene::processDrag(QGraphicsSceneMouseEvent *mouseEvent, QGraphicsItem* dragItem)
{
	if (m_editController)
	{
		m_editController->onDragItem(*this, mouseEvent, dragItem);
	}

	QPointF d = mouseEvent->scenePos() - mouseEvent->lastScenePos();	// delta pos

	if (m_startDragItem)
	{
		auto keys = qApp->queryKeyboardModifiers();

		if (keys & Qt::ShiftModifier)
		{
			auto hpos = mouseEvent->scenePos();
			auto delta = hpos - m_leftClickPos;
			if (qAbs(delta.x()) > qAbs(delta.y()))
				hpos.setY(m_leftClickPos.y());
			else
				hpos.setX(m_leftClickPos.x());

			d = hpos - m_lastDragPos;
			m_lastDragPos = hpos;
		}
		else
		{
			d = mouseEvent->scenePos() - m_lastDragPos;	// delta pos
			m_lastDragPos = mouseEvent->scenePos();
		}
	}

	// if control point: move only it
	if (auto ctrl = dynamic_cast<CControlPoint*>(m_startDragItem))
	{
		//deselectAll();
		ctrl->moveBy(d.x(), d.y());
		return;
	}

	// fallback
	moveSelectedItemsBy(d);
}


void CEditorScene::moveDrag(QGraphicsSceneMouseEvent *mouseEvent, QGraphicsItem* dragItem, bool performDrag)
{
	if (dragItem)
	{
		m_dragInProgress = true;

		if (dragItem->flags() & dragItem->ItemIsMovable)
		{
			if (performDrag)
			{
				processDrag(mouseEvent, dragItem);		
			}

			QSet<IInteractive*> oldHovers = m_acceptedHovers + m_rejectedHovers;

			QList<QGraphicsItem*> hoveredItems = dragItem->collidingItems();

			for (auto hoverItem: hoveredItems)
			{
				// filter out children
				if (hoverItem->parentItem() == dragItem)
					continue;

				// dont drop on disabled
				if (!hoverItem->isEnabled())
					continue;

				IInteractive* item = dynamic_cast<IInteractive*>(hoverItem);
				CItem* citem = dynamic_cast<CItem*>(hoverItem);

				if (item)
				{
					oldHovers.remove(item);
					if (m_acceptedHovers.contains(item) || m_rejectedHovers.contains(item))
						continue;

					ItemDragTestResult result = item->acceptDragFromItem(dragItem);

					if (result == Accepted)
					{
						m_acceptedHovers.insert(item);

						if (citem)
						{
							citem->setItemStateFlag(IS_Drag_Accepted);
							citem->resetItemStateFlag(IS_Drag_Rejected);
						}
					}
					else if (result == Rejected)
					{
						m_rejectedHovers.insert(item);

						if (citem)
						{
							citem->resetItemStateFlag(IS_Drag_Accepted);
							citem->setItemStateFlag(IS_Drag_Rejected);
						}
					}

					hoverItem->update();
				}
			}

			// deactivate left hovers
			for (IInteractive* item : oldHovers)
			{
				item->leaveDragFromItem(dragItem);

				m_acceptedHovers.remove(item);
				m_rejectedHovers.remove(item);

				CItem* citem = dynamic_cast<CItem*>(item);
				if (citem)
				{
					citem->resetItemStateFlag(IS_Drag_Accepted);
					citem->resetItemStateFlag(IS_Drag_Rejected);
				}

				if (auto hoverItem = dynamic_cast<QGraphicsItem*>(item))
					hoverItem->update();
			}

			// inform the dragger
			IInteractive* draggedItem = dynamic_cast<IInteractive*>(dragItem);
			if (draggedItem)
			{
				draggedItem->onDraggedOver(m_acceptedHovers, m_rejectedHovers);
			}

			// inform the scene
			onDragging(dragItem, m_acceptedHovers, m_rejectedHovers);
		}
	}
	else	// no drag, just hover
	{	
		QGraphicsItem *hoverItem = getItemAt(mouseEvent->scenePos());

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
	if (m_editController)
	{
		if (m_editController->onMouseReleased(*this, mouseEvent))
		{
			mouseEvent->setAccepted(true);
			return;
		}
		else
			mouseEvent->setAccepted(false);
	}

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
			QGraphicsItem *hoverItem = getItemAt(mouseEvent->scenePos());

			if (m_doubleClick)
				onLeftDoubleClick(mouseEvent, hoverItem);
			else
				onLeftClick(mouseEvent, hoverItem);
		}
	}

	m_doubleClick = false;
	m_dragInProgress = false;
}


void CEditorScene::finishDrag(QGraphicsSceneMouseEvent* mouseEvent, QGraphicsItem* dragItem, bool dragCancelled)
{
	if (dragItem)
	{
		// deactivate left hovers
		for (IInteractive* item : m_acceptedHovers)
		{
			item->leaveDragFromItem(dragItem);

			if (auto *citem = dynamic_cast<CItem*>(item))
			{
				citem->resetItemStateFlag(IS_Drag_Accepted);
				citem->resetItemStateFlag(IS_Drag_Rejected);
			}
		}

		for (IInteractive* item : m_rejectedHovers)
		{
			item->leaveDragFromItem(dragItem);

			if (auto *citem = dynamic_cast<CItem*>(item))
			{
				citem->resetItemStateFlag(IS_Drag_Accepted);
				citem->resetItemStateFlag(IS_Drag_Rejected);
			}
		}

		// inform the dragger
		IInteractive* draggedItem = dynamic_cast<IInteractive*>(dragItem);
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
			{
				onDropped(mouseEvent, dragItem);
			}

			// update undo manager
			addUndoState();
		}
	}

	m_startDragItem = NULL;
	m_dragInProgress = false;
}


void CEditorScene::onMoving(QGraphicsSceneMouseEvent *mouseEvent, QGraphicsItem* hoverItem)
{
	updateCursorState();

	if (hoverItem)
		setInfoStatus(SIS_Hover);
	else
		setInfoStatus(SIS_Select);
}


void CEditorScene::onDragging(QGraphicsItem* /*dragItem*/, const QSet<IInteractive*>& /*acceptedItems*/, const QSet<IInteractive*>& /*rejectedItems*/)
{
	updateCursorState();

	setInfoStatus(SIS_Drag);
}


void CEditorScene::onDropped(QGraphicsSceneMouseEvent* mouseEvent, QGraphicsItem* dragItem)
{
	auto keys = mouseEvent->modifiers();
	bool isSnap = (keys & Qt::AltModifier) ? !m_gridSnap : m_gridSnap;
	if (isSnap)
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


void CEditorScene::onLeftDoubleClick(QGraphicsSceneMouseEvent* mouseEvent, QGraphicsItem* clickedItem)
{
	// clicked on empty space?
	//if (!clickedItem)
	//	return;

	// else check clicked item...
	if (CItem *item = dynamic_cast<CItem*>(clickedItem))
	{
		onActionEditLabel(item);
		//item->onDoubleClick(mouseEvent);
	}

	// emit signals
	Q_EMIT sceneDoubleClicked(mouseEvent, clickedItem);
}


bool CEditorScene::onClickDrag(QGraphicsSceneMouseEvent *mouseEvent, const QPointF &clickPos)
{
	if (QGraphicsItem* item = getItemAt(clickPos))
	{
		if (!item->isEnabled())
			return false;

        if (!(item->flags() & item->ItemIsMovable))
			return false;

		if (CItem *citem = dynamic_cast<CItem*>(item))
		{
			// clone?
			if (mouseEvent->modifiers() == Qt::ControlModifier)
			{
				// select under mouse if clone
				selectUnderMouse(mouseEvent);

				// clone selection
				QList<CItem*> clonedList = cloneSelectedItems();
				if (clonedList.isEmpty())
					return false;

				selectItems(clonedList);

				// start drag via 1st item
				startDrag(clonedList.first()->getSceneItem());

				return true;
			}

			//qDebug() << clickPos << citem;

			// else handle by item
			if (!citem->onClickDrag(mouseEvent, clickPos))
				return false;
		}

		// else start drag of item
		startDrag(item);
		return true;
	}

	// nothing to do
	return false;
}


bool CEditorScene::onDoubleClickDrag(QGraphicsSceneMouseEvent *mouseEvent, const QPointF &clickPos)
{
	// handle by object under mouse
	QGraphicsItem* item = getItemAt(clickPos);
	if (item)
	{
		if (!item->isEnabled())
			return false;

        if (!(item->flags() & item->ItemIsMovable))
			return false;

		CItem *citem = dynamic_cast<CItem*>(item);
		if (citem)
			return citem->onDoubleClickDrag(mouseEvent, clickPos);
	}

	// nothing to do
	return false;
}


// scene

void CEditorScene::setInfoStatus(int status)
{
	if (m_infoStatus != status)
	{
		m_infoStatus = status;

		Q_EMIT infoStatusChanged(status);
	}
}


void CEditorScene::updateCursorState()
{
	auto keys = qApp->queryKeyboardModifiers();
	auto buttons = qApp->mouseButtons();
	QGraphicsItem *hoverItem = getItemAt(m_mousePos);
	doUpdateCursorState(keys, buttons, hoverItem);
}


bool CEditorScene::doUpdateCursorState(Qt::KeyboardModifiers keys, Qt::MouseButtons buttons, QGraphicsItem *hoverItem)
{
	// drag?
	if (m_dragInProgress)
	{
		if (m_acceptedHovers.size())
		{
			setSceneCursor(Qt::CrossCursor);
			return true;
		}

		if (m_rejectedHovers.size())
		{
			setSceneCursor(Qt::ForbiddenCursor);
			return true;
		}

		// clone?
		//if (keys == Qt::ControlModifier)
		//{
		//	setSceneCursor(Qt::DragCopyCursor);
		//	return;
		//}

		setSceneCursor(Qt::SizeAllCursor);
		return true;
	}

	// can drag a hover item?
	if (hoverItem)
	{
		if (hoverItem->isEnabled() && (hoverItem->flags() & hoverItem->ItemIsMovable))
		{
			// clone?
			if (keys == Qt::ControlModifier)
			{
				setSceneCursor(Qt::DragCopyCursor);
				return true;
			}

			if (buttons == Qt::NoButton)
			{
				setSceneCursor(Qt::SizeAllCursor);
				return true;
			}
		}
	}

	// default
	setSceneCursor(Qt::ArrowCursor);
	return false;
}


QPointF CEditorScene::getSnapped(const QPointF& pos) const
{
	auto keys = qApp->queryKeyboardModifiers();
	bool isSnap = (keys & Qt::AltModifier) ? !gridSnapEnabled() : gridSnapEnabled();
	if (isSnap)
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


QGraphicsItem* CEditorScene::getItemAt(const QPointF& pos) const
{
	QGraphicsItem *hoverItem = itemAt(pos, QTransform());
	
	// if label: return parent instead
    if (dynamic_cast<QGraphicsSimpleTextItem*>(hoverItem))
	{
		return hoverItem->parentItem();
	}
	else
		return hoverItem;
}


QGraphicsView* CEditorScene::getCurrentView()
{
	for (auto view: views())
	{
		if (view->underMouse() || view->hasFocus()) 
			return view;
	}

	if (views().count() == 1)
		return views().first();

	return nullptr;
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

void CEditorScene::keyReleaseEvent(QKeyEvent *keyEvent)
{
	Super::keyReleaseEvent(keyEvent);

	updateCursorState();
}


void CEditorScene::keyPressEvent(QKeyEvent *keyEvent)
{
	bool isCtrl = (keyEvent->modifiers() == Qt::ControlModifier);
	bool isAlt = (keyEvent->modifiers() == Qt::AltModifier);
	bool isShift = (keyEvent->modifiers() == Qt::ShiftModifier);

	Super::keyPressEvent(keyEvent);

	updateCursorState();

	if (isAlt || keyEvent->isAccepted())
		return;


	if (keyEvent->key() == Qt::Key_Delete)
	{
		onActionDelete();

		keyEvent->accept();
		return;
	}


	if (keyEvent->key() == Qt::Key_A && isCtrl)
	{
		onActionSelectAll();

		keyEvent->accept();
		return;
	}


	// no modifier moves by 1 pixel, shift moves by grid size
	int moveStep = isShift ? m_gridSize : 1;

	if (keyEvent->key() == Qt::Key_Right)
	{
		moveSelectedItemsBy(moveStep, 0);
		addUndoState();

		keyEvent->accept();
		return;
	}

	if (keyEvent->key() == Qt::Key_Left)
	{
		moveSelectedItemsBy(-moveStep, 0);
		addUndoState();

		keyEvent->accept();
		return;
	}

	if (keyEvent->key() == Qt::Key_Up)
	{
		moveSelectedItemsBy(0, -moveStep);
		addUndoState();

		keyEvent->accept();
		return;
	}

	if (keyEvent->key() == Qt::Key_Down)
	{
		moveSelectedItemsBy(0, moveStep);
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
		//addUndoState();
	}
}


// menu stuff 

void CEditorScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *contextMenuEvent)
{
	if (m_skipMenuEvent)
	{
		m_skipMenuEvent = false;
		return;
	}

	m_menuTriggerItem = getItemAt(contextMenuEvent->scenePos()); //Get the item at the position

	// check if item provides own menu
	if (auto menuItem = dynamic_cast<IContextMenuProvider*>(m_menuTriggerItem))
	{
		QMenu sceneMenu;
		if (menuItem->populateMenu(sceneMenu, selectedItems()))
		{
			sceneMenu.exec(contextMenuEvent->screenPos());
			return;
		}
	}

	// else custom menu
	if (m_menuController)
		m_menuController->exec(this, m_menuTriggerItem, contextMenuEvent);
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


// label edit

void CEditorScene::onActionEditLabel(CItem *item)
{
	setInfoStatus(SIS_Edit_Label);
	setSceneCursor(Qt::IBeamCursor);

	m_pimpl->m_labelEditor.startEdit(item);

	m_editItem = item;
}


void CEditorScene::onItemEditingFinished(CItem * /*item*/, bool /*cancelled*/)
{
	m_editItem = nullptr;
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


void CEditorScene::selectItems(const QList<CItem*>& items, bool exclusive)
{
	beginSelection();

	if (exclusive)
		deselectAll();

	for (auto item : items)
		item->getSceneItem()->setSelected(true);

	endSelection();
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


void CEditorScene::ensureSelectionVisible()
{
	auto items = selectedItems();

	QRectF r;
	for (const auto item : items)
		r |= item->sceneBoundingRect();

	if (items.count())
		items.first()->ensureVisible(r);
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


// actions

CEditorSceneActions* CEditorScene::actions()
{
	return dynamic_cast<CEditorSceneActions*>(getActions());
}


QObject* CEditorScene::getActions()
{
    if (m_actions == nullptr)
		m_actions = createActions();

	return m_actions;
}


QObject* CEditorScene::createActions()
{
	return new CEditorSceneActions(this);
}


// edit extenders

void CEditorScene::startTransform(bool on)
{
	if (on)
		setSceneEditController(&m_pimpl->m_transformRect);
	else
		setSceneEditController(nullptr);
}


void CEditorScene::setSceneEditController(ISceneEditController *controller)
{
	if (m_editController != controller)
	{
		if (m_editController)
			m_editController->onDeactivated(*this);

		m_editController = controller;

		if (m_editController)
			m_editController->onActivated(*this);
	}
}
