#include "CNodeEditorScene.h"
#include "CNodeSceneActions.h"
#include "CNode.h"
#include "CNodePort.h"
#include "CEdge.h"
#include "CDirectEdge.h"
#include "CPolyEdge.h"
#include "CControlPoint.h"
#include "CEditorSceneDefines.h"

#include <qvgeio/CGraphBase.h>

#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>
#include <QColorDialog> 
#include <QKeyEvent>
#include <QApplication>
#include <QDebug>
#include <QElapsedTimer>


CNodeEditorScene::CNodeEditorScene(QObject *parent) : Super(parent),
	m_editMode(EM_Default),
	m_startNode(NULL),
	m_endNode(NULL),
	m_connection(NULL),
	m_realStart(false),
	m_state(IS_None)
{
	// default factories
	registerItemFactory<CDirectEdge>();
	registerItemFactory<CNode>();
	registerItemFactory<CPolyEdge>();

	m_nodesFactory = factory<CNode>();
	m_edgesFactory = factory<CDirectEdge>();

	// test
	//setEdgesFactory(factory<CPolyEdge>());

	// go
	initialize();
}


bool CNodeEditorScene::fromGraph(const Graph& g)
{
	reset();

	
	// Graph attrs
	for (const auto& attr : g.graphAttrs)
	{
		if (attr.id == attr_labels_visIds)
		{
			auto graphVis = CUtils::visFromString(attr.defaultValue.toString());
			setVisibleClassAttributes("", graphVis);
			continue;
		}

		createClassAttribute("", attr.id, attr.name, attr.defaultValue, ATTR_NONE);
	}

	for (auto it = g.attrs.constBegin(); it != g.attrs.constEnd(); ++it)
	{
		if (it.key() == attr_labels_visIds)
			continue;

		setClassAttribute("", it.key(), it.value());
	}


	// Class attrs
	for (const auto& attr : g.nodeAttrs)
	{
		if (attr.id == attr_labels_visIds)
		{
			auto nodeVis = CUtils::visFromString(attr.defaultValue.toString());
			setVisibleClassAttributes("node", nodeVis);
			continue;
		}

		if (attr.id == attr_size)
			continue;	// ignore for now

		createClassAttribute("node", attr.id, attr.name, attr.defaultValue, ATTR_NONE);
	}


	for (const auto& attr : g.edgeAttrs)
	{
		if (attr.id == attr_labels_visIds)
		{
			auto edgeVis = CUtils::visFromString(attr.defaultValue.toString());
			setVisibleClassAttributes("edge", edgeVis);
			continue;
		}

		createClassAttribute("edge", attr.id, attr.name, attr.defaultValue, ATTR_NONE);
	}


	// Nodes
	QMap<QByteArray, CNode*> nodesMap;

	for (const Node& n : g.nodes)
	{
		CNode* node = createNewNode();
		addItem(node);

		node->setId(n.id);
		nodesMap[n.id] = node;

		for (auto it = n.attrs.constBegin(); it != n.attrs.constEnd(); ++it)
		{
			node->setAttribute(it.key(), it.value());
		}

		for (auto it = n.ports.constBegin(); it != n.ports.constEnd(); ++it)
		{
			CNodePort* port = node->addPort(it.key().toLatin1(), it.value().anchor, it.value().x, it.value().y);
			Q_ASSERT(port != nullptr);
			port->setColor(it.value().color);
		}
	}


	// Edges
	for (const Edge& e : g.edges)
	{
		CEdge* edge = createNewConnection();
		addItem(edge);

		edge->setId(e.id);
		edge->setFirstNode(nodesMap[e.startNodeId], e.startPortId);
		edge->setLastNode(nodesMap[e.endNodeId], e.endPortId);

		for (auto it = e.attrs.constBegin(); it != e.attrs.constEnd(); ++it)
		{
			edge->setAttribute(it.key(), it.value());
		}
	}

	// finalize
	setSceneRect(itemsBoundingRect());

	addUndoState();

	return true;
}


bool CNodeEditorScene::toGraph(Graph& g)
{
	g.clear();


	// class attributes
	auto graphAttrs = getClassAttributes("", false);
	for (auto it = graphAttrs.constBegin(); it != graphAttrs.constEnd(); ++it)
	{
		const auto& attr = *it;
		if (attr.flags & ATTR_VIRTUAL)
			continue;
		g.graphAttrs[it.key()] = attr;
	}

	auto nodeAttrs = getClassAttributes("node", false);
	for (auto it = nodeAttrs.constBegin(); it != nodeAttrs.constEnd(); ++it)
	{
		const auto& attr = *it;
		if (attr.flags & ATTR_VIRTUAL)
			continue;
		g.nodeAttrs[it.key()] = attr;
	}

	auto edgeAttrs = getClassAttributes("edge", false);
	for (auto it = edgeAttrs.constBegin(); it != edgeAttrs.constEnd(); ++it)
	{
		const auto& attr = *it;
		if (attr.flags & ATTR_VIRTUAL)
			continue;
		g.edgeAttrs[it.key()] = attr;
	}

	// temp solution
	g.nodeAttrs.remove("size");
	g.nodeAttrs.remove("pos");


	// visibility
	static AttrInfo _vis_({ attr_labels_visIds , "Visible Labels", QVariant::StringList});

	auto nodeVis = getVisibleClassAttributes("node", false);
	if (nodeVis.size())
	{
		_vis_.defaultValue = CUtils::byteArraySetToStringList(nodeVis);
		g.nodeAttrs[attr_labels_visIds] = _vis_;
	}

	auto edgeVis = getVisibleClassAttributes("edge", false);
	if (edgeVis.size())
	{
		_vis_.defaultValue = CUtils::byteArraySetToStringList(edgeVis);
		g.edgeAttrs[attr_labels_visIds] = _vis_;
	}

	auto graphVis = getVisibleClassAttributes("", false);
	if (graphVis.size())
	{
		_vis_.defaultValue = CUtils::byteArraySetToStringList(graphVis);
		g.graphAttrs[attr_labels_visIds] = _vis_;
	}


	// nodes
	auto nodes = getItems<CNode>();
	for (const auto &node : nodes)
	{
		Node n;
		n.id = node->getId().toLatin1();

		QByteArrayList ports = node->getPortIds();
		for (const auto &portId : ports)
		{
			auto port = node->getPort(portId);
			Q_ASSERT(port != NULL);

			NodePort p;
			p.name = portId;
			p.anchor = port->getAlign();
			p.x = port->getX();
			p.y = port->getY();
			p.color = port->getColor();

			n.ports[portId] = p;
		}

		n.attrs = node->getLocalAttributes();
		// temp solution
		n.attrs["x"] = node->pos().x();
		n.attrs["y"] = node->pos().y();
		n.attrs.remove("pos");
		n.attrs["width"] = node->getSize().width();
		n.attrs["height"] = node->getSize().height();
		n.attrs.remove("size");

		g.nodes.append(n);
	}


	// edges
	auto edges = getItems<CEdge>();
	for (const auto &edge : edges)
	{
		Edge e;
		e.id = edge->getId().toLatin1();
		e.startNodeId = edge->firstNode()->getId().toLatin1();
		e.endNodeId = edge->lastNode()->getId().toLatin1();
		e.startPortId = edge->firstPortId();
		e.endPortId = edge->lastPortId();

		e.attrs = edge->getLocalAttributes();

		g.edges.append(e);
	}


	// TODO

	return true;
}


// reimp

void CNodeEditorScene::initialize()
{
	Super::initialize();


	// common constrains
	static CAttributeConstrainsList *edgeStyles = new CAttributeConstrainsList();
	if (edgeStyles->ids.isEmpty()) {
		edgeStyles->names << tr("None") << tr("Solid") << tr("Dots") << tr("Dashes") << tr("Dash-Dot") << tr("Dash-Dot-Dot");
		edgeStyles->ids << "none" << "solid" << "dotted" << "dashed" << "dashdot" << "dashdotdot";
	}


	// default node attr
    CAttribute nodeAttr("color", "Color", QColor(Qt::magenta), ATTR_FIXED);
	setClassAttribute("node", nodeAttr);

    CAttribute shapeAttr("shape", "Shape", "disc", ATTR_FIXED);
	setClassAttribute("node", shapeAttr);

	createClassAttribute("node", "size", "Size", QSizeF(11.0, 11.0), ATTR_MAPPED | ATTR_FIXED);
	//createClassAttribute("node", "width", "Width", 11.0f, ATTR_MAPPED);
	//createClassAttribute("node", "height", "Height", 11.0f, ATTR_MAPPED);

	//createClassAttribute("node", "pos", "Position", QPointF(), ATTR_NODEFAULT | ATTR_MAPPED);
	createClassAttribute("node", "x", "X-Coordinate", 0.0f, ATTR_NODEFAULT | ATTR_MAPPED | ATTR_FIXED);
	createClassAttribute("node", "y", "Y-Coordinate", 0.0f, ATTR_NODEFAULT | ATTR_MAPPED | ATTR_FIXED);

	createClassAttribute("node", "stroke.style", "Stroke Style", "solid", ATTR_FIXED, edgeStyles);
	createClassAttribute("node", "stroke.size", "Stroke Size", 1.0, ATTR_FIXED);
	createClassAttribute("node", "stroke.color", "Stroke Color", QColor(Qt::black), ATTR_FIXED);

	createClassAttribute("node", "degree", "Degree", 0, ATTR_NODEFAULT | ATTR_VIRTUAL | ATTR_FIXED);


	// default edge attr
    CAttribute edgeAttr("color", "Color", QColor(Qt::gray), ATTR_FIXED);
	setClassAttribute("edge", edgeAttr);

	CAttribute directionAttr("direction", "Direction", "directed", ATTR_FIXED);
	setClassAttribute("edge", directionAttr);

	CAttribute weightAttr("weight", "Weight", 1.0, ATTR_FIXED);
	setClassAttribute("edge", weightAttr);

    CAttribute styleAttr("style", "Style", "solid", ATTR_FIXED);
    setClassAttribute("edge", styleAttr);


	static CAttributeConstrainsList *edgeDirections = new CAttributeConstrainsList();
	if (edgeDirections->ids.isEmpty()) {
		edgeDirections->names << tr("Directed (one end)") << tr("Mutual (both ends)") << tr("None (no ends)");
		edgeDirections->ids << "directed" << "mutual" << "undirected";
		edgeDirections->icons << QIcon(":/Icons/Edge-Directed") << QIcon(":/Icons/Edge-Mutual") << QIcon(":/Icons/Edge-Undirected");
	}
	setClassAttributeConstrains("edge", "direction", edgeDirections);

	setClassAttributeConstrains("edge", "style", edgeStyles);

	static CAttributeConstrainsList *nodeShapes = new CAttributeConstrainsList();
	if (nodeShapes->ids.isEmpty()) {
		nodeShapes->names << tr("Dics") << tr("Square") << tr("Triangle (up)") << tr("Triangle (down)") << tr("Diamond") << tr("Hexagon");
		nodeShapes->ids << "disc" << "square" << "triangle" << "triangle2" << "diamond" << "hexagon";
		nodeShapes->icons << QIcon(":/Icons/Node-Disc") << QIcon(":/Icons/Node-Square") << QIcon(":/Icons/Node-Triangle") 
			<< QIcon(":/Icons/Node-Triangle-Down") << QIcon(":/Icons/Node-Diamond") << QIcon(":/Icons/Node-Hexagon");
	}
	setClassAttributeConstrains("node", "shape", nodeShapes);
}


// nodes creation

void CNodeEditorScene::setEditMode(EditMode mode)
{
	if (m_editMode != mode)
	{
		m_editMode = mode;

		switch (m_editMode)
		{
		case EM_Transform:
			getCurrentView()->setDragMode(QGraphicsView::RubberBandDrag);
			startTransform(true);
			break;

		case EM_AddNodes:
			getCurrentView()->setDragMode(QGraphicsView::NoDrag);
			startTransform(false);
			break;

		default:
			getCurrentView()->setDragMode(QGraphicsView::RubberBandDrag);
			startTransform(false);
			break;
		}

		Q_EMIT editModeChanged(m_editMode);
	}
}


bool CNodeEditorScene::startNewConnection(const QPointF& pos)
{
	if (m_editMode == EM_Transform)
		return false;

	if (QGraphicsItem* item = getItemAt(pos))
	{
		if (!item->isEnabled())
			return false;

		// check for port first
		CNodePort *port = dynamic_cast<CNodePort*>(item);
		if (port)
		{
			CNode *node = port->getNode();
			Q_ASSERT(node != NULL);

			if (!node->allowStartConnection())
				return false;

			m_realStart = false;
			m_startNode = node;
			m_startNodePort = port;
		}
		else
		{
			// check for node
			CNode *node = dynamic_cast<CNode*>(item);
			if (!node)
				return false;

			if (!node->allowStartConnection())
				return false;

			m_realStart = false;
			m_startNode = node;
			m_startNodePort = NULL;
		}
	}
	else
	{
		m_realStart = true;
		m_startNode = createNewNode(getSnapped(pos));
		m_startNodePort = NULL;
	}

	m_endNode = createNewNode(getSnapped(pos));

	Super::startDrag(m_endNode);

	m_connection = createNewConnection(m_startNode, m_endNode);

	if (m_startNodePort)
		m_connection->setFirstNode(m_startNode, m_startNodePort->getId());

	m_state = IS_Creating;

    // auto select created items
    m_startNode->setSelected(false);
    m_connection->setSelected(true);
    m_endNode->setSelected(true);

	return true;
}


void CNodeEditorScene::cancel(const QPointF& /*pos*/)
{
	// if not cancelling already
	if (m_state != IS_Cancelling)
	{
		// cancel current drag operation
		Super::finishDrag(NULL, m_startDragItem, true);

		// if no creating state: return
		if (m_state != IS_Creating)
		{
			m_state = IS_None;
			return;
		}
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
	if (m_nodesFactory)
	{
		auto node = dynamic_cast<CNode*>(m_nodesFactory->create());
		Q_ASSERT(node);
		node->copyDataFrom(m_nodesFactory);
		return node;
	}

	// here default
	return new CNode;
}


CNode* CNodeEditorScene::createNewNode(const QPointF& pos)
{
	auto node = createNewNode();
	addItem(node);
	node->setPos(pos);
	return node;
}


CEdge* CNodeEditorScene::createNewConnection() const
{
	if (m_edgesFactory)
	{
		auto edge = dynamic_cast<CEdge*>(m_edgesFactory->create());
		Q_ASSERT(edge);
		edge->copyDataFrom(m_edgesFactory);
		return edge;
	}

	// here default
	return new CDirectEdge();
}


CEdge* CNodeEditorScene::createNewConnection(CNode* startNode, CNode* endNode)
{
	auto edge = createNewConnection();
	addItem(edge);
	edge->setFirstNode(startNode);
	edge->setLastNode(endNode);
	return edge;
}


void CNodeEditorScene::setNodesFactory(CNode* nodeFactory)
{
	m_nodesFactory = nodeFactory;
}


void CNodeEditorScene::setEdgesFactory(CEdge* edgeFactory)
{
	m_edgesFactory = edgeFactory;
}


// events

void CNodeEditorScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
	if (m_editItem)
	{
		// call super
		Super::mouseReleaseEvent(mouseEvent);
		return;
	}

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
			m_skipMenuEvent = true;
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
	if (m_state == IS_Finishing)
	{
		//m_connection->setSelected(true);
	}

	m_state = IS_None;

	// necessary to handle scene events properly
	QGraphicsScene::mouseReleaseEvent(mouseEvent);

	updateCursorState();
}


void CNodeEditorScene::keyPressEvent(QKeyEvent *keyEvent)
{
	bool isCtrl = (keyEvent->modifiers() == Qt::ControlModifier);
	bool isAlt = (keyEvent->modifiers() == Qt::AltModifier);
	bool isShift = (keyEvent->modifiers() == Qt::ShiftModifier);


	// Ctrl+Up/Down; alter size by 10%
	if (keyEvent->key() == Qt::Key_Up && isCtrl)
	{
		auto &nodes = getSelectedNodes();
		for (auto &node : nodes)
		{
			node->setAttribute(attr_size, node->getSize() * 1.1);
		}

		addUndoState();

		keyEvent->accept();
		return;
	}


	if (keyEvent->key() == Qt::Key_Down && isCtrl)
	{
		auto &nodes = getSelectedNodes();
		for (auto &node : nodes)
		{
			node->setAttribute(attr_size, node->getSize() / 1.1);
		}

		addUndoState();

		keyEvent->accept();
		return;
	}


	// cancel label edit
	if (keyEvent->key() == Qt::Key_Escape)
	{
		cancel();
		return;
	}

	Super::keyPressEvent(keyEvent);
}


// handlers

void CNodeEditorScene::onLeftButtonPressed(QGraphicsSceneMouseEvent *mouseEvent)
{
	Super::onLeftButtonPressed(mouseEvent);

	// add nodes?
	if (m_editMode == EM_AddNodes || isItemAt<CNodePort>(mouseEvent->scenePos()))
	{
		deselectAll();

		// skip calling super to avoid auto selection
		mouseEvent->accept();
	}
}


bool CNodeEditorScene::onClickDrag(QGraphicsSceneMouseEvent *mouseEvent, const QPointF &clickPos)
{
	// add nodes?
	if ((m_editMode == EM_AddNodes) || isItemAt<CNodePort>(clickPos))
	{
		if (startNewConnection(clickPos))
		{
			//setEditMode(EM_Default);
			return true;
		}
	}

	// else super
	return Super::onClickDrag(mouseEvent, clickPos);
}


bool CNodeEditorScene::onDoubleClickDrag(QGraphicsSceneMouseEvent *mouseEvent, const QPointF &clickPos)
{
	// debug
	if (clickPos == QPointF())
		qDebug() << "bug";

	// try to start new connection at click point
	if (startNewConnection(clickPos))
	{
		//mouseEvent->accept();
		return true;
	}

	// else call super
	return Super::onDoubleClickDrag(mouseEvent, clickPos);
}


void CNodeEditorScene::onDropped(QGraphicsSceneMouseEvent* mouseEvent, QGraphicsItem* dragItem)
{
	CNode *dragNode = dynamic_cast<CNode*>(dragItem);
	CEdge *dragEdge = dynamic_cast<CEdge*>(dragItem);

	// perform snap
	auto keys = mouseEvent->modifiers();
	bool isSnap = (keys & Qt::AltModifier) ? !gridSnapEnabled() : gridSnapEnabled();
	if (isSnap)
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
		QSet<CEdge*> edges;

		if (dragEdge) 
		{
			edges << dragEdge;

			dragNode = dragEdge->firstNode();
		}

		if (dragNode)
		{
			items << dragNode;

			auto newPos = getSnapped(dragNode->scenePos());
			auto d = newPos - dragNode->scenePos();

			for (auto item : selectedItems())
			{
				if (auto edge = dynamic_cast<CEdge*>(item))
				{
					edges << edge;

					if (dragEdge)
					{
						items << edge->firstNode();
						items << edge->lastNode();
					}
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


void CNodeEditorScene::onLeftClick(QGraphicsSceneMouseEvent* mouseEvent, QGraphicsItem* clickedItem)
{
	if (m_editMode == EM_AddNodes)
	{
		// clicked on empty space?
		if (!clickedItem)
		{
			onLeftDoubleClick(mouseEvent, clickedItem);
			//setEditMode(EM_Default);
			return;
		}
	}

	Super::onLeftClick(mouseEvent, clickedItem);
}


void CNodeEditorScene::onLeftDoubleClick(QGraphicsSceneMouseEvent* mouseEvent, QGraphicsItem* clickedItem)
{
	// clicked on empty space?
	if (!clickedItem)
	{
		// create a node here
		auto node = createNewNode(getSnapped(mouseEvent->scenePos()));
		node->setSelected(true);

		addUndoState();
		return;
	}

	Super::onLeftDoubleClick(mouseEvent, clickedItem);
}


// movement

void CNodeEditorScene::moveSelectedEdgesBy(const QPointF& d)
{
	auto edges = getSelectedEdges();
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
	QSet<CEdge*> edges;

	// if dragging nodes and there are selected nodes: do not drag not-selected nodes
	auto dragNode = dynamic_cast<CNode*>(m_startDragItem);

	for (auto item : selectedItems())
	{
		if (!(item->flags() & item->ItemIsMovable))
			continue;

		if (auto edge = dynamic_cast<CEdge*>(item))
		{
			edges << edge;

			if (!dragNode)
			{
				items << edge->firstNode();
				items << edge->lastNode();
			}
		}
		else
			items << item; 
	}

	for (auto item : items)
		item->moveBy(d.x(), d.y());

	for (auto edge : edges)
		edge->onItemMoved(d);
}


QList<QGraphicsItem*> CNodeEditorScene::getCopyPasteItems() const
{
	// only selected edges & their nodes
	QList<QGraphicsItem*> result;

	QSet<QGraphicsItem*> nodes;

	for (auto item: selectedItems())
	{
		if (auto edge = dynamic_cast<CEdge*>(item))
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


QList<QGraphicsItem*> CNodeEditorScene::getTransformableItems() const
{
	QList<QGraphicsItem*> result;
	
	auto nodes = getSelectedNodes();
	for (auto node : nodes)
		result << node;

	return result;
}


bool CNodeEditorScene::doUpdateCursorState(Qt::KeyboardModifiers keys, Qt::MouseButtons buttons, QGraphicsItem *hoverItem)
{
	// port?
	if (CNodePort *portItem = dynamic_cast<CNodePort*>(hoverItem))
	{
		if (portItem->isEnabled())
		{
			setSceneCursor(Qt::CrossCursor);
			setInfoStatus(SIS_Hover_Port);
			return true;
		}
	}

	// hover item?
	if (m_editMode == EM_AddNodes)
	{
		if (hoverItem)
		{
			if (hoverItem->isEnabled())
			{
				setSceneCursor(Qt::CrossCursor);
				setInfoStatus(SIS_Hover);
				return true;
			}
		}
	}

	// handled by super?
	if (Super::doUpdateCursorState(keys, buttons, hoverItem))
		return true;

	// still not handled
	return false;
}


// painting

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



// selections

const QList<CNode*>& CNodeEditorScene::getSelectedNodes() const
{
    if (m_selNodes.isEmpty())
        prefetchSelection();

    return m_selNodes;
}


const QList<CEdge*>& CNodeEditorScene::getSelectedEdges() const
{
    if (m_selEdges.isEmpty())
        prefetchSelection();

    return m_selEdges;
}


const QList<CItem*>& CNodeEditorScene::getSelectedNodesEdges() const
{
	if (m_selItems.isEmpty())
		prefetchSelection();

	return m_selItems;
}


void CNodeEditorScene::onSelectionChanged()
{
    // drop cached selections
    m_selNodes.clear();
    m_selEdges.clear();
	m_selItems.clear();

	Super::onSelectionChanged();
}


void CNodeEditorScene::prefetchSelection() const
{
    m_selNodes.clear();
    m_selEdges.clear();
	m_selItems.clear();

    auto selItems = selectedItems();

    for (auto* item : selItems)
    {
        if (CNode* node = dynamic_cast<CNode*>(item))
        {
            m_selNodes << node;
			m_selItems << node;
            continue;
        }

        if (CEdge* edge = dynamic_cast<CEdge*>(item))
        {
            m_selEdges << edge;
			m_selItems << edge;
            continue;
        }
    }
}


// menu & actions

QObject* CNodeEditorScene::createActions()
{
	return new CNodeSceneActions(this);
}

