/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2021 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CFormatPlainDOT.h"

#include <QFile>
#include <QDebug>
#include <QTextStream>
#include <QFont>


// helpers

static QString fromDotNodeShape(const QString& shape)
{
	// rename to conform dot
	if (shape == "ellipse")		return "disc";
	if (shape == "rect" || shape == "box" ) return "square";
	if (shape == "invtriangle")	return "triangle2";

	// else take original
	return shape;
}


static void fromDotNodeStyle(const QString& style, GraphAttributes& nodeAttr)
{
	if (style.contains("dashed"))
		nodeAttr["stroke.style"] = "dashed";
	else
	if (style.contains("dotted"))
		nodeAttr["stroke.style"] = "dotted";

	if (style.contains("invis"))
		nodeAttr["stroke.size"] = 0;
	else
	if (style.contains("solid"))
		nodeAttr["stroke.size"] = 1;
	else
	if (style.contains("bold"))
		nodeAttr["stroke.size"] = 3;
}


class QStringRefsConstIterator
{
public:
	QStringRefsConstIterator(const QStringList& refs): m_refs(refs)
	{
		m_pos = (m_refs.size()) ? 0 : -1;
	}

	int pos() const
	{
		return m_pos;
	}

	int restCount() const
	{
		return canNext() ? m_refs.count() - m_pos : 0;
	}

	bool canNext() const
	{
		return (m_pos >= 0 && m_pos < m_refs.count());
	}

	bool next()
	{
		if (canNext())
		{
			m_pos++;
			return true;
		}

		return false;
	}

	bool next(float &f)
	{
		if (canNext())
		{
			bool ok = false;
			float tf = m_refs.at(m_pos++).toFloat(&ok);
			if (ok)
			{
				f = tf;
				return true;
			}
		}

		return false;
	}

	bool next(int &i)
	{
		if (canNext())
		{
			bool ok = false;
			int ti = m_refs.at(m_pos++).toInt(&ok);
			if (ok)
			{
				i = ti;
				return true;
			}
		}

		return false;
	}

	bool next(QString &s)
	{
		if (canNext())
		{
			s = m_refs.at(m_pos++);
			return true;
		}

		return false;
	}

	bool next(QByteArray &s)
	{
		if (canNext())
		{
			s = m_refs.at(m_pos++).toUtf8();
			return true;
		}

		return false;
	}

private:
	const QStringList& m_refs;
	int m_pos = -1;
};


// parsing plain dot

static QStringList parseLine(QTextStream &ts)
{
	QStringList tokens;

	// normal mode
	while (!ts.atEnd())
	{
		QString line = ts.readLine().trimmed();
		if (line.isEmpty())
			continue;

		// add EOL
		line += '\0';

		int i = 0;

	_loop:
		// skip to next token
		while (line[i].isSpace())
			i++;
		if (line[i] == '\0')
			break;

		// check for "
		if (line[i] == '"')
		{
			i++;
			QString token;
			while (line[i] != '"' && line[i] != '\0')
				token += line[i++];
			tokens << token;

			goto _loop;
		}

		// check for < + eol
		if (line[i] == '<' && line[i+1] == '\0')
		{
			QString token;
			while (!ts.atEnd())
			{
				QString l = ts.readLine();
				QString lt = l.trimmed() + '\0';
				if (lt[0] == '>' && (lt[1].isSpace() || lt[1] == '\0'))
				{
					tokens << token;
					line = lt;
					i = 1;
					goto _loop;
				}
				else
				{
					token += l + "\n";
				}
			}
		}
		
		// read normal tokens
		QString token;
		while (line[i] != '\0' && !line[i].isSpace())
			token += line[i++];

		tokens << token;
		goto _loop;
	}

	return tokens;
}


static void splitEdgePortIds(QByteArray& nodeId, QByteArray& portId)
{
	int index = nodeId.indexOf(':');
	if (index <= 0) 
	{
		portId.clear();
		return;
	}

	portId = nodeId.mid(index + 1);
	nodeId = nodeId.left(index);
}


// reimp

bool CFormatPlainDOT::load(const QString& fileName, Graph& g, QString* lastError) const
{
	GraphInternal gi;
	gi.g = &g;

	QFile f(fileName);
	if (!f.open(QFile::ReadOnly))
	{
		if (lastError)
			*lastError = QObject::tr("Cannot open file");

		return false;
	}

	QTextStream ts(&f);

	while (!ts.atEnd())
	{
		auto refs = parseLine(ts);
		if (refs.first() == "stop")
			break;

		if (refs.first() == "graph")
		{
			parseGraph(refs, gi);
			continue;
		}

		if (refs.first() == "node")
		{
			parseNode(refs, gi);
			continue;
		}

		if (refs.first() == "edge")
		{
			parseEdge(refs, gi);
			continue;
		}
	}

	// done
	f.close();

    return true;
}


bool CFormatPlainDOT::save(const QString& fileName, Graph& g, QString* lastError) const
{
	return false;
}


// privates

bool CFormatPlainDOT::parseGraph(const QStringList &refs, GraphInternal &gi) const
{
	QStringRefsConstIterator rit(refs);
	rit.next();	// skip header
	rit.next(gi.g_scale);
	rit.next(gi.g_x);
	rit.next(gi.g_y);

	return true;
}


bool CFormatPlainDOT::parseNode(const QStringList &refs, GraphInternal &gi) const
{
	QStringRefsConstIterator rit(refs);
	rit.next();	// skip header

	Node node;
	rit.next(node.id);

	QString label, style, shape, color, fillcolor;
	float x, y, width, height;

	rit.next(x);
	rit.next(y);
	rit.next(width);
	rit.next(height);
	
	rit.next(label);
	rit.next(style);
	rit.next(shape);
	rit.next(color);
	rit.next(fillcolor);

	node.attrs["x"] = x * 72.0 * gi.g_scale;
	node.attrs["y"] = y * 72.0 * gi.g_scale;
	node.attrs["width"] = width * 72.0 * gi.g_scale;
	node.attrs["height"] = height * 72.0 * gi.g_scale;

	label = label.replace("\\n", "\n");
	node.attrs["label"] = label;
	node.attrs["shape"] = fromDotNodeShape(shape);
	fromDotNodeStyle(style, node.attrs);
	node.attrs["color"] = fillcolor;
	node.attrs["stroke.color"] = color;

	gi.g->nodes << node;

	return true;
}


bool CFormatPlainDOT::parseEdge(const QStringList &refs, GraphInternal &gi) const
{
	QStringRefsConstIterator rit(refs);
	rit.next();	// skip header

	Edge edge;
	rit.next(edge.startNodeId);
	rit.next(edge.endNodeId);

	int jointCount = 0;
	float x, y;
	rit.next(jointCount);
	if (jointCount > 0)
	{
		QString points;
		for (int i = 0; i < jointCount; ++i)
		{
			rit.next(x);
			rit.next(y);
			x = x * 72.0 * gi.g_scale;
			y = y * 72.0 * gi.g_scale;
			points += QString("%1 %2 ").arg(x).arg(y);
		}
		//edge.attrs["points"] = points;
	}


	if (rit.restCount() > 2)
	{
		QString label;
		rit.next(label);
		label = label.replace("\\n", "\n");

		rit.next(x);
		rit.next(y);
		edge.attrs["label"] = label;
		edge.attrs["label.x"] = x * 72.0 * gi.g_scale;
		edge.attrs["label.y"] = y * 72.0 * gi.g_scale;

		edge.id = label.toUtf8();
	}

	if (rit.canNext())
	{
		QString style; rit.next(style);	
		edge.attrs["style"] = style;
	}

	if (rit.canNext())
	{
		QString color; rit.next(color);
		edge.attrs["color"] = color;
	}

	 
	// check id
	if (edge.id.isEmpty())
	{
		edge.id = edge.startNodeId + "-" + edge.endNodeId;
	}


	// split ports if any
	splitEdgePortIds(edge.startNodeId, edge.startPortId);
	splitEdgePortIds(edge.endNodeId, edge.endPortId);


	gi.g->edges << edge;

	return true;
}

