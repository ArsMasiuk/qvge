/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CFormatDOT.h"

#include <QFile>
#include <QDebug>
#include <QTextStream>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>


struct DotVertex {
	std::string id;
	std::string label;
	std::string color;
	std::string fillcolor;
	std::string pos;
	std::string shape;
	float width = .0;
	float height = .0;
};

struct DotEdge {
	std::string id;
	std::string label;
	std::string color;
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, DotVertex, DotEdge> graph_t;


// helpers

static QString fromDotShape(const std::string& shape)
{
	// rename to conform dot
	if (shape == "ellipse")		return "disc";
	if (shape == "rect")		return "square";
	if (shape == "invtriangle")	return "triangle2";

	// else take original
	return QString::fromStdString(shape);
}


// reimp

bool CFormatDOT::load(const QString& fileName, Graph& g, QString* lastError) const
{
	graph_t graphviz;
	boost::dynamic_properties dp(boost::ignore_other_properties);

	dp.property("node_id", boost::get(&DotVertex::id, graphviz));
	dp.property("label", boost::get(&DotVertex::label, graphviz));
	dp.property("color", boost::get(&DotVertex::color, graphviz));
	dp.property("fillcolor", boost::get(&DotVertex::fillcolor, graphviz));
	dp.property("width", boost::get(&DotVertex::width, graphviz));
	dp.property("height", boost::get(&DotVertex::height, graphviz));
	dp.property("pos", boost::get(&DotVertex::pos, graphviz));
	dp.property("shape", boost::get(&DotVertex::shape, graphviz));

	dp.property("id", boost::get(&DotEdge::id, graphviz));
	dp.property("label", boost::get(&DotEdge::label, graphviz));
	dp.property("color", boost::get(&DotEdge::color, graphviz));

	std::ifstream dot(fileName.toStdString());
	bool status = boost::read_graphviz(dot, graphviz, dp);

	if (!status)
	{
		*lastError = ("Failed reading DOT format");
		return false;
	}

	//int edgesCount = graphviz.m_edges.size();	// is always empty...

	// nodes
	int nodesCount = graphviz.m_vertices.size();

	for (int i = 0; i < nodesCount; ++i)
	{
		const DotVertex &v = graphviz.m_vertices[i].m_property;

		Node n;
		n.id = QByteArray::fromStdString(v.id);

		if (v.fillcolor.size())
			n.attrs["color"] = QColor(QString::fromStdString(v.fillcolor));

		if (v.color.size())
			n.attrs["stroke.color"] = QColor(QString::fromStdString(v.color));
		
		if (v.width != .0)
			n.attrs["width"] = v.width * 72.0;

		if (v.height != .0)
			n.attrs["height"] = v.height * 72.0;

		if (v.pos.size())
		{
			float x, y;
			std::sscanf(v.pos.data(), "%f,%f", &x, &y);
			n.attrs["x"] = x * 72.0;
			n.attrs["y"] = -y * 72.0;
		}

		if (v.shape.size())
			n.attrs["shape"] = fromDotShape(v.shape);

		// to do:
		// - stroke
		// - label
		// - fill

		g.nodes.append(n);
	}

	// edges
	for (int i = 0; i < nodesCount; ++i)
	{
		const auto &gvnode_out_edges = graphviz.m_vertices[i].m_out_edges;
		for (const auto &gvedge_props : gvnode_out_edges)
		{
			auto gvtarget = gvedge_props.get_target();
			const DotEdge &edge = gvedge_props.get_property();

			Edge e;
			e.startNodeId = g.nodes.at(i).id;
			e.endNodeId = g.nodes.at(gvtarget).id;

			// to do: attrs

			g.edges.append(e);
		}
	}

	// done
    return status;
}


bool CFormatDOT::save(const QString& fileName, Graph& graph, QString* lastError) const
{


	return true;
}

