/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QtXml/QDomDocument>
#include <QMap>
#include <QByteArray>
#include <QVariant>
#include <QXmlStreamWriter>

#include <qvgeio/CGraphBase.h>


class CFormatGraphML
{
public:
	bool load(const QString& fileName, Graph& graph, QString* lastError = nullptr) const;
	bool save(const QString& fileName, Graph& graph, QString* lastError = nullptr) const;

private:
	typedef QPair<QByteArray, QByteArray> ClassAttrId;
	typedef QMap<QByteArray, ClassAttrId> KeyAttrMap;

	bool readAttrKey(int index, const QDomNode &domNode, Graph& graph, KeyAttrMap& cka) const;
	bool readNode(int index, const QDomNode &domNode, Graph& graph, const KeyAttrMap& cka) const;
	bool readEdge(int index, const QDomNode &domNode, Graph& graph, const KeyAttrMap& cka) const;

	void writeAttributes(QXmlStreamWriter &xsw, const AttributeInfos &attrs, const QByteArray &classId) const;

	enum EdgeType {
		Directed,
		Undirected,
		Mutual
	};
	mutable QString m_edgeType;
};



