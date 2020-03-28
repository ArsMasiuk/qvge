/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

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

