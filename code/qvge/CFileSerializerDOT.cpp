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

bool CFileSerializerDOT::save(const QString& fileName, CEditorScene& scene) const
{
	QFile saveFile(fileName);
	if (saveFile.open(QFile::WriteOnly))
	{
        QTextStream ts(&saveFile);

        QString graphId = QFileInfo(fileName).completeBaseName();

        ts << "digraph \"" << graphId << "\" {\n";

		ts << "\n\n";

        // nodes
		doWriteNodeDefaults(ts, scene);

		ts << "\n\n";

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
		doWriteEdgeDefaults(ts, scene);

		ts << "\n\n";

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


static QString toDotString(const QVariant& v)
{
	QString val = v.toString();
	return val.replace('"', "'");
}


void CFileSerializerDOT::doWriteNodeDefaults(QTextStream& ts, const CEditorScene& scene) const
{
	// build map of default attrs
	QMap<QByteArray, QVariant> nodeAttrs;

	const AttributesMap& nodeAttrMap = scene.getClassAttributes("node", false);
	for (const auto &attr : nodeAttrMap)
	{
		if (!attr.noDefault)
			nodeAttrs[attr.id] = attr.defaultValue;
	}

	// write it down
	if (nodeAttrs.size())
	{
		ts << "node [\n";
		ts << "class = \"node\"\n";

		doWriteNodeAttrs(ts, nodeAttrs);

		ts << "];\n";
	}
}


void CFileSerializerDOT::doWriteNode(QTextStream& ts, const CNode& node, const CEditorScene& /*scene*/) const
{
	ts << "pos = \"" << node.pos().x() / 72.0  << "," << -node.pos().y() / 72.0 << "!\"\n";	//  / 72.0 -> point to inch; -y

	const QMap<QByteArray, QVariant>& nodeAttrs = node.getLocalAttributes();

	doWriteNodeAttrs(ts, nodeAttrs);
}


void CFileSerializerDOT::doWriteNodeAttrs(QTextStream& ts, QMap<QByteArray, QVariant> nodeAttrs) const
{
	// standard attrs
	if (nodeAttrs.contains("color")) {
		QColor c(nodeAttrs["color"].value<QColor>());
		if (c.isValid())
		{
			ts << ",fillcolor = \"" << c.name() << "\"";
			ts << ",style = \"filled\"\n";
		}

		nodeAttrs.remove("color");
	}

	if (nodeAttrs.contains("size")) {
		ts << ",width = \"" << nodeAttrs["size"].toSizeF().width() / 72.0 << "\"";		//  / 72.0 -> point to inch
		ts << ",height = \"" << nodeAttrs["size"].toSizeF().height() / 72.0 << "\"";
		ts << "\n";
		nodeAttrs.remove("size");
	}

	if (nodeAttrs.contains("shape")) {
		ts << ",shape = \"" << toDotShape(nodeAttrs["shape"].toString()) << "\"\n";
		nodeAttrs.remove("shape");
	}

	if (nodeAttrs.contains("label")) {
		ts << ",xlabel = \"" << toDotString(nodeAttrs["label"]) << "\"\n";
		nodeAttrs.remove("label");
	}

	if (nodeAttrs.contains("label.color")) {
		ts << ",fontcolor = \"" << nodeAttrs["label.color"].toString() << "\"\n";
		nodeAttrs.remove("label.color");
	}

	if (nodeAttrs.contains("label.size")) {
		ts << ",fontsize = \"" << nodeAttrs["label.size"].toString() << "\"\n";
		nodeAttrs.remove("label.size");
	}

	if (nodeAttrs.contains("label.font")) {
		ts << ",fontname = \"" << nodeAttrs["label.font"].value<QFont>().family() << "\"\n";
		nodeAttrs.remove("label.font");
	}

	if (nodeAttrs.contains("stroke.color")) {
		ts << ",color = \"" << nodeAttrs["stroke.color"].toString() << "\"\n";
		nodeAttrs.remove("stroke.color");
	}

	if (nodeAttrs.contains("stroke.size")) {
		ts << ",penwidth = \"" << nodeAttrs["stroke.size"].toString() << "\"\n";
		nodeAttrs.remove("stroke.size");
	}

	if (nodeAttrs.contains("stroke.style")) {
		ts << ",style = \"" << nodeAttrs["stroke.style"].toString() << "\"\n";
		nodeAttrs.remove("stroke.style");
	}
	
	// custom attrs
	for (auto it = nodeAttrs.constBegin(); it != nodeAttrs.constEnd(); ++it)
	{
		ts << ",\"" << it.key() << "\" = \"" << toDotString(it.value()) << "\"\n";
	}
}


void CFileSerializerDOT::doWriteEdgeDefaults(QTextStream& ts, const CEditorScene& scene) const
{
	// build map of default attrs
	QMap<QByteArray, QVariant> edgeAttrs;

	const AttributesMap& edgeAttrMap = scene.getClassAttributes("edge", false);
	for (const auto &attr : edgeAttrMap)
	{
		if (!attr.noDefault)
			edgeAttrs[attr.id] = attr.defaultValue;
	}

	// write it down
	if (edgeAttrs.size())
	{
		ts << "edge [\n";
		ts << "class = \"edge\"\n";

		doWriteEdgeAttrs(ts, edgeAttrs);

		ts << "];\n";
	}
}


void CFileSerializerDOT::doWriteEdge(QTextStream& ts, const CConnection& edge, const CEditorScene& /*scene*/) const
{
	const auto& edgeAttrs = edge.getLocalAttributes();

	ts << edge.firstNode()->getId();
	ts << " -> ";
	ts << edge.lastNode()->getId();
	
	ts << " [id = \"" << edge.getId() << "\"\n";

	doWriteEdgeAttrs(ts, edgeAttrs);

	ts << "];\n";
}


void CFileSerializerDOT::doWriteEdgeAttrs(QTextStream& ts, QMap<QByteArray, QVariant> edgeAttrs) const
{
	if (edgeAttrs.contains("direction")) {
		auto dir = edgeAttrs["direction"].toString();
		edgeAttrs.remove("direction");

		if (dir == "mutual")
			ts << ",dir=both" << "\n";
		else if (dir == "undirected")
			ts << ",dir=none" << "\n";
	}

	if (edgeAttrs.contains("weight")) {
		ts << ",weight = \"" << edgeAttrs["weight"].toString() << "\"\n";
		ts << ",penwidth = \"" << edgeAttrs["weight"].toString() << "\"\n";
		edgeAttrs.remove("weight");
	}

	if (edgeAttrs.contains("label")) {
		ts << ",xlabel = \"" << toDotString(edgeAttrs["label"]) << "\"\n";
		edgeAttrs.remove("label");
	}

	if (edgeAttrs.contains("label.color")) {
		ts << ",fontcolor = \"" << edgeAttrs["label.color"].toString() << "\"\n";
		edgeAttrs.remove("label.color");
	}

	if (edgeAttrs.contains("label.size")) {
		ts << ",fontsize = \"" << edgeAttrs["label.size"].toString() << "\"\n";
		edgeAttrs.remove("label.size");
	}

	if (edgeAttrs.contains("label.font")) {
		ts << ",fontname = \"" << edgeAttrs["label.font"].value<QFont>().family() << "\"\n";
		edgeAttrs.remove("label.font");
	}

	// custom attrs
	for (auto it = edgeAttrs.constBegin(); it != edgeAttrs.constEnd(); ++it)
	{
		ts << ",\"" << it.key() << "\" = \"" << toDotString(it.value()) << "\"\n";
	}
}

