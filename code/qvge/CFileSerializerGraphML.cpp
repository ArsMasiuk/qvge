/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#include "CFileSerializerGraphML.h"
#include "CAttribute.h"
#include "CNode.h"
#include "CDirectConnection.h"

#include <QFile>
#include <QDebug>


// reimp

bool CFileSerializerGraphML::load(const QString& fileName, CEditorScene& scene) const
{
	// read file into document
	QDomDocument doc("graphml");
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

	QDomNodeList graph = doc.elementsByTagName("graph");
	if (graph.count())
	{
		m_edgeType = graph.at(0).toElement().attribute("edgedefault", "undirected");
	}

	KeyAttrMap cka;

	QDomNodeList keys = doc.elementsByTagName("key");
	for (int i = 0; i < keys.count(); ++i)
	{
		readAttrKey(i, keys.at(i), scene, cka);
	}

	QDomNodeList nodes = doc.elementsByTagName("node");
	for (int i = 0; i < nodes.count(); ++i)
	{
		readNode(i, nodes.at(i), scene, cka);
	}

	QDomNodeList edges = doc.elementsByTagName("edge");
	for (int i = 0; i < edges.count(); ++i)
	{
		readEdge(i, edges.at(i), scene, cka);
	}

    // update scene rect
    scene.setSceneRect(scene.itemsBoundingRect());

    scene.addUndoState();

	// done
	return true;
}


bool CFileSerializerGraphML::readAttrKey(int /*index*/, const QDomNode & domNode, CEditorScene & scene, KeyAttrMap& cka) const
{
	QDomElement elem = domNode.toElement();

	QString key = elem.attribute("id", "");
	QString classId = elem.attribute("for", "");
	QString valueId = elem.attribute("attr.name", "");
	QString valueType = elem.attribute("attr.type", "");

	if (key.isEmpty() || valueId.isEmpty())
		return false;

	CAttribute attr;
	attr.id = valueId.toLower().toLatin1();
	QByteArray attrclassId = classId.toLower().toLatin1();
	if (attrclassId.isEmpty())
		attrclassId = "item";
	attr.name = valueId; // ?

	if (valueType == "integer")			
		attr.defaultValue.setValue(elem.text().toInt());
	else if (valueType == "double")		
		attr.defaultValue.setValue(elem.text().toDouble());
	else if (valueType == "float")		
		attr.defaultValue.setValue(elem.text().toFloat());
	else
		attr.defaultValue.setValue(elem.text());

	scene.setClassAttribute(attrclassId, attr);

	cka[key.toLatin1()] = ClassAttrId(attrclassId, attr.id);	// d0 = node:x

	return true;
}


bool CFileSerializerGraphML::readNode(int /*index*/, const QDomNode &domNode, CEditorScene& scene, const KeyAttrMap& cka) const
{
	QDomElement elem = domNode.toElement();

	CNode* node = scene.createItemOfType<CNode>();
	if (!node)
		return false;

	// common attrs
	QString id = elem.attribute("id", "");
	node->setAttribute("id", id);

	QDomNodeList data = elem.elementsByTagName("data");
	for (int i = 0; i < data.count(); ++i)
	{
		QDomNode dm = data.at(i);
		QDomElement de = dm.toElement();

		QString key = de.attribute("key", "");
		ClassAttrId classAttrId = cka[key.toLatin1()];
		QByteArray attrId = classAttrId.second;

		if (!attrId.isEmpty())
		{
			node->setAttribute(attrId, de.text());

			if (attrId == "tooltip")
				node->setAttribute("label", de.text());
			else
			if (attrId == "x_coordinate")
				node->setAttribute("x", de.text());
			else
			if (attrId == "y_coordinate")
				node->setAttribute("y", de.text());	
		}
	}

	scene.addItem(node);

	m_nodeMap[id] = node;

	return true;
}


bool CFileSerializerGraphML::readEdge(int /*index*/, const QDomNode &domNode, CEditorScene& scene, const KeyAttrMap& cka) const
{
	QDomElement elem = domNode.toElement();
	
	QString source = elem.attribute("source", "");
	QString target = elem.attribute("target", "");

	CNode* start = m_nodeMap[source];
	CNode* last = m_nodeMap[target];
	if (!start || !last)
		return false;

	CConnection* link = scene.createItemOfType<CDirectConnection>();
	if (!link)
		return false;

	link->setFirstNode(start);
	link->setLastNode(last);

	// common attrs
	QString id = elem.attribute("id", "");
	link->setAttribute("id", id);

	QDomNodeList data = elem.elementsByTagName("data");
	for (int i = 0; i < data.count(); ++i)
	{
		QDomNode dm = data.at(i);
		QDomElement de = dm.toElement();

		QString key = de.attribute("key", "");
		QByteArray attrId = cka[key.toLatin1()].second;
		if (!attrId.isEmpty())
		{
			link->setAttribute(attrId, de.text());
		}
	}

	scene.addItem(link);

	return true;
}


