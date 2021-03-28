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
#include <QFont>

#ifdef USE_BOOST
    #include <boost/graph/adjacency_list.hpp>
    #include <boost/graph/graphviz.hpp>
#endif

struct DotVertex
{
	std::string id;
	// shape
	std::string pos;
	std::string fillcolor;
	std::string shape;
	float width = .0;
	float height = .0;
	// stroke
	std::string color;
	float penwidth = .0;
	std::string style;

	// label
	std::string label;
	std::string xlabel;
	std::string fontcolor;
	std::string fontname;
	float fontsize = .0;
};

struct DotEdge
{
	std::string id;
	std::string dir;
	
	std::string color;
	std::string style;
	float penwidth = .0;
	float weight = .0;

	// label
	std::string label;
	std::string xlabel;
	std::string fontcolor;
	std::string fontname;
	float fontsize = .0;
};

#ifdef USE_BOOST
    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, DotVertex, DotEdge> graph_t;
#endif


// helpers

static QString fromDotShape(const std::string& shape)
{
	// rename to conform dot
	if (shape == "ellipse")		return "disc";
	if (shape == "rect" || shape == "box" ) return "square";
	if (shape == "invtriangle")	return "triangle2";

	// else take original
	return QString::fromStdString(shape);
}


template<class DotLabel>
static void readLabel(const DotLabel &v, GraphAttributes &attrs)
{
	if (v.label.size())
		attrs["label"] = QString::fromStdString(v.label);
	else
	if (v.xlabel.size())
		attrs["label"] = QString::fromStdString(v.xlabel);

	if (v.fontcolor.size())
		attrs["label.color"] = QColor(QString::fromStdString(v.fontcolor));

	if (v.fontname.size() || v.fontsize > .0)
	{
		QFont f;
		if (v.fontname.size())
		{
			QString fontstring = QString::fromStdString(v.fontname).toLower();
			
			if (fontstring.contains("bold"))
			{
				fontstring = fontstring.remove("bold");
				f.setBold(true);
			}
			
			if (fontstring.contains("italic"))
			{
				fontstring = fontstring.remove("italic");
				f.setItalic(true);
			}

			f.setFamily(fontstring);
		}

		if (v.fontsize > .0)
			f.setPointSizeF(v.fontsize);

		attrs["label.font"] = f;
	}
}


// reimp

bool CFormatDOT::load(const QString& fileName, Graph& g, QString* lastError) const
{
#ifdef USE_BOOST
	graph_t graphviz;
	boost::dynamic_properties dp(boost::ignore_other_properties);

	dp.property("node_id", boost::get(&DotVertex::id, graphviz));
	dp.property("color", boost::get(&DotVertex::color, graphviz));
	dp.property("fillcolor", boost::get(&DotVertex::fillcolor, graphviz));
	dp.property("width", boost::get(&DotVertex::width, graphviz));
	dp.property("height", boost::get(&DotVertex::height, graphviz));
	dp.property("pos", boost::get(&DotVertex::pos, graphviz));
	dp.property("shape", boost::get(&DotVertex::shape, graphviz));
	dp.property("penwidth", boost::get(&DotVertex::penwidth, graphviz));
	dp.property("style", boost::get(&DotVertex::style, graphviz));
	dp.property("fontcolor", boost::get(&DotVertex::fontcolor, graphviz));
	dp.property("fontname", boost::get(&DotVertex::fontname, graphviz));
	dp.property("fontsize", boost::get(&DotVertex::fontsize, graphviz));
	dp.property("label", boost::get(&DotVertex::label, graphviz));
	dp.property("xlabel", boost::get(&DotVertex::xlabel, graphviz));

	dp.property("id", boost::get(&DotEdge::id, graphviz));
	dp.property("color", boost::get(&DotEdge::color, graphviz));
	dp.property("penwidth", boost::get(&DotEdge::penwidth, graphviz));
	dp.property("weight", boost::get(&DotEdge::weight, graphviz));
	dp.property("dir", boost::get(&DotEdge::dir, graphviz));
	dp.property("style", boost::get(&DotEdge::style, graphviz));
	dp.property("fontcolor", boost::get(&DotEdge::fontcolor, graphviz));
	dp.property("fontname", boost::get(&DotEdge::fontname, graphviz));
	dp.property("fontsize", boost::get(&DotEdge::fontsize, graphviz));
	dp.property("label", boost::get(&DotEdge::label, graphviz));
	dp.property("xlabel", boost::get(&DotEdge::xlabel, graphviz));


	std::ifstream dot(fileName.toStdString());
	bool status = false;

	try
	{
		status = boost::read_graphviz(dot, graphviz, dp);

		if (!status)
		{
			*lastError = ("Failed reading DOT format");
			return false;
		}
	}

	catch (boost::bad_graphviz_syntax e)
	{
		*lastError = QString::fromStdString(e.what());
		return false;
	}

	catch (...)
	{
		*lastError = ("BGL: unknown exception");
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
		
		if (v.width > .0)
			n.attrs["width"] = v.width * 72.0;

		if (v.height > .0)
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

		if (v.color.size())
			n.attrs["stroke.color"] = QColor(QString::fromStdString(v.color));

		if (v.style.size())
			n.attrs["stroke.style"] = QString::fromStdString(v.style);

		if (v.penwidth > .0)
			n.attrs["stroke.size"] = v.penwidth;

		readLabel(v, n.attrs);

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

			Q_ASSERT(i >= 0 && i < nodesCount);
			Q_ASSERT(gvtarget >= 0 && gvtarget < nodesCount);
			
			Edge e;
			e.startNodeId = g.nodes.at(i).id;
			e.endNodeId = g.nodes.at(gvtarget).id;

			if (edge.weight > .0)
				e.attrs["weight"] = edge.weight;
			else if (edge.penwidth > .0)
				e.attrs["weight"] = edge.penwidth;

			if (edge.dir == "both")
				e.attrs["direction"] = "mutual";
			else if (edge.dir == "none")
				e.attrs["direction"] = "undirected";

			if (edge.style.size())
				e.attrs["style"] = QString::fromStdString(edge.style);

			readLabel(edge, e.attrs);

			g.edges.append(e);
		}
	}

	// done
    return status;

#else
	return false;
#endif
}


bool CFormatDOT::save(const QString& fileName, Graph& graph, QString* lastError) const
{
	return false;
}

