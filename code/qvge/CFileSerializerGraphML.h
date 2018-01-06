/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include "IFileSerializer.h"

#include <QtXml/QDomDocument>

class CNode;


class CFileSerializerGraphML : public IFileSerializer
{
public:

	// reimp
	virtual QString description() const {
		return "GraphML Format";
	}

	virtual QString filters() const {
		return "*.graphml";
	}

	virtual QString defaultFileExtension() const {
		return "graphml";
	}

	virtual bool loadSupported() const {
		return true;
	}

	virtual bool load(const QString& fileName, CEditorScene& scene) const;

	virtual bool saveSupported() const {
		return false;
	}

	virtual bool save(const QString& /*fileName*/, const CEditorScene& /*scene*/) const {
		return false;
	}

private:
	typedef QPair<QByteArray, QByteArray> ClassAttrId;
	typedef QMap<QByteArray, ClassAttrId> KeyAttrMap;

	bool readAttrKey(int index, const QDomNode &domNode, CEditorScene& scene, KeyAttrMap& cka) const;
	bool readNode(int index, const QDomNode &domNode, CEditorScene& scene, const KeyAttrMap& cka) const;
	bool readEdge(int index, const QDomNode &domNode, CEditorScene& scene, const KeyAttrMap& cka) const;

	mutable QMap<QString, CNode*> m_nodeMap;

	enum EdgeType {
		Directed,
		Undirected,
		Mutual
	};
	mutable QString m_edgeType;
};



