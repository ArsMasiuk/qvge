/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <qvgeio/CGraphBase.h>


class CFormatPlainDOT
{
public:
	bool load(const QString& fileName, Graph& graph, QString* lastError = nullptr) const;
	bool save(const QString& fileName, Graph& graph, QString* lastError = nullptr) const;

private:
	struct GraphInternal
	{
		float g_scale = 1.0;
		float g_x = 1.0;
		float g_y = 1.0;
	};

	bool parseGraph(QString& line, const QVector<QStringRef>& refs, GraphInternal &gi) const;
	bool parseNode(QString& line, const QVector<QStringRef>& refs, GraphInternal &gi) const;
	bool parseEdge(QString& line, const QVector<QStringRef>& refs, GraphInternal &gi) const;
};



