/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CFileSerializerDOT.h"
#include "CNode.h"
#include "CEdge.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>


// reimp

bool CFileSerializerDOT::save(const QString& fileName, CEditorScene& scene, QString* lastError) const
{
	QFile saveFile(fileName);
	if (saveFile.open(QFile::WriteOnly))
	{
        QTextStream ts(&saveFile);
		ts.setCodec("UTF-8");
		ts.setGenerateByteOrderMark(true);

        QString graphId = QFileInfo(fileName).completeBaseName();

        ts << "digraph \"" << graphId << "\"\n{";
		ts << "\n\n";

		// we'll output points, not inches
		//ts << "\n\n";
		//ts << "inputscale = 72;";

		// background
		if (m_writeBackground)
		{
			ts << "bgcolor = \"" << scene.backgroundBrush().color().name() << "\"";
			ts << "\n\n";
		}

        // nodes
		if (m_writeAttrs)
		{
			doWriteNodeDefaults(ts, scene);
			ts << "\n\n";
		}

        auto nodes = scene.getItems<CNode>();
        for (auto node: nodes)
        {
			doWriteNode(ts, *node, scene);
        }

		ts << "\n\n";


        // edges
		if (m_writeAttrs)
		{
			doWriteEdgeDefaults(ts, scene);
			ts << "\n\n";
		}

        auto edges = scene.getItems<CEdge>();
        for (auto edge: edges)
        {
			doWriteEdge(ts, *edge, scene);
        }

		ts << "\n}\n";

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
		if (!(attr.flags & ATTR_NODEFAULT))
			nodeAttrs[attr.id] = attr.defaultValue;
	}

	// add visible state if any
	QSet<QByteArray> visSet = scene.getVisibleClassAttributes("node", false);
	if (!visSet.isEmpty())
		nodeAttrs["_vis_"] = visSet.toList().join('|');

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
	ts << "\"" << node.getId() << "\"";

	if (m_writeAttrs)
	{
		ts << " [\n";

		ts << "pos = \"" << node.pos().x() / 72.0 << "," << -node.pos().y() / 72.0 << "!\"\n";	//  / 72.0 -> point to inch; -y

		const QMap<QByteArray, QVariant>& nodeAttrs = node.getLocalAttributes();

		doWriteNodeAttrs(ts, nodeAttrs);

		ts << "]";
	}

	ts << "\n\n";
}


void CFileSerializerDOT::doWriteNodeAttrs(QTextStream& ts, QMap<QByteArray, QVariant> nodeAttrs) const
{
	bool styleUsed = false;	// to avoid duplicated setting of node style

	// standard attrs
	if (nodeAttrs.contains("color")) {
		QColor c(nodeAttrs["color"].value<QColor>());
		if (c.isValid())
		{
			ts << ",fillcolor = \"" << c.name() << "\"";
			ts << ",style = \"filled\"\n";
			styleUsed = true;
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

	if (nodeAttrs.contains("label.font")) {
		auto f = nodeAttrs["label.font"].value<QFont>();
		ts << ",fontname = \"" << f.family() << "\"\n";
		ts << ",fontsize = \"" << f.pointSizeF() << "\"\n";
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
		if (!styleUsed) {
			ts << ",style = \"" << nodeAttrs["stroke.style"].toString() << "\"\n";
			styleUsed = true;
		}
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
		if (!(attr.flags & ATTR_NODEFAULT))
			edgeAttrs[attr.id] = attr.defaultValue;
	}

	// add visible state if any
	QSet<QByteArray> visSet = scene.getVisibleClassAttributes("edge", false);
	if (!visSet.isEmpty())
		edgeAttrs["_vis_"] = visSet.toList().join('|');

	// write it down
	if (edgeAttrs.size())
	{
		ts << "edge [\n";
		ts << "class = \"edge\"\n";

		doWriteEdgeAttrs(ts, edgeAttrs);

		ts << "];\n";
	}
}


void CFileSerializerDOT::doWriteEdge(QTextStream& ts, const CEdge& edge, const CEditorScene& /*scene*/) const
{
	const auto& edgeAttrs = edge.getLocalAttributes();

	ts << "\"" << edge.firstNode()->getId() << "\"";
	if (edge.firstPortId().size())
		ts << ":" << "\"" << edge.firstPortId() << "\"";

	ts << " -> ";

	ts << "\"" << edge.lastNode()->getId() << "\"";
	if (edge.lastPortId().size())
		ts << ":" << "\"" << edge.lastPortId() << "\"";

	ts << " [id = \"" << edge.getId() << "\"\n";

	if (m_writeAttrs)
	{
		doWriteEdgeAttrs(ts, edgeAttrs);
	}

	ts << "];\n\n";
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

	if (edgeAttrs.contains("label.font")) {
		auto f = edgeAttrs["label.font"].value<QFont>();
		ts << ",fontname = \"" << f.family() << "\"\n";
		ts << ",fontsize = \"" << f.pointSizeF() << "\"\n";
		edgeAttrs.remove("label.font");
	}

	// custom attrs
	for (auto it = edgeAttrs.constBegin(); it != edgeAttrs.constEnd(); ++it)
	{
		ts << ",\"" << it.key() << "\" = \"" << toDotString(it.value()) << "\"\n";
	}
}

