#include "COGDFLayout.h"

#include <qvge/CNodeEditorScene.h>
#include <qvge/CNode.h>
#include <qvge/CConnection.h>

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/module/LayoutModule.h>
#include <ogdf/fileformats/GraphIO.h>

#include <QMap>
#include <QApplication>
#include <QFileInfo>


COGDFLayout::COGDFLayout()
{
}


static QVariant toVariant(ogdf::Shape shape)
{
    using namespace ogdf;

    switch (shape)
    {
    case Shape::Rect:           return "square";
    case Shape::RoundedRect:    return "rsquare";
    case Shape::Ellipse:        return "disc";
    case Shape::Triangle:       return "triangle";
    case Shape::Pentagon:       return "star";
    case Shape::Hexagon:        return "hexagon";
    case Shape::Octagon:        return "octagon";
    case Shape::Rhomb:          return "diamond";
    case Shape::Trapeze:        return "trapeze";
    case Shape::Parallelogram:  return "parallelogram";
    case Shape::InvTriangle:    return "triangle2";
    case Shape::InvTrapeze:     return "trapeze2";
    case Shape::InvParallelogram:  return "parallelogram2";
    case Shape::Image:          return "image";
    }

    return QVariant();
}


static QVariant toVariant(ogdf::StrokeType stroke)
{
    using namespace ogdf;

    switch (stroke)
    {
    case StrokeType::Solid:         return "solid";
    case StrokeType::Dash:          return "dashed";
    case StrokeType::Dot:           return "dotted";
    case StrokeType::Dashdot:       return "dashdot";
    case StrokeType::Dashdotdot:    return "dashdotdot";
    default:;
    }

    return QVariant();
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
//        GA.x(n) = node->pos().x();
//        GA.y(n) = node->pos().y();
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

        node->setPos(GA.x(n), GA.y(n));
    }

    // finalize
    scene.setSceneRect(scene.itemsBoundingRect());

    scene.addUndoState();


	QApplication::restoreOverrideCursor();
}


void COGDFLayout::graphTopologyToScene(const ogdf::Graph &G, const ogdf::GraphAttributes &GA, CNodeEditorScene &scene)
{
    scene.reset();

    // create nodes
    QMap<ogdf::node, CNode*> nodeMap;

    for (auto n: G.nodes)
    {
        CNode* node = scene.createNewNode();
        scene.addItem(node);

        nodeMap[n] = node;

        if (GA.has(GA.nodeGraphics))
        {
            node->setPos(GA.x(n), GA.y(n));
        }
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


void COGDFLayout::graphToScene(const ogdf::Graph &G, const ogdf::GraphAttributes &GA, CNodeEditorScene &scene)
{
    scene.reset();

    // create nodes
    QMap<ogdf::node, CNode*> nodeMap;

    for (auto n: G.nodes)
    {
        CNode* node = scene.createNewNode();
        scene.addItem(node);

        nodeMap[n] = node;

        if (GA.has(GA.nodeGraphics))
        {
            node->setPos(GA.x(n), GA.y(n));
            node->setAttribute("size", QSizeF(GA.width(n), GA.height(n)));
            node->setAttribute("shape", toVariant(GA.shape(n)));
        }

        if (GA.has(GA.nodeStyle))
        {
            auto c = GA.fillColor(n);
            node->setAttribute("color", QColor(c.red(), c.green(), c.blue()));
        }

		int id = -1;
		if (GA.has(GA.nodeId)) {
			id = GA.idNode(n);
			if (id >= 0) node->setId(QString::number(id));
		}

        if (GA.has(GA.nodeLabel)) {
			auto label = QString::fromStdString(GA.label(n));	// label -> ID
			if (id < 0 && label.size()) node->setId(label);
		}

		if (GA.has(GA.nodeTemplate)) {
			auto label = QString::fromStdString(GA.templateNode(n));	// comment -> label
			if (label.size())
				node->setAttribute("label", label);
		}

        if (GA.has(GA.nodeWeight))
            node->setAttribute("weight", GA.weight(n));
    }


    for (auto e: G.edges)
    {
        CConnection* edge = scene.createNewConnection();
        scene.addItem(edge);

        edge->setFirstNode(nodeMap[e->source()]);
        edge->setLastNode(nodeMap[e->target()]);

        if (GA.has(GA.edgeDoubleWeight))
            edge->setAttribute("weight", GA.doubleWeight(e));
        else if (GA.has(GA.edgeIntWeight))
            edge->setAttribute("weight", GA.intWeight(e));

        if (GA.has(GA.edgeLabel))
            edge->setAttribute("label", QString::fromStdString(GA.label(e)));

        if (GA.has(GA.edgeStyle))
        {
            auto c = GA.strokeColor(e);
            edge->setAttribute("color", QColor(c.red(), c.green(), c.blue()));

            edge->setAttribute("style", toVariant(GA.strokeType(e)));
        }
    }


    // finalize
    scene.setSceneRect(scene.itemsBoundingRect());

    scene.addUndoState();
}


// file IO

bool COGDFLayout::loadGraph(const std::string &filename, CNodeEditorScene &scene)
{
    ogdf::Graph G;
    ogdf::GraphAttributes GA(G, 0xffffff);   // all attrs

    QString format = QFileInfo(QString::fromStdString(filename)).suffix().toLower();

    bool ok = false;
    if (format == "gml")
        ok = ogdf::GraphIO::readGML(GA, G, filename);
    else if (format == "dot")
        ok = ogdf::GraphIO::readDOT(GA, G, filename);

    if (ok)
    {
        graphToScene(G, GA, scene);
    }

    return ok;
}

