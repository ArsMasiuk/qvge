#include "CNodeEditorScene.h"
#include "CNode.h"
#include "CConnection.h"
#include "CDirectConnection.h"
#include "CControlPoint.h"

#include <QGraphicsSceneMouseEvent>
#include <QColorDialog> 
#include <QKeyEvent>
#include <QApplication>
#include <QDebug>
#include <QElapsedTimer>


CNodeEditorScene::CNodeEditorScene(QObject *parent) : Super(parent),
	m_startNode(NULL),
	m_endNode(NULL),
	m_connection(NULL),
	m_realStart(false),
	m_activeConnectionFactory(NULL),
	m_state(IS_None)
{
	// default factories
	registerItemFactory<CDirectConnection>();
	registerItemFactory<CNode>();

	// go
	initialize();

    // connections
    connect(this, SIGNAL(selectionChanged()), this, SLOT(onSceneOrSelectionChanged()));
    //connect(this, SIGNAL(sceneChanged()), this, SLOT(onSceneOrSelectionChanged()));
}


// reimp

void CNodeEditorScene::initialize()
{
	Super::initialize();

	// default node attr
    CAttribute nodeAttr("color", "Color", QColor(Qt::magenta));
	setClassAttribute("node", nodeAttr);

    CAttribute shapeAttr("shape", "Shape", "disc");
	setClassAttribute("node", shapeAttr);

	//CAttribute sizeAttr("size", "Size", 11.0);
	//setClassAttribute("node", sizeAttr);
	//setClassAttributeConstrains("node", "size", new CDoubleConstrains(0.1, 1000.0));
	//createClassAttribute("node", "size", "Size", 11.0, new CDoubleConstrains(0.1, 1000.0));
	createClassAttribute("node", "size", "Size", QSizeF(11.0, 11.0));

    CAttribute posAttr("pos", "Position", QPointF());
	posAttr.noDefault = true;
	setClassAttribute("node", posAttr);

	// default edge attr
    CAttribute edgeAttr("color", "Color", QColor(Qt::gray));
	setClassAttribute("edge", edgeAttr);

	CAttribute directionAttr("direction", "Direction", "directed");
	setClassAttribute("edge", directionAttr);

	CAttribute weightAttr("weight", "Weight", 1.0);
	setClassAttribute("edge", weightAttr);

    CAttribute styleAttr("style", "Style", "solid");
    setClassAttribute("edge", styleAttr);


	CAttributeConstrainsList *edgeDirections = new CAttributeConstrainsList();
	edgeDirections->names << "Directed (one end)" << "Mutual (both ends)" << "None (no ends)";
	edgeDirections->ids << "directed" << "mutual" << "undirected";
	edgeDirections->icons << QIcon(":/Icons/Edge-Directed") << QIcon(":/Icons/Edge-Mutual") << QIcon(":/Icons/Edge-Undirected");
	setClassAttributeConstrains("edge", "direction", edgeDirections);

	CAttributeConstrainsList *edgeStyles = new CAttributeConstrainsList();
	edgeStyles->names << "Solid" << "Dots" << "Dashes";
	edgeStyles->ids << "solid" << "dotted" << "dashed";
	setClassAttributeConstrains("edge", "style", edgeStyles);

	CAttributeConstrainsList *nodeShapes = new CAttributeConstrainsList();
	nodeShapes->names << "Dics" << "Square" << "Triangle (up)" << "Triangle (down)" << "Diamond";
	nodeShapes->ids << "disc" << "square" << "triangle" << "triangle2" << "diamond";
	nodeShapes->icons << QIcon(":/Icons/Node-Disc") << QIcon(":/Icons/Node-Square") << QIcon(":/Icons/Node-Triangle") << QIcon(":/Icons/Node-Triangle-Down") << QIcon(":/Icons/Node-Diamond");
	setClassAttributeConstrains("node", "shape", nodeShapes);
}


void CNodeEditorScene::initializeOnce()
{
	Super::initializeOnce();
}


// nodes creation

bool CNodeEditorScene::startNewConnection(const QPointF& pos)
{
	QGraphicsItem* item = itemAt(pos, QTransform());
	if (item)
	{
		if (!item->isEnabled())
			return false;

		CNode *node = dynamic_cast<CNode*>(item);
		if (!node)
			return false;

		if (!node->allowStartConnection())
			return false;

		m_realStart = false;
		m_startNode = node;
	}
	else
	{
		m_realStart = true;
		m_startNode = createNewNode();
		item = m_startNode;
		addItem(item);
		item->setPos(getSnapped(pos));
	}

	m_endNode = dynamic_cast<CNode*>(m_startNode->clone());
	Super::startDrag(m_endNode);

	m_connection = createNewConnection();
	addItem(m_connection);
	m_connection->setFirstNode(m_startNode);
	m_connection->setLastNode(m_endNode);

	m_state = IS_Creating;

    // auto select created items
    m_startNode->setSelected(false);
    //m_connection->setSelected(true);
    m_endNode->setSelected(true);

	return true;
}


void CNodeEditorScene::cancel(const QPointF& /*pos*/)
{
	// cancel current drag operation
	Super::finishDrag(NULL, m_startDragItem, true);

	// if no creating state: return
	if (m_state != IS_Creating)
	{
		m_state = IS_None;
		return;
	}

	m_state = IS_None;

	// kill connector
	m_connection->setFirstNode(NULL);
	m_connection->setLastNode(NULL);
	delete m_connection;
	m_connection = NULL;

	// kill end
	delete m_endNode;
	m_endNode = NULL;

	// kill start if real
	if (m_realStart)
		delete m_startNode;

	m_startNode = NULL;
	m_realStart = false;
}


CNode* CNodeEditorScene::createNewNode() const
{
	if (getActiveItemFactory()) 
	{
		CItem* nodeItem = getActiveItemFactory()->create();
		if (nodeItem) 
		{
			if (CNode* node = dynamic_cast<CNode*>(nodeItem))
				return node;

			delete nodeItem;
		}
	}

	// here default
	return new CNode();
}


CConnection* CNodeEditorScene::createNewConnection() const
{
	if (m_activeConnectionFactory)
	{
		CItem* connItem = m_activeConnectionFactory->create();
		if (connItem) 
		{
			if (CConnection* conn = dynamic_cast<CConnection*>(connItem))
				return conn;

			delete connItem;
		}
	}

	// here default
	return new CDirectConnection();
}


CConnection* CNodeEditorScene::activateConnectionFactory(const QByteArray& factoryId)
{
	if (factoryId.isEmpty() || !m_itemFactories.contains(factoryId))
	{
		m_activeConnectionFactory = NULL;
	}
	else
	{
		m_activeConnectionFactory = dynamic_cast<CConnection*>(m_itemFactories[factoryId]);
	}

	return NULL;
}


// events

void CNodeEditorScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
	setSceneCursor(Qt::SizeAllCursor);

	Super::mouseDoubleClickEvent(mouseEvent);
}


void CNodeEditorScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
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

		// edges drag
		if (isDragging && mouseGrabberItem())
		{
			QPointF d = mouseEvent->scenePos() - mouseEvent->lastScenePos();	// delta pos
			moveSelectedEdgesBy(d);
		}

		// call super
		Super::mouseMoveEvent(mouseEvent);
		return;
	}

	// custom dragging
	moveDrag(mouseEvent, m_startDragItem, true);
}


void CNodeEditorScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
	//if (m_state == IS_None)
	if (m_startDragItem == NULL)
	{
		// call super
 		Super::mouseReleaseEvent(mouseEvent);
		return;
	}

	// release local grabber if any
	if (m_state == IS_Creating)
	{
		m_state = IS_Finishing;

		// cancel on RMB
		if (mouseEvent->button() == Qt::RightButton)
		{
			m_state = IS_Cancelling;
		}

		// cancel on same position
		if (m_startNode->pos() == m_endNode->pos())
		{
			m_state = IS_Cancelling;
		}
	}

	// call super
	finishDrag(mouseEvent, m_startDragItem, m_state == IS_Cancelling);

	// finish
	if (m_state == IS_Cancelling)
	{
		cancel(mouseEvent->scenePos());
	}
	else
	if (m_state = IS_Finishing)
	{
		m_connection->setSelected(true);
	}

	m_state = IS_None;

	// necessary to handle scene events properly
	QGraphicsScene::mouseReleaseEvent(mouseEvent);
}


void CNodeEditorScene::keyPressEvent(QKeyEvent *keyEvent)
{
	if (keyEvent->key() == Qt::Key_Escape)
	{
		cancel();
		return;
	}

	Super::keyPressEvent(keyEvent);
}


// handlers

bool CNodeEditorScene::onClickDrag(QGraphicsSceneMouseEvent *mouseEvent, const QPointF &clickPos)
{
	QGraphicsItem* item = itemAt(clickPos, QTransform());
	if (item)
	{
		if (!item->isEnabled())
			return false;

		if (!item->flags() & item->ItemIsMovable)
			return false;

		CItem *citem = dynamic_cast<CItem*>(item);
		if (citem)
			return citem->onClickDrag(mouseEvent, clickPos);

		// else start drag of item
		startDrag(item);
		return true;
	}

	// nothing to do
	return false;
}


bool CNodeEditorScene::onDoubleClickDrag(QGraphicsSceneMouseEvent *mouseEvent, const QPointF &clickPos)
{
	// try to start new connection at click point
	if (startNewConnection(clickPos))
		return true;

	// else handle by object under mouse
	QGraphicsItem* item = itemAt(clickPos, QTransform());
	if (item)
	{
		if (!item->isEnabled())
			return false;

		if (!item->flags() & item->ItemIsMovable)
			return false;

		CItem *citem = dynamic_cast<CItem*>(item);
		if (citem)
			return citem->onDoubleClickDrag(mouseEvent, clickPos);
	}

	// nothing to do
	return false;
}


void CNodeEditorScene::onDropped(QGraphicsSceneMouseEvent* mouseEvent, QGraphicsItem* dragItem)
{
	if (gridSnapEnabled())
	{
		// control point:
		if (auto cp = dynamic_cast<CControlPoint*>(dragItem))
		{
			auto newPos = getSnapped(cp->scenePos());
			cp->setPos(newPos);
			return;
		}

		// nodes & edges:
		QSet<QGraphicsItem*> items;
		QSet<CConnection*> edges;

		CNode *dragNode = dynamic_cast<CNode*>(dragItem);
		if (!dragNode) 
		{
			if (auto edge = dynamic_cast<CConnection*>(dragItem))
			{
				edges << edge;

				dragNode = edge->firstNode();
			}
		}

		if (dragNode)
		{
			items << dragNode;

			auto newPos = getSnapped(dragNode->scenePos());
			auto d = newPos - dragNode->scenePos();

			for (auto item : selectedItems())
			{
				if (auto edge = dynamic_cast<CConnection*>(item))
				{
					edges << edge;
					items << edge->firstNode();
					items << edge->lastNode();
				}
				else
					items << item;
			}

			for (auto item : items)
				item->moveBy(d.x(), d.y());

			for (auto edge : edges)
				edge->onItemMoved(d);

			return;
		}

		// whatever:
	}

	Super::onDropped(mouseEvent, dragItem);
}


// movement

void CNodeEditorScene::moveSelectedEdgesBy(const QPointF& d)
{
	QList<CConnection*> edges = getSelectedItems<CConnection>();
	if (edges.size())
	{
		QSet<CNode*> unselNodes;	// not selected nodes

									// move selected edges
		for (auto edge : edges)
		{
			if (!edge->firstNode()->isSelected())
				unselNodes << edge->firstNode();

			if (!edge->lastNode()->isSelected())
				unselNodes << edge->lastNode();

			edge->onItemMoved(d);
		}

		// force move non selected nodes of the selected edges
		for (auto node : unselNodes)
		{
			node->moveBy(d.x(), d.y());
		}
	}
}


// reimp

void CNodeEditorScene::moveSelectedItemsBy(const QPointF& d)
{
	QSet<QGraphicsItem*> items;
	QSet<CConnection*> edges;

	for (auto item : selectedItems())
	{
		if (auto edge = dynamic_cast<CConnection*>(item))
		{
			edges << edge;
			items << edge->firstNode();
			items << edge->lastNode();
		}
		else
			items << item; 
	}

	for (auto item : items)
		item->moveBy(d.x(), d.y());

	for (auto edge : edges)
		edge->onItemMoved(d);
}


QList<QGraphicsItem*> CNodeEditorScene::copyPasteItems() const
{
	// only selected edges & their nodes
	QList<QGraphicsItem*> result;

	QSet<QGraphicsItem*> nodes;

	for (auto item: selectedItems())
	{
		if (auto edge = dynamic_cast<CConnection*>(item))
		{
			result << edge;
			nodes << edge->firstNode();
			nodes << edge->lastNode();
		}
		else
		if (auto node = dynamic_cast<CNode*>(item))
		{
			// orphaned nodes only
			if (node->nodeFlags() & NF_OrphanAllowed)
				nodes << node;
		}
		else
			result << item;
	}

	result << nodes.toList();

    return result;
}


void CNodeEditorScene::drawBackground(QPainter *painter, const QRectF &r)
{
    Super::drawBackground(painter, r);
}


void CNodeEditorScene::drawItems(QPainter *painter, int numItems, QGraphicsItem *items[],
                                 const QStyleOptionGraphicsItem options[],
                                 QWidget *widget)
{
    QElapsedTimer tm;
    tm.start();

    static int maxtime = 0;

    // test only
//    Super::drawItems(painter, numItems, items, options, widget);

    for (int i = m_nextIndex; i < numItems; ++i)
    {
        // Draw the item
        painter->save();
        painter->setTransform(items[i]->sceneTransform(), true);
        items[i]->paint(painter, &options[i], widget);
        painter->restore();

//        if (tm.elapsed() > 50)
//        {
//            m_nextIndex = i+1;
//            update();
//            return;
//        }
    }

    m_nextIndex = 0;

    if (tm.elapsed() > maxtime)
    {
        maxtime = tm.elapsed();
        qDebug() << tm.elapsed();
    }

}


void CNodeEditorScene::updateMovedCursor(QGraphicsSceneMouseEvent *mouseEvent, QGraphicsItem* hoverItem)
{
	if (mouseEvent->buttons() == Qt::NoButton)
	{
		if (dynamic_cast<CControlPoint*>(hoverItem))
		{
			setSceneCursor(Qt::CrossCursor);
			return;
		}
	}

	return Super::updateMovedCursor(mouseEvent, hoverItem);
}


// selections

const QList<CNode*>& CNodeEditorScene::getSelectedNodes()
{
    if (m_selNodes.isEmpty())
        prefetchSelection();

    return m_selNodes;
}


const QList<CConnection*>& CNodeEditorScene::getSelectedEdges()
{
    if (m_selEdges.isEmpty())
        prefetchSelection();

    return m_selEdges;
}


void CNodeEditorScene::onSceneOrSelectionChanged()
{
    // drop cached selections
    m_selNodes.clear();
    m_selEdges.clear();
}


void CNodeEditorScene::prefetchSelection()
{
    m_selNodes.clear();
    m_selEdges.clear();

    auto selItems = selectedItems();

    for (auto* item : selItems)
    {
        if (CNode* node = dynamic_cast<CNode*>(item))
        {
            m_selNodes << node;
            continue;
        }

        if (CConnection* edge = dynamic_cast<CConnection*>(item))
        {
            m_selEdges << edge;
            continue;
        }
    }
}


// menu & actions

void CNodeEditorScene::onActionUnlink()
{
    QList<CNode*> nodes = getSelectedItems<CNode>(true);
    if (nodes.isEmpty())
        return;

    for (auto node : nodes)
    {
        node->unlink();
    }

    addUndoState();
}


void CNodeEditorScene::onActionNodeColor()
{
    QList<CNode*> nodes = getSelectedItems<CNode>(true);
    if (nodes.isEmpty())
        return;

    QColor color = QColorDialog::getColor(nodes.first()->getAttribute("color").value<QColor>());
    if (!color.isValid())
        return;

    for (auto node: nodes)
    {
        node->setAttribute("color", color);
    }

    addUndoState();
}


void CNodeEditorScene::onActionEdgeColor()
{
    QList<CConnection*> edges = getSelectedItems<CConnection>(true);
    if (edges.isEmpty())
        return;

    QColor color = QColorDialog::getColor(edges.first()->getAttribute("color").value<QColor>());
    if (!color.isValid())
        return;

    for (auto edge : edges)
    {
        edge->setAttribute("color", color);
    }

    addUndoState();
}


void CNodeEditorScene::onActionEdgeReverse()
{
    QList<CConnection*> edges = getSelectedItems<CConnection>(true);
    if (edges.isEmpty())
        return;

    for (auto edge : edges)
    {
        edge->reverse();
    }

    addUndoState();
}


void CNodeEditorScene::onActionEdgeDirected()
{
    QList<CConnection*> edges = getSelectedItems<CConnection>(true);
    if (edges.isEmpty())
        return;

    for (auto edge : edges)
    {
        edge->setAttribute("direction", "directed");
        edge->update();
    }

    addUndoState();
}


void CNodeEditorScene::onActionEdgeMutual()
{
    QList<CConnection*> edges = getSelectedItems<CConnection>(true);
    if (edges.isEmpty())
        return;

    for (auto edge : edges)
    {
        edge->setAttribute("direction", "mutual");
        edge->update();
    }

    addUndoState();
}


void CNodeEditorScene::onActionEdgeUndirected()
{
    QList<CConnection*> edges = getSelectedItems<CConnection>(true);
    if (edges.isEmpty())
        return;

    for (auto edge : edges)
    {
        edge->setAttribute("direction", "undirected");
        edge->update();
    }

    addUndoState();
}


bool CNodeEditorScene::populateMenu(QMenu& menu, QGraphicsItem* item, const QList<QGraphicsItem*>& selectedItems)
{
	if (!Super::populateMenu(menu, item, selectedItems))
		return false;

	// add default node actions
	menu.addSeparator();

	bool nodesSelected = getSelectedItems<CNode>(true).size();

	QAction *unlinkAction = menu.addAction(tr("Unlink"), this, SLOT(onActionUnlink()));
	unlinkAction->setEnabled(nodesSelected);

	QAction *nodeColorAction = menu.addAction(tr("Node(s) Color..."), this, SLOT(onActionNodeColor()));
	nodeColorAction->setEnabled(nodesSelected);

	// add default edge actions
	menu.addSeparator();

	bool edgesSelected = getSelectedItems<CConnection>(true).size();

	QAction *edgeColorAction = menu.addAction(tr("Connection(s) Color..."), this, SLOT(onActionEdgeColor()));
	edgeColorAction->setEnabled(edgesSelected);

	QMenu *arrowsMenu = menu.addMenu(tr("Direction"));
	arrowsMenu->setEnabled(edgesSelected);
	arrowsMenu->addAction(tr("Directed"), this, SLOT(onActionEdgeDirected()));
	arrowsMenu->addAction(tr("Mutual"), this, SLOT(onActionEdgeMutual()));
	arrowsMenu->addAction(tr("None"), this, SLOT(onActionEdgeUndirected()));
	arrowsMenu->addSeparator();
	arrowsMenu->addAction(tr("Reverse"), this, SLOT(onActionEdgeReverse()));

	return true;
}
