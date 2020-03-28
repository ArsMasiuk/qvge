/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CFileSerializerGEXF.h"
#include "CNode.h"
#include "CDirectEdge.h"

#include <QFile>
#include <QDate>
#include <QDebug>
#include <QApplication>
#include <QMessageBox>


// reimp

bool CFileSerializerGEXF::load(const QString& fileName, CEditorScene& scene, QString* lastError) const
{
	// read file into document
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
		return false;
	
	QString errorString;
	int errorLine, errorColumn;

	QDomDocument doc("gexf");
	if (!doc.setContent(&file, false, &errorString, &errorLine, &errorColumn))
	{
		file.close();

		if (lastError)
			*lastError = QObject::tr("%1\nline: %2, column: %3").arg(errorString).arg(errorLine).arg(errorColumn);
			
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

    scene.addUndoState();

	return true;
}


bool CFileSerializerGEXF::readAttrs(int /*index*/, const QDomNode &domNode, CEditorScene &scene) const
{
    QDomElement elem = domNode.toElement();
    QByteArray classId = elem.attribute("class", "").toLatin1();

	AttributesMap existingAttrs = scene.getClassAttributes(classId, true);

    auto attrs = elem.elementsByTagName("attribute");
    for (int i = 0; i < attrs.count(); ++i)
    {
		QDomElement attrElem = attrs.at(i).toElement();
        QByteArray id = attrElem.attribute("id", "").toLatin1();
        if (id.isEmpty())
            continue;
        QByteArray attrId = attrElem.attribute("title", "").toLatin1();
        if (attrId.isEmpty())
            attrId = id;
        QByteArray type = attrElem.attribute("type", "").toLatin1();

        AttrInfo attrInfo = {attrId, 0};

		QString def;
		auto defs = attrElem.elementsByTagName("default");
		if (defs.size())
			def = defs.at(0).toElement().text();

        if (type == "integer" || type == "long")
        {
            attrInfo.variantType = QVariant::Int;
        }
        else if (type == "double" || type == "float")
        {
            attrInfo.variantType = QVariant::Double;
        }
        else if (type == "boolean")
        {
            attrInfo.variantType = QVariant::Bool;
        }
		else if (type == "liststring")
		{
			attrInfo.variantType = QVariant::StringList;
		}
        else    // string
        {
            attrInfo.variantType = QVariant::String;
        }


		CAttribute attr = existingAttrs[attrId];
		if (attr.id.isEmpty())
		{	// no such attr
			attr.id = attrId;
			attr.classId = classId;
			attr.valueType = attrInfo.variantType;
		}
		else // exist
			attrInfo.variantType = attr.valueType;


		if (def.size())
		{
			// visibility attr
			if (attrId == "_vis_")
			{
				auto visList = def.splitRef('|');
				for (auto& id : visList)
					scene.setClassAttributeVisible(classId, id.toLatin1());
				continue;
			}

			// stringlists
			if (attrInfo.variantType == QVariant::StringList)
			{
				attr.defaultValue = def.split('|');
				continue;
			}

			// other attrs
			QVariant v = CUtils::textToVariant(def, attrInfo.variantType);

			if (attrId == "size" && classId == "node")
			{
				v = QSizeF(v.toDouble(), v.toDouble());
			}

			attr.defaultValue = v;
		}

		scene.setClassAttribute(classId, attr);

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

	if (viz_size.size()) 
	{
		QSizeF sz = node->getSize();
		QDomElement viz_elem = viz_size.at(0).toElement();
		
		if (viz_elem.hasAttribute("value")) 
		{
			float v = viz_elem.attribute("value", "5").toFloat();
			sz.setWidth(v);
			sz.setHeight(v);
		}
		if (viz_elem.hasAttribute("width"))
			sz.setWidth(viz_elem.attribute("width").toFloat());
		if (viz_elem.hasAttribute("height"))
			sz.setHeight(viz_elem.attribute("height").toFloat());

		node->setAttribute("size", sz);
	}

	// shape
	QDomNodeList viz_shape = elem.elementsByTagName("viz:shape");   // v1.2
	if (viz_shape.isEmpty())
		viz_shape = elem.elementsByTagName("ns0:shape");    // v1.1

	if (viz_shape.size()) {
		QDomElement viz_elem = viz_shape.at(0).toElement();
		QString v = viz_elem.attribute("value", "disc");
		node->setAttribute("shape", v);
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

        QVariant value = CUtils::textToVariant(attrElem.attribute("value"), idMap[attrId].variantType);
        node->setAttribute(idMap[attrId].id, value);
    }


	scene.addItem(node);

	m_nodeMap[id] = node;

	//node->onItemRestored();

	return true;
}


bool CFileSerializerGEXF::readEdge(int /*index*/, const QDomNode &domNode, const IdToAttrMap &idMap, CEditorScene& scene) const
{
	QDomElement elem = domNode.toElement();

	auto* link = scene.createItemOfType<CDirectEdge>();
	if (!link)
		return false;

	// common attrs
	QString id = elem.attribute("id", "");
	link->setAttribute("id", id);

	QString label = elem.attribute("label", "");
	link->setAttribute("label", label);

	QString source = elem.attribute("source", "");
	QString target = elem.attribute("target", "");

	CNode* start = m_nodeMap[source];
	CNode* last = m_nodeMap[target];
	link->setFirstNode(start);
	link->setLastNode(last);

	// line
	double weight = elem.attribute("weight", "-1").toDouble();
	if (weight >= 0)
		link->setAttribute("weight", weight);

	// direction
	QString edgeType = elem.attribute("edgetype", "");
	if (edgeType.isEmpty())
		edgeType = m_edgeType;

	link->setAttribute("direction", edgeType);

	// color
	QDomNodeList viz_color = elem.elementsByTagName("viz:color");   // v1.2
	if (viz_color.isEmpty())
		viz_color = elem.elementsByTagName("ns0:color");    // v1.1

	if (viz_color.size()) {
		QDomElement viz_elem = viz_color.at(0).toElement();
		int r = viz_elem.attribute("r", "0").toInt();
		int g = viz_elem.attribute("g", "0").toInt();
		int b = viz_elem.attribute("b", "0").toInt();
		QColor color(r, g, b);
		link->setAttribute("color", color);
	}

	// thickness
	QDomNodeList viz_thickness = elem.elementsByTagName("viz:thickness");   // v1.2
	if (viz_thickness.isEmpty())
		viz_thickness = elem.elementsByTagName("ns0:thickness");    // v1.1

	if (viz_thickness.size()) {
		QDomElement viz_elem = viz_thickness.at(0).toElement();
		float v = viz_elem.attribute("value", "1").toFloat();
		link->setAttribute("thickness", v);
	}

	// shape
	QDomNodeList viz_shape = elem.elementsByTagName("viz:shape");   // v1.2
	if (viz_shape.isEmpty())
		viz_shape = elem.elementsByTagName("ns0:shape");    // v1.1

	if (viz_shape.size()) {
		QDomElement viz_elem = viz_shape.at(0).toElement();
		QString v = viz_elem.attribute("value", "solid");
		link->setAttribute("style", v);
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

        QVariant value = CUtils::textToVariant(attrElem.attribute("value"), idMap[attrId].variantType);
        link->setAttribute(idMap[attrId].id, value);
    }

	scene.addItem(link);

	//link->onItemRestored();

	return true;
}


bool CFileSerializerGEXF::save(const QString& fileName, CEditorScene& scene, QString* /*lastError*/) const
{
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly))
		return false;

	QTextStream ts(&file);
	ts.setCodec("UTF-8");

	// header
	ts << 
		"<?xml version = \"1.0\" encoding = \"UTF-8\"?>\n"
		"<gexf xmlns = \"http://www.gexf.net/1.2draft\" version = \"1.2\"\n"
		"    xmlns:viz = \"http://www.gexf.net/1.2draft/viz\"\n"
		"    xmlns:xsi = \"http://www.w3.org/2001/XMLSchema-instance\"\n"
		"    xsi:schemaLocation = \"http://www.gexf.net/1.2draft http://www.gexf.net/1.2draft/gexf.xsd\">\n";

	ts << 
		"    <meta lastmodifieddate = \"" << QDate::currentDate().toString(Qt::ISODate) << "\">\n"
		"        <creator>" << QApplication::applicationDisplayName() << "</creator>\n"
		"        <description>" << scene.getClassAttribute("", "comment", false).defaultValue.toString().toHtmlEscaped() << "</description>\n"
		"    </meta>\n";

	// graph
	QString edgetype = scene.getClassAttribute("edge", "direction", false).defaultValue.toString();
	//QString edgetype = scene.getClassAttribute<CEdge>("direction", false).defaultValue.toString();
	ts << "    <graph mode=\"static\" defaultedgetype=\"" << edgetype << "\">\n";

	writeClassAttrs(ts, scene, "");

	// node attrs
	writeClassAttrs(ts, scene, "node");

	// edge attrs
	writeClassAttrs(ts, scene, "edge");

	// nodes
	writeNodes(ts, scene);

	// edges
	writeEdges(ts, scene);

	// footer
	ts << "    </graph>\n";
	ts << "</gexf>\n";

	return true;
}


static QString typeToString(int valueType)
{
	switch (valueType)
	{
	case QMetaType::Bool:		
		return "boolean";

	case QMetaType::Int:
	case QMetaType::UInt:
		return "integer";

	case QMetaType::Long:
	case QMetaType::ULong:
		return "long";

	case QMetaType::Double:
		return "double";

	case QMetaType::Float:
		return "float";

	case QMetaType::QStringList:
		return "liststring";

	// url

	default:
		return "string";
	}
}


void CFileSerializerGEXF::writeClassAttrs(QTextStream &ts, const CEditorScene& scene, const QByteArray &classId) const
{
	auto attrs = scene.getClassAttributes(classId, false);

	// add local attributes if any
	if (classId.size())
	{
		auto items = (classId == "node") ? 
			scene.getItems<CItem, CNode>(): 
			scene.getItems<CItem, CEdge>();

		for (auto item : items)
		{
			auto itemAttrs = item->getLocalAttributes();
			for (auto it = itemAttrs.constBegin(); it != itemAttrs.constEnd(); ++it)
			{
				auto id = it.key();
				if (!attrs.contains(id))
					attrs[id] = CAttribute(id);
			}
		}
	}

	// add visible state if any
	QSet<QByteArray> visSet = scene.getVisibleClassAttributes(classId, false);
	if (!visSet.isEmpty())
	{
		QStringList visList;
		for (auto& id : visSet)
			visList << id;
		CAttribute visAttr("_vis_", "Visibility", visList);
		attrs["_vis_"] = visAttr;
	}

	if (attrs.isEmpty())
		return;

	ts << "    <attributes class=\"" << classId << "\" mode=\"static\">\n";

	for (auto it = attrs.constBegin(); it != attrs.constEnd(); ++it)
	{
		const auto &attr = it.value();
		if (attr.flags & ATTR_VIRTUAL)
			continue;

		// size
		if (it.key() == "size")
		{
			ts << "        <attribute id=\"" << "size" << "\" title=\"" << "size" << "\" type=\"" << "float" << "\">\n";

			if (attr.defaultValue.canConvert(QMetaType::QSizeF))
			{
				QSizeF size = attr.defaultValue.toSizeF();
				ts << "            <default>" << qMax(size.width(), size.height()) << "</default>\n";
			}
			else
				ts << "            <default>" << attr.defaultValue.toFloat() << "</default>\n";

			ts << "        </attribute>\n";
			continue;
		}

		// others (id = title)
		ts << "        <attribute id=\"" << it.key() << "\" title=\"" << it.key() << "\" type=\"" << typeToString(attr.valueType) << "\">\n";
		
		if (!(attr.flags & ATTR_NODEFAULT) && attr.defaultValue.isValid())
		{
			ts << "            <default>";

			if (attr.valueType == QMetaType::QStringList)
				ts << attr.defaultValue.toStringList().join('|');
			else
				ts << attr.defaultValue.toString();

			ts << "</default>\n";
		}

		ts << "        </attribute>\n";
	}

	ts << "    </attributes>\n";
}


void CFileSerializerGEXF::writeNodes(QTextStream &ts, const CEditorScene& scene) const
{
	ts << "    <nodes>\n";

	auto nodes = scene.getItems<CNode>();
	for (const auto &node : nodes)
	{
		QMap<QByteArray, QVariant> nodeAttrs = node->getLocalAttributes();

		ts << "        <node id=\"" << node->getId() << "\" label=\"" << nodeAttrs.take("label").toString().toHtmlEscaped() << "\">\n";
		ts << "            <viz:position x=\"" << node->pos().x() << "\" y=\"" << node->pos().y() << "\"/>\n";

		if (nodeAttrs.contains("size"))
		{
			QVariant sizeV = nodeAttrs.take("size");
			if (sizeV.canConvert(QMetaType::QSizeF))
			{
				QSizeF size = sizeV.toSizeF();
				if (size.width() == size.height())
					ts << "            <viz:size value=\"" << size.width() << "\"/>\n";
				else
					ts << "            <viz:size value=\"" << size.width() << "\" width=\"" << size.width() << "\" height=\"" << size.height() << "\"/>\n";		// non-standard extension
			}
			else
				ts << "            <viz:size value=\"" << sizeV.toFloat() << "\"/>\n";
		}

		if (nodeAttrs.contains("color"))
		{
			QColor c = nodeAttrs.take("color").value<QColor>();
			ts << "            <viz:color r=\"" << c.red() << "\" g=\"" << c.green() << "\" b=\"" << c.blue() << "\"/>\n";
		}

		if (nodeAttrs.contains("shape"))
		{
			QString shape = nodeAttrs.take("shape").toString();
			ts << "            <viz:shape value=\"" << shape << "\"/>\n";
		}

		writeAttValues(ts, nodeAttrs);

		ts << "        </node>\n";
	}

	ts << "    </nodes>\n";
}


void CFileSerializerGEXF::writeEdges(QTextStream &ts, const CEditorScene& scene) const
{
	ts << "    <edges>\n";

	auto edges = scene.getItems<CEdge>();
	for (auto edge : edges)
	{
		QMap<QByteArray, QVariant> edgeAttrs = edge->getLocalAttributes();

		ts << "        <edge id=\"" << edge->getId() << "\" label=\"" << edgeAttrs.take("label").toString().toHtmlEscaped()
			<< "\" source=\"" << edge->firstNode()->getId() << "\" target=\"" << edge->lastNode()->getId();
		
		QString edgetype = edgeAttrs.take("direction").toString();
		if (edgetype.size())
		{
			ts << "\" edgetype=\"" << edgetype;
		}
		
		ts << "\">\n";

		if (edgeAttrs.contains("thickness"))
		{
			ts << "            <viz:thickness value=\"" << edgeAttrs.take("thickness").toFloat() << "\"/>\n";
		}

		if (edgeAttrs.contains("color"))
		{
			QColor c = edgeAttrs.take("color").value<QColor>();
			ts << "            <viz:color r=\"" << c.red() << "\" g=\"" << c.green() << "\" b=\"" << c.blue() << "\"/>\n";
		}

		if (edgeAttrs.contains("style"))
		{
			QString shape = edgeAttrs.take("style").toString();
			ts << "            <viz:shape value=\"" << shape << "\"/>\n";
		}

		writeAttValues(ts, edgeAttrs);

		ts << "        </edge>\n";
	}

	ts << "    </edges>\n";
}


void CFileSerializerGEXF::writeAttValues(QTextStream &ts, const QMap<QByteArray, QVariant>& attvalues) const
{
	if (attvalues.size())
	{
		ts << "            <attvalues>\n";

		for (auto it = attvalues.constBegin(); it != attvalues.constEnd(); ++it)
		{
			ts << "                <attvalue for=\"" << it.key() << "\" value=\"" << it.value().toString().toHtmlEscaped() << "\"/>\n";
		}

		ts << "            </attvalues>\n";
	}
}
