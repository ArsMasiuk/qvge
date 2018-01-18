/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CFileSerializerGEXF.h"
#include "CNode.h"
#include "CDirectConnection.h"

#include <QFile>
#include <QDebug>

// reimp

bool CFileSerializerGEXF::load(const QString& fileName, CEditorScene& scene) const
{
	// read file into document
	QDomDocument doc("gexf");
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
		return false;
	if (!doc.setContent(&file)) {
		file.close();
		return false;
	}
	file.close();

	// try to parse
	scene.reset();

    m_classIdMap.clear();
    m_nodeMap.clear();

	QDomNodeList graph = doc.elementsByTagName("graph");
	if (graph.count())
	{
		m_edgeType = graph.at(0).toElement().attribute("defaultedgetype", "undirected");
	}

    QDomNodeList attrs = doc.elementsByTagName("attributes");
    for (int i = 0; i < attrs.count(); ++i)
    {
        readAttrs(i, attrs.at(i), scene);
    }

    auto nodeIds = m_classIdMap["node"];
	QDomNodeList nodes = doc.elementsByTagName("node");
	for (int i = 0; i < nodes.count(); ++i)
	{
        readNode(i, nodes.at(i), nodeIds, scene);
	}

    auto edgeIds = m_classIdMap["edge"];
	QDomNodeList edges = doc.elementsByTagName("edge");
	for (int i = 0; i < edges.count(); ++i)
	{
        readEdge(i, edges.at(i), edgeIds, scene);
	}

    // update scene rect
    scene.setSceneRect(scene.itemsBoundingRect());

	return true;
}


bool CFileSerializerGEXF::readAttrs(int /*index*/, const QDomNode &domNode, CEditorScene &scene) const
{
    auto elem = domNode.toElement();
    QByteArray classId = elem.attribute("class", "").toLatin1();

    auto attrs = elem.elementsByTagName("attribute");
    for (int i = 0; i < attrs.count(); ++i)
    {
        auto attrElem = attrs.at(i).toElement();
        QByteArray id = attrElem.attribute("id", "").toLatin1();
        if (id.isEmpty())
            continue;
        QByteArray attrId = attrElem.attribute("title", "").toLatin1();
        if (attrId.isEmpty())
            attrId = id;
        QByteArray type = attrElem.attribute("type", "").toLatin1();

        AttrInfo attrInfo = {attrId, 0};

        if (type == "integer" || type == "long")
        {
            attrInfo.variantType = QVariant::Int;
            QString def = attrElem.attribute("default", "0");
            QVariant v = Utils::textToVariant(def, attrInfo.variantType);
            scene.setClassAttribute(classId, attrId, v);
        }
        else if (type == "double" || type == "float")
        {
            attrInfo.variantType = QVariant::Double;
            QString def = attrElem.attribute("default", "0.0");
            QVariant v = Utils::textToVariant(def, attrInfo.variantType);
            scene.setClassAttribute(classId, attrId, v);
        }
        else if (type == "boolean")
        {
            attrInfo.variantType = QVariant::Bool;
            QString def = attrElem.attribute("default", "true");
            QVariant v = Utils::textToVariant(def, attrInfo.variantType);
            scene.setClassAttribute(classId, attrId, v);
        }
        else    // string
        {
            attrInfo.variantType = QVariant::String;
            QString def = attrElem.attribute("default", "");
            QVariant v = Utils::textToVariant(def, attrInfo.variantType);
            scene.setClassAttribute(classId, attrId, v);
        }

        m_classIdMap[classId][id] = attrInfo;
    }

    return true;
}


bool CFileSerializerGEXF::readNode(int index, const QDomNode &domNode, const IdToAttrMap &idMap, CEditorScene &scene) const
{
	QDomElement elem = domNode.toElement();

	CNode* node = scene.createItemOfType<CNode>();
	if (!node)
		return false;

	// common attrs
	QString id = elem.attribute("id", "");
	node->setAttribute("id", id);

	QString label = elem.attribute("label", "");
	node->setAttribute("label", label);

    // viz: attrs (v1.2), ns0: attrs (v1.1)
    QDomNodeList viz_pos = elem.elementsByTagName("viz:position");  // v1.2
    if (viz_pos.isEmpty())
        viz_pos = elem.elementsByTagName("ns0:position");   // v1.1

	if (viz_pos.size()) {
		QDomElement viz_elem = viz_pos.at(0).toElement();
		float x = viz_elem.attribute("x", "0").toFloat();
		float y = viz_elem.attribute("y", "0").toFloat();
		float z = viz_elem.attribute("z", QString(index)).toFloat();
		node->setPos(x, y);
		node->setZValue(z);
	}

    QDomNodeList viz_color = elem.elementsByTagName("viz:color");   // v1.2
    if (viz_color.isEmpty())
        viz_color = elem.elementsByTagName("ns0:color");    // v1.1

	if (viz_color.size()) {
		QDomElement viz_elem = viz_color.at(0).toElement();
		int r = viz_elem.attribute("r", "0").toInt();
		int g = viz_elem.attribute("g", "0").toInt();
		int b = viz_elem.attribute("b", "0").toInt();
		QColor color(r, g, b);
		node->setAttribute("color", color);
	}

    QDomNodeList viz_size = elem.elementsByTagName("viz:size");     // v1.2
    if (viz_size.isEmpty())
        viz_size = elem.elementsByTagName("ns0:size");      // v1.1

	if (viz_size.size()) {
		QDomElement viz_elem = viz_size.at(0).toElement();
		float v = viz_elem.attribute("value", "5").toFloat();
		node->resize(v);
	}

    // attrs
    QDomNodeList attrs = elem.elementsByTagName("attvalue");
    for (int i = 0; i < attrs.count(); ++i)
    {
        auto attrElem = attrs.at(i).toElement();
        QByteArray attrId = attrElem.attribute("id", "").toLatin1();     // v1.2
        if (attrId.isEmpty())
            attrId = attrElem.attribute("for", "").toLatin1();           // v1.1
        if (attrId.isEmpty())
            continue;   // error: no id
        if (!idMap.contains(attrId))
            continue;      // error: not valid id

        QVariant value = Utils::textToVariant(attrElem.attribute("value"), idMap[attrId].variantType);
        node->setAttribute(idMap[attrId].id, value);
    }


	scene.addItem(node);

	m_nodeMap[id] = node;

	return true;
}


bool CFileSerializerGEXF::readEdge(int /*index*/, const QDomNode &domNode, const IdToAttrMap &idMap, CEditorScene& scene) const
{
	QDomElement elem = domNode.toElement();

	auto* link = scene.createItemOfType<CDirectConnection>();
	if (!link)
		return false;

	// common attrs
	QString id = elem.attribute("id", "");
	link->setAttribute("id", id);

	QString label = elem.attribute("label", "");
	link->setAttribute("label", id);

	QString source = elem.attribute("source", "");
	QString target = elem.attribute("target", "");

	CNode* start = m_nodeMap[source];
	CNode* last = m_nodeMap[target];
	link->setFirstNode(start);
	link->setLastNode(last);

	// line
	float weight = elem.attribute("weight", "-1").toFloat();
	if (weight >= 0)
		link->setAttribute("weight", weight);

	// direction
	QString edgeType = elem.attribute("defaultedgetype", "");
	if (edgeType.isEmpty())
		edgeType = m_edgeType;

	link->setAttribute("direction", edgeType);

    // attrs
    QDomNodeList attrs = elem.elementsByTagName("attvalue");
    for (int i = 0; i < attrs.count(); ++i)
    {
        auto attrElem = attrs.at(i).toElement();
        QByteArray attrId = attrElem.attribute("id", "").toLatin1();     // v1.2
        if (attrId.isEmpty())
            attrId = attrElem.attribute("for", "").toLatin1();           // v1.1
        if (attrId.isEmpty())
            continue;   // error: no id
        if (!idMap.contains(attrId))
            continue;      // error: not valid id

        QVariant value = Utils::textToVariant(attrElem.attribute("value"), idMap[attrId].variantType);
        link->setAttribute(idMap[attrId].id, value);
    }

	scene.addItem(link);

	return true;
}

