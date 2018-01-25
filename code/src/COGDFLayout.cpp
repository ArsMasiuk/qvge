#include "COGDFLayout.h"

#include <qvge/CNodeEditorScene.h>
#include <qvge/CNode.h>
#include <qvge/CConnection.h>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/module/LayoutModule.h>

#include <QMap>
#include <QApplication>


COGDFLayout::COGDFLayout()
{
}


void COGDFLayout::doLayout(ogdf::LayoutModule &layout, CNodeEditorScene &scene)
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    ogdf::Graph G;
    ogdf::GraphAttributes GA(G, ogdf::GraphAttributes::nodeGraphics | ogdf::GraphAttributes::edgeGraphics);

    // qvge -> ogdf
    auto nodes = scene.getItems<CNode>();
    auto edges = scene.getItems<CConnection>();

    QMap<CNode*, ogdf::node> nodeMap;

    for (CNode* node : nodes)
    {
        ogdf::node n = G.newNode();
//        GA.x(n) = node->getSceneItem()->pos().x();
//        GA.y(n) = node->getSceneItem()->pos().y();
        GA.x(n) = 0;
        GA.y(n) = 0;

        nodeMap[node] = n;
    }

    for (CConnection* edge: edges)
    {
        ogdf::node n1 = nodeMap[edge->firstNode()];
        ogdf::node n2 = nodeMap[edge->lastNode()];
        ogdf::edge e = G.newEdge(n1, n2);
    }


    // ogdf layout
    layout.call(GA);


    // ogdf -> qvge
    for (auto it = nodeMap.begin(); it != nodeMap.end(); ++it)
    {
        CNode* node = it.key();
        ogdf::node n = it.value();

        node->getSceneItem()->setPos(GA.x(n), GA.y(n));
    }

    // finalize
    scene.setSceneRect(scene.itemsBoundingRect());

    scene.addUndoState();


	QApplication::restoreOverrideCursor();
}


void COGDFLayout::graphToScene(const ogdf::Graph &G, const ogdf::GraphAttributes &GA, CNodeEditorScene &scene)
{
    scene.reset();

    // create nodes
    QMap<ogdf::node, CNode*> nodeMap;

    for (auto n: G.nodes)
    {
        CNode* node = scene.createNewNode();
        scene.addItem(node);

        node->getSceneItem()->setPos(GA.x(n), GA.y(n));

        nodeMap[n] = node;
    }

    for (auto e: G.edges)
    {
        CConnection* edge = scene.createNewConnection();
        scene.addItem(edge);

        edge->setFirstNode(nodeMap[e->source()]);
        edge->setLastNode(nodeMap[e->target()]);
    }


    // finalize
    scene.setSceneRect(scene.itemsBoundingRect());

    scene.addUndoState();
}

