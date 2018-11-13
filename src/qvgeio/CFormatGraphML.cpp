/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CFormatGraphML.h"

#include <QFile>
#include <QDebug>


// reimp

bool CFormatGraphML::load(const QString& fileName, Graph& graph) const
{
	// read file into document
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
		return false;

	QDomDocument doc("graphml");
	if (!doc.setContent(&file)) 
	{
		file.close();
		return false;
	}

	file.close();

	// try to parse
	graph.clear();

	QDomNodeList tree = doc.elementsByTagName("graph");
	if (tree.count())
	{
		m_edgeType = tree.at(0).toElement().attribute("edgedefault", "undirected");
	}

	KeyAttrMap cka;

	QDomNodeList keys = doc.elementsByTagName("key");
	for (int i = 0; i < keys.count(); ++i)
	{
		readAttrKey(i, keys.at(i), graph, cka);
	}

	QDomNodeList nodes = doc.elementsByTagName("node");
	for (int i = 0; i < nodes.count(); ++i)
	{
		readNode(i, nodes.at(i), graph, cka);
	}

	QDomNodeList edges = doc.elementsByTagName("edge");
	for (int i = 0; i < edges.count(); ++i)
	{
		readEdge(i, edges.at(i), graph, cka);
	}

	// done
	return true;
}


bool CFormatGraphML::readAttrKey(int /*index*/, const QDomNode& domNode, Graph& graph, KeyAttrMap& cka) const
{
	QDomElement elem = domNode.toElement();

	QString attrId = elem.attribute("id", "");
	QString classId = elem.attribute("for", "");
	QString nameId = elem.attribute("attr.name", "");
	QString valueType = elem.attribute("attr.type", "");

	if (attrId.isEmpty())
		return false;

	AttrInfo attr;
	attr.id = attrId.toLatin1();
	attr.name = nameId.isEmpty() ? attrId : nameId;

	if (valueType == "integer" || valueType == "long") {
		attr.valueType = QVariant::Int;
		attr.defaultValue.setValue(elem.text().toInt());
	}
	//else if (valueType == "long") {
	//	attr.valueType = QVariant::LongLong;
	//	attr.defaultValue.setValue(elem.text().toLongLong());
	//}
	else if (valueType == "double") {
		attr.valueType = QVariant::Double;
		attr.defaultValue.setValue(elem.text().toDouble());
	}
	else if (valueType == "float") {
		attr.valueType = QMetaType::Float;
		attr.defaultValue.setValue(elem.text().toFloat());
	}
	else if (valueType == "boolean") {
		attr.valueType = QMetaType::Bool;
		attr.defaultValue.setValue(!!elem.text().toInt());
	}
	else {
		attr.valueType = QMetaType::QString;
		attr.defaultValue.setValue(elem.text());
	}

	QByteArray attrClassId = classId.toLower().toLatin1();
	AttributeInfos& attrInfos =
		(attrClassId == "node") ? graph.nodeAttrs :
		(attrClassId == "edge") ? graph.edgeAttrs :
		graph.graphAttrs;

	attrInfos[attr.id] = attr;

	if (attrClassId == "graph") attrClassId = "";

	cka[attr.id] = ClassAttrId(attrClassId, attr.id);	// d0 = node:x

	return true;
}


bool CFormatGraphML::readNode(int /*index*/, const QDomNode &domNode, Graph& graph, const KeyAttrMap& cka) const
{
	QDomElement elem = domNode.toElement();

	Node node;

	// common attrs
	auto id = elem.attribute("id", "").toLocal8Bit();
	node.id = id;

	QDomNodeList data = elem.elementsByTagName("data");
	for (int i = 0; i < data.count(); ++i)
	{
		QDomNode dm = data.at(i);
		QDomElement de = dm.toElement();

		QString key = de.attribute("key", "");
		ClassAttrId classAttrId = cka[key.toLocal8Bit()];
		QByteArray attrId = classAttrId.second;

		if (!attrId.isEmpty())
		{
			node.attrs[attrId] = de.text();

			if (attrId == "tooltip")
				node.attrs["label"] = de.text();
			else
			if (attrId == "x_coordinate")
				node.attrs["x"] = de.text();
			else
			if (attrId == "y_coordinate")
				node.attrs["y"] = de.text();
		}
	}

	// ports
	QDomNodeList portNodes = elem.elementsByTagName("port");
	for (int i = 0; i < data.count(); ++i)
	{
		QDomNode dm = data.at(i);
		QDomElement de = dm.toElement();

		QString portName = de.attribute("name", "");
		if (portName.isEmpty())
			continue;

		NodePort port;
		port.name = portName;
		node.ports[portName] = port;
	}

	graph.nodes.append(node);

	return true;
}


bool CFormatGraphML::readEdge(int /*index*/, const QDomNode &domNode, Graph& graph, const KeyAttrMap& cka) const
{
	QDomElement elem = domNode.toElement();
	
	Edge edge;
	edge.startNodeId = elem.attribute("source", "").toLocal8Bit();
	edge.startPortId = elem.attribute("sourceport", "").toLocal8Bit();
	edge.endNodeId = elem.attribute("target", "").toLocal8Bit();
	edge.endPortId = elem.attribute("targetport", "").toLocal8Bit();

	// common attrs
	QString id = elem.attribute("id", "");
	edge.id = id.toLocal8Bit();

	QDomNodeList data = elem.elementsByTagName("data");
	for (int i = 0; i < data.count(); ++i)
	{
		QDomNode dm = data.at(i);
		QDomElement de = dm.toElement();

		QString key = de.attribute("key", "");
		QByteArray attrId = cka[key.toLatin1()].second;
		if (!attrId.isEmpty())
		{
			edge.attrs[attrId] = de.text();
		}
	}

	graph.edges.append(edge);

	return true;
}


