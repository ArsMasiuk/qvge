/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include <QMap>
#include <QByteArray>
#include <QVariant>

#include "qvge/IFileSerializer.h"

class CNode;
class CEdge;

class QTextStream;


class CFileSerializerDOT : public IFileSerializer
{
public:
	CFileSerializerDOT(bool writeBackground = true, bool writeAttrs = true) :
		m_writeBackground(writeBackground),
		m_writeAttrs(writeAttrs) 
	{}


	// reimp
	virtual QString description() const {
        return "DOT/GraphViz graph format";
	}

	virtual QString filters() const {
		return "DOT/GraphViz graphs (*.gv *.dot)";
	}

	virtual QString defaultFileExtension() const {
        return "gv";
	}

	virtual bool loadSupported() const {
        return false;
	}

    virtual bool load(const QString& /*fileName*/, CEditorScene& /*scene*/, QString* /*lastError = nullptr*/) const {
        return false;
    }

	virtual bool saveSupported() const {
		return true;
	}

	virtual bool save(const QString& fileName, CEditorScene& scene, QString* lastError = nullptr) const;

private:
	void doWriteNodeDefaults(QTextStream& ts, const CEditorScene& scene) const;
	void doWriteNode(QTextStream& ts, const CNode& node, const CEditorScene& scene) const;
	void doWriteNodeAttrs(QTextStream& ts, QMap<QByteArray, QVariant> nodeAttrs) const;

	void doWriteEdgeDefaults(QTextStream& ts, const CEditorScene& scene) const;
	void doWriteEdge(QTextStream& ts, const CEdge& edge, const CEditorScene& scene) const;
	void doWriteEdgeAttrs(QTextStream& ts, QMap<QByteArray, QVariant> edgeAttrs) const;

	bool m_writeBackground = true;
	bool m_writeAttrs = true;
};

