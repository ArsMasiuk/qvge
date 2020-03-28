/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include "IFileSerializer.h"

#include <QtXml/QDomDocument>
#include <QByteArray>
#include <QMap>
#include <QVariant>

class CNode;


class CFileSerializerGEXF : public IFileSerializer 
{
public:

	// reimp
	virtual QString description() const {
		return "Graph Exchange XML Format";
	}

	virtual QString filters() const {
		return "*.gexf";
	}

	virtual QString defaultFileExtension() const {
		return "gexf";
	}

	virtual bool loadSupported() const {
		return true;
	}

	virtual bool load(const QString& fileName, CEditorScene& scene, QString* lastError = nullptr) const;

	virtual bool saveSupported() const {
		return true;
	}

	virtual bool save(const QString& fileName, CEditorScene& scene, QString* lastError = nullptr) const;

private:
    struct AttrInfo {
        QByteArray id;
        int variantType;
    };
    typedef QMap<QByteArray, AttrInfo> IdToAttrMap;
    mutable QMap<QByteArray, IdToAttrMap> m_classIdMap;

    bool readAttrs(int index, const QDomNode &domNode, CEditorScene& scene) const;
    bool readNode(int index, const QDomNode &domNode, const IdToAttrMap &idMap, CEditorScene& scene) const;
    bool readEdge(int index, const QDomNode &domNode, const IdToAttrMap &idMap, CEditorScene& scene) const;
	void writeClassAttrs(QTextStream &ts, const CEditorScene& scene, const QByteArray &classId) const;
	void writeNodes(QTextStream &ts, const CEditorScene& scene) const;
	void writeEdges(QTextStream &ts, const CEditorScene& scene) const;
	void writeAttValues(QTextStream &ts, const QMap<QByteArray, QVariant>& attvalues) const;

	mutable QMap<QString, CNode*> m_nodeMap;

	enum EdgeType {
		Directed,
		Undirected,
		Mutual
	};
	mutable QString m_edgeType;
};

