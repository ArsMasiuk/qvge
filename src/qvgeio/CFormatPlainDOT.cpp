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

bool CFormatPlainDOT::load(const QString& fileName, Graph& g, QString* lastError) const
{
	bool status = false;
	GraphInternal gi;

	QFile f(fileName);
	if (!f.open(QFile::ReadOnly))
		return false;

	QTextStream ts(&f);
	while (!ts.atEnd())
	{
		QString line = ts.readLine().simplified();
		if (line.isEmpty())
			continue;

		auto refs = line.splitRef(" ", QString::SkipEmptyParts);
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

    return status;
}


bool CFormatPlainDOT::save(const QString& fileName, Graph& graph, QString* lastError) const
{
	return false;
}


// privates

bool CFormatPlainDOT::parseGraph(QString& line, const QVector<QStringRef> &refs, GraphInternal &gi) const
{
	QTextStream ts(&line);
	ts >> gi.g_scale >> gi.g_x >> gi.g_y;

	return true;
}


bool CFormatPlainDOT::parseNode(QString& line, const QVector<QStringRef> &refs, GraphInternal &gi) const
{
	return true;
}


bool CFormatPlainDOT::parseEdge(QString& line, const QVector<QStringRef> &refs, GraphInternal &gi) const
{
	return true;
}

