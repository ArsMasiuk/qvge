/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CFileSerializerDOT.h"
#include "CNode.h"
#include "CConnection.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>


// reimp

bool CFileSerializerDOT::save(const QString& fileName, const CEditorScene& scene) const
{
	QFile saveFile(fileName);
	if (saveFile.open(QFile::WriteOnly))
	{
        QTextStream ts(&saveFile);

        QString graphId = QFileInfo(fileName).completeBaseName();

        ts << "digraph \"" << graphId << "\" {\n";

		ts << "\n\n";

        // nodes
        auto nodes = scene.getItems<CNode>();
        for (auto node: nodes)
        {
            ts << node->getId();
            ts << " [\n";
			doWriteNode(ts, *node, scene);
            ts << "];\n";
        }

		ts << "\n\n";

        // edges
        auto edges = scene.getItems<CConnection>();
        for (auto edge: edges)
        {
			doWriteEdge(ts, *edge, scene);
        }

		ts << "\n\n";
        ts << "}\n";

		return true;
	}

	return false;
}


// helpers

static QString toDotShape(const QString& shape)
{
	// rename to conform dot
	if (shape == "disc")		return "ellipse";
	if (shape == "square")		return "rect";
	if (shape == "triangle2")	return "invtriangle";

	// else take original
	return shape;
}


void CFileSerializerDOT::doWriteNode(QTextStream& ts, const CNode& node, const CEditorScene& scene) const
{
	ts << "pos = \"" << node.pos().x() << "," << node.pos().y() << "\"\n";

	const auto& nodeAttrs = node.getLocalAttributes();

	if (nodeAttrs.contains("color")) {
		ts << ",fillcolor = \"" << nodeAttrs["color"].toString() << "\"\n";
	}

	if (nodeAttrs.contains("label")) {
		ts << ",comment = \"" << nodeAttrs["label"].toString() << "\"\n";	// ogdf 
		ts << ",xlabel = \"" << nodeAttrs["label"].toString() << "\"\n";	// dot standard
	}

	if (nodeAttrs.contains("size")) {
		ts << ",width = \"" << nodeAttrs["size"].toSizeF().width() << "\"";
		ts << ",height = \"" << nodeAttrs["size"].toSizeF().height() << "\"";
		ts << "\n";
	}

	if (nodeAttrs.contains("shape")) {
		ts << ",shape = \"" << toDotShape(nodeAttrs["shape"].toString()) << "\"\n";	
	}
}


void CFileSerializerDOT::doWriteEdge(QTextStream& ts, const CConnection& edge, const CEditorScene& scene) const
{
	const auto& edgeAttrs = edge.getLocalAttributes();

	ts << edge.firstNode()->getId();

	auto dir = edge.getAttribute("direction").toString();
	if (dir == "undirected")
		ts << " -- ";
	else
		ts << " -> ";

	ts << edge.lastNode()->getId();
	
	ts << " [id = \"" << edge.getId() << "\"\n";

	if (dir == "mutual") {
		ts << ",dir=both" << "\n";
	}

	if (edgeAttrs.contains("color")) {
		ts << ",color = \"" << edgeAttrs["color"].toString() << "\"\n";
	}

	if (edgeAttrs.contains("label")) {
		ts << ",label = \"" << edgeAttrs["label"].toString() << "\"\n";	
	}

	if (edgeAttrs.contains("weight")) {
		ts << ",weight = \"" << edgeAttrs["weight"].toString() << "\"\n";
	}

	if (edgeAttrs.contains("style")) {
		ts << ",style = \"" << edgeAttrs["style"].toString() << "\"\n";
	}

	ts << "];\n";
}
