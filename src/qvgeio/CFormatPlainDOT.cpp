/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CFormatPlainDOT.h"

#include <QFile>
#include <QDebug>
#include <QTextStream>
#include <QFont>


// helpers

static QString fromDotShape(const QString& shape)
{
	// rename to conform dot
	if (shape == "ellipse")		return "disc";
	if (shape == "rect")		return "square";
	if (shape == "invtriangle")	return "triangle2";

	// else take original
	return shape;
}


static QString fromDotStyle(const QString& style)
{
	// rename to conform dot
	QStringList sl = style.split(',');

	// else take original
	return style;
}


static QVector<QStringRef> tokenize(const QString& str)
{
	QVector<QStringRef> refs;

	// split on quotes keeping empties
	auto tempRefs = str.splitRef('"', QString::KeepEmptyParts);

	// split every even ref on spaces
	for (int i = 0; i < tempRefs.count(); i++)
	{
		if (i & 1)
		{
			refs << tempRefs.at(i);
		}
		else
		{
			auto tempSpaced = tempRefs.at(i).split(' ', QString::SkipEmptyParts);
			for (auto ref : tempSpaced)
				refs << ref;
		}
	}

	return refs;
}


class QStringRefsConstIterator
{
public:
	QStringRefsConstIterator(const QVector<QStringRef>& refs): m_refs(refs)
	{
		m_pos = (m_refs.size()) ? 0 : -1;
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
			s = m_refs.at(m_pos++).toString();
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
	const QVector<QStringRef>& m_refs;
	int m_pos = -1;
};


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
		QString line = ts.readLine();
		if (line.isEmpty())
			continue;

		//auto refs = line.splitRef(" ", QString::SkipEmptyParts);
		auto refs = tokenize(line);
		if (refs.first() == "stop")
			break;

		if (refs.first() == "graph")
		{
			parseGraph(line, refs, gi);
			continue;
		}

		if (refs.first() == "node")
		{
			parseNode(line, refs, gi);
			continue;
		}

		if (refs.first() == "edge")
		{
			parseEdge(line, refs, gi);
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

bool CFormatPlainDOT::parseGraph(QString& line, const QVector<QStringRef> &refs, GraphInternal &gi) const
{
	QStringRefsConstIterator rit(refs);
	rit.next();	// skip header
	rit.next(gi.g_scale);
	rit.next(gi.g_x);
	rit.next(gi.g_y);

	return true;
}


bool CFormatPlainDOT::parseNode(QString& line, const QVector<QStringRef> &refs, GraphInternal &gi) const
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

	node.attrs["label"] = label;
	node.attrs["shape"] = fromDotShape(shape);
	//node.attrs["style"] = fromDotStyle(style);	// to do
	node.attrs["color"] = fillcolor;
	node.attrs["stroke.color"] = color;

	gi.g->nodes << node;

	return true;
}


bool CFormatPlainDOT::parseEdge(QString& line, const QVector<QStringRef> &refs, GraphInternal &gi) const
{
	QTextStream ts(&line);
	QString dummy; ts >> dummy;

	Edge edge;
	ts >> edge.startNodeId >> edge.endNodeId;

	int jointCount = 0;
	float x, y;
	ts >> jointCount;
	if (jointCount > 0)
	{
		QString points;
		for (int i = 0; i < jointCount; ++i)
		{
			ts >> x >> y;
			x = x * 72.0 * gi.g_scale;
			y = y * 72.0 * gi.g_scale;
			points += QString("%1 %2 ").arg(x).arg(y);
		}
		//edge.attrs["points"] = points;
	}


	int parsedCount = 4 + jointCount * 2;
	int parsedRest = refs.size() - parsedCount;
	if (parsedRest > 2)
	{
		QString label;
		ts >> label >> x >> y;
		edge.attrs["label"] = label;
		edge.attrs["label.x"] = x * 72.0 * gi.g_scale;
		edge.attrs["label.y"] = y * 72.0 * gi.g_scale;
	}

	edge.attrs["style"] = refs.at(refs.size() - 2).toString();
	edge.attrs["color"] = refs.at(refs.size() - 1).toString();

	gi.g->edges << edge;

	return true;
}

