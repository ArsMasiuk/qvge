/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CFormatGraphML.h"

#include <QFile>
#include <QDebug>


// reimp

bool CFormatGraphML::save(const QString& fileName, Graph& graph, QString* lastError) const
{
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly))
	{
		if (lastError)
			*lastError = QString("%1: File cannot be opened for writing").arg(fileName);

		return false;
	}

	QXmlStreamWriter xsw(&file);
	xsw.setCodec("UTF-8");
	xsw.setAutoFormatting(true);
	xsw.setAutoFormattingIndent(4);

	xsw.writeStartDocument();
	xsw.writeStartElement("graphml");
	xsw.writeAttribute("xmlns", "http://graphml.graphdrawing.org/xmlns");
	xsw.writeAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
	xsw.writeAttribute("xsi:schemaLocation", "http://graphml.graphdrawing.org/xmlns http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd");

	// attrs
	writeAttributes(xsw, graph.graphAttrs, "graph");
	writeAttributes(xsw, graph.edgeAttrs, "edge");
	writeAttributes(xsw, graph.nodeAttrs, "node");

	xsw.writeStartElement("graph");

	// graph attrs
	if (graph.edgeAttrs.contains("direction"))
	{
		xsw.writeAttribute("edgedefault", graph.edgeAttrs["direction"].defaultValue.toString());
	}

	// nodes
	writeNodes(xsw, graph);

	// edges
	writeEdges(xsw, graph);


	xsw.writeEndElement();	// graph

	xsw.writeEndElement();	// graphml
	xsw.writeEndDocument();

	return true;
}


void CFormatGraphML::writeNodes(QXmlStreamWriter &xsw, const Graph& graph) const
{
	for (const auto& node : graph.nodes)
	{
		xsw.writeStartElement("node");
		
		// topology
		xsw.writeAttribute("id", node.id);

		for (const auto& port : node.ports)
		{
			xsw.writeStartElement("port");
			
			xsw.writeAttribute("name", port.name);
			xsw.writeAttribute("color", port.color.name());
			xsw.writeAttribute("anchor", QString::number(port.anchor));
			xsw.writeAttribute("x", QString::number(port.x));
			xsw.writeAttribute("y", QString::number(port.y));

			xsw.writeEndElement();
		}

		// attributes
		for (auto it = node.attrs.constBegin(); it != node.attrs.constEnd(); it++)
		{
			//if (it.key() == "size")
			//{
			//	QSizeF sf = it.value().toSizeF();
			//	writeAttribute(xsw, "width", sf.width());
			//	writeAttribute(xsw, "height", sf.height());
			//	continue;
			//}

			//if (it.key() == "pos")
			//{
			//	QPointF sf = it.value().toPointF();
			//	writeAttribute(xsw, "x", sf.x());
			//	writeAttribute(xsw, "y", sf.y());
			//	continue;
			//}

			writeAttribute(xsw, it.key(), it.value());
		}

		xsw.writeEndElement();
	}
}


void CFormatGraphML::writeEdges(QXmlStreamWriter &xsw, const Graph& graph) const
{
	for (const auto& edge: graph.edges)
	{
		xsw.writeStartElement("edge");

		// topology
		xsw.writeAttribute("id", edge.id);
		xsw.writeAttribute("source", edge.startNodeId);
		xsw.writeAttribute("target", edge.endNodeId);
		if (edge.startPortId.size())
			xsw.writeAttribute("sourceport", edge.startPortId);
		if (edge.endPortId.size())
			xsw.writeAttribute("endport", edge.endPortId);

		// attributes
		for (auto it = edge.attrs.constBegin(); it != edge.attrs.constEnd(); it++)
		{
			writeAttribute(xsw, it.key(), it.value());
		}

		xsw.writeEndElement();
	}
}


void CFormatGraphML::writeAttributes(QXmlStreamWriter &xsw, const AttributeInfos &attrs, const QByteArray &classId) const
{
	for (auto& attr : attrs)
	{
		xsw.writeStartElement("key");

		xsw.writeAttribute("id", attr.id);
		//xsw.writeAttribute("attr.name", attr.id);		// GraphML::name = qvge::id
		//xsw.writeAttribute("attr.title", attr.name);	// GraphML::title = qvge::name
		xsw.writeAttribute("attr.name", attr.name);

		if (classId.size())
			xsw.writeAttribute("for", classId);

		switch (attr.valueType)
		{
		case QVariant::Int:			xsw.writeAttribute("attr.type", "integer"); break;
		case QVariant::LongLong:	xsw.writeAttribute("attr.type", "long"); break;
		case QVariant::Double:		xsw.writeAttribute("attr.type", "double"); break;
		case QMetaType::Float:		xsw.writeAttribute("attr.type", "float"); break;
		case QMetaType::Bool:		xsw.writeAttribute("attr.type", "boolen"); break;
		default:					xsw.writeAttribute("attr.type", "string"); break;
		}

		if (attr.defaultValue.isValid())
		{
			xsw.writeTextElement("default", 
				attr.valueType == QMetaType::QStringList ? attr.defaultValue.toStringList().join('|') :
				attr.defaultValue.toString()
			);
		}

		xsw.writeEndElement();	// key
	}
}


void CFormatGraphML::writeAttribute(QXmlStreamWriter &xsw, const QString &keyId, const QVariant &value) const
{
	xsw.writeStartElement("data");

	xsw.writeAttribute("key", keyId);
	xsw.writeCharacters(value.toString());

	xsw.writeEndElement();
}


bool CFormatGraphML::load(const QString& fileName, Graph& graph, QString* lastError) const
{
	// read file into document
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
		return false;

	QString errorString;
	int errorLine, errorColumn;

	QDomDocument doc("graphml");
	if (!doc.setContent(&file, false, &errorString, &errorLine, &errorColumn))
	{
		file.close();

		if (lastError)
			*lastError = QObject::tr("%1\nline: %2, column: %3").arg(errorString).arg(errorLine).arg(errorColumn);

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

	ClassKeyAttrMap cka;

	QDomNodeList keys = doc.elementsByTagName("key");
	for (int i = 0; i < keys.count(); ++i)
	{
		readAttrKey(i, keys.at(i), graph, cka);
	}

	QDomNodeList nodes = doc.elementsByTagName("node");
	for (int i = 0; i < nodes.count(); ++i)
	{
		readNode(i, nodes.at(i), graph, cka["node"]);
	}

	QDomNodeList edges = doc.elementsByTagName("edge");
	for (int i = 0; i < edges.count(); ++i)
	{
		readEdge(i, edges.at(i), graph, cka["edge"]);
	}

	// done
	return true;
}


bool CFormatGraphML::readAttrKey(int /*index*/, const QDomNode& domNode, Graph& graph, ClassKeyAttrMap& cka) const
{
	QDomElement elem = domNode.toElement();

	QString keyId = elem.attribute("id", "");
	//QString attrId = elem.attribute("attr.name", "");
	//QString nameId = elem.attribute("attr.title", "");
	QString attrId = keyId;
	QString nameId = elem.attribute("attr.name", "");

	QString classId = elem.attribute("for", "");
	QString valueType = elem.attribute("attr.type", "");

	if (keyId.isEmpty() || attrId.isEmpty())
		return false;

	AttrInfo attr;
	attr.id = attrId.toLatin1();
	attr.name = nameId.isEmpty() ? attrId : nameId;

	if (valueType == "integer"/* || valueType == "long"*/) {
		attr.valueType = QVariant::Int;
		attr.defaultValue.setValue(elem.text().toInt());
	}
	else if (valueType == "long") {
		attr.valueType = QVariant::LongLong;
		attr.defaultValue.setValue(elem.text().toLongLong());
	}
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

	cka[attrClassId][keyId.toLatin1()] = attr.id;

	return true;
}


bool CFormatGraphML::readNode(int /*index*/, const QDomNode &domNode, Graph& graph, const KeyAttrMap& nodeKeys) const
{
	QDomElement elem = domNode.toElement();

	Node node;

	// common attrs
	auto id = elem.attribute("id", "").toLatin1();
	node.id = id;

	QDomNodeList data = elem.elementsByTagName("data");
	for (int i = 0; i < data.count(); ++i)
	{
		QDomNode dm = data.at(i);
		QDomElement de = dm.toElement();

		QByteArray keyId = de.attribute("key", "").toLatin1();
		QByteArray attrId = nodeKeys.contains(keyId) ? 
			nodeKeys[keyId] : 
			keyId;				// warning: no key registered

		if (attrId.isEmpty())	// error should be here
			continue;

		QVariant value = de.text();

		node.attrs[attrId] = value;
	}

	// ports
	QDomNodeList portNodes = elem.elementsByTagName("port");
	for (int i = 0; i < portNodes.count(); ++i)
	{
		QDomNode dm = portNodes.at(i);
		QDomElement de = dm.toElement();

		QString portName = de.attribute("name", "");
		if (portName.isEmpty())
			continue;

		NodePort port;
		port.name = portName;
		port.color = de.attribute("color");
		port.anchor = de.attribute("anchor").toInt();
		port.x = de.attribute("x").toFloat();
		port.y = de.attribute("y").toFloat();
		node.ports[portName] = port;
	}

	graph.nodes.append(node);

	return true;
}


bool CFormatGraphML::readEdge(int /*index*/, const QDomNode &domNode, Graph& graph, const KeyAttrMap& edgeKeys) const
{
	QDomElement elem = domNode.toElement();
	
	Edge edge;
	edge.startNodeId = elem.attribute("source", "").toLatin1();
	edge.startPortId = elem.attribute("sourceport", "").toLatin1();
	edge.endNodeId = elem.attribute("target", "").toLatin1();
	edge.endPortId = elem.attribute("targetport", "").toLatin1();

	// common attrs
	QString id = elem.attribute("id", "");
	edge.id = id.toLatin1();

	QDomNodeList data = elem.elementsByTagName("data");
	for (int i = 0; i < data.count(); ++i)
	{
		QDomNode dm = data.at(i);
		QDomElement de = dm.toElement();

		QByteArray keyId = de.attribute("key", "").toLatin1();
		QByteArray attrId = edgeKeys.contains(keyId) ?
			edgeKeys[keyId] :
			keyId;				// warning: no key registered

		if (attrId.isEmpty())	// error should be here
			continue;

		edge.attrs[attrId] = de.text();
	}

	graph.edges.append(edge);

	return true;
}


