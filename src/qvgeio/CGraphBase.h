/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2021 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QMap>
#include <QByteArray>
#include <QVariant>
#include <QList>
#include <QColor>
#include <QPointF>


typedef QMap<QByteArray, QVariant> GraphAttributes;


enum AttrFlags
{
	ATTR_NONE = 0,
	ATTR_VIRTUAL = 1,		// read only, not to be stored & read
	ATTR_FIXED = 2,			// non-user defined
	ATTR_NODEFAULT = 4,		// has no default value
	ATTR_MAPPED = 8			// internal: mapped to some system value, i.e. coordinate or size
};


struct AttrInfo
{
	QByteArray id;
	QString name;
	int valueType = 0;
	QVariant defaultValue;
};

typedef QMap<QByteArray, AttrInfo> AttributeInfos;


struct NodePort
{
	QString name;
	float x = 0, y = 0;
	QColor color;
	int anchor = 0;
};

typedef QMap<QString, NodePort> NodePorts;


struct Node
{
	// generic data
	QByteArray id;
	GraphAttributes attrs;

	// only for ports
	NodePorts ports;
};


struct Edge
{
	// generic data
	QByteArray id;
	GraphAttributes attrs;

	QByteArray startNodeId;
	QByteArray endNodeId;

	// only for ports
	QByteArray startPortId;
	QByteArray endPortId;
};


struct Graph
{
	QList<Node> nodes;
	QList<Edge> edges;
	GraphAttributes attrs;

	AttributeInfos nodeAttrs;
	AttributeInfos edgeAttrs;
	AttributeInfos graphAttrs;

	// methods
	void clear();
	int findNodeIndex(const QString &id) const;
};

