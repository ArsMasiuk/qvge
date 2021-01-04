/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2021 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CGraphBase.h"


void Graph::clear()
{
	attrs.clear();
	nodes.clear();
	edges.clear();

	nodeAttrs.clear();
	edgeAttrs.clear();
	graphAttrs.clear();
}


int Graph::findNodeIndex(const QString &id) const
{
	QByteArray bid = id.toUtf8();

	for (int i = 0; i < nodes.count(); ++i)
		if (nodes.at(i).id == bid)
			return i;

	return -1;
}
