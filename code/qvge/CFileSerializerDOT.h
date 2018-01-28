/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2018 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include "qvge/IFileSerializer.h"


class CNode;
class CConnection;

class QTextStream;


class CFileSerializerDOT : public IFileSerializer
{
public:
	// reimp
	virtual QString description() const {
        return "DOT graph format";
	}

	virtual QString filters() const {
        return "*.dot;*.gv";
	}

	virtual QString defaultFileExtension() const {
        return "dot";
	}

	virtual bool loadSupported() const {
        return false;
	}

    virtual bool load(const QString& fileName, CEditorScene& scene) const {
        return false;
    }

	virtual bool saveSupported() const {
		return true;
	}

	virtual bool save(const QString& fileName, const CEditorScene& scene) const;

private:
	void doWriteNode(QTextStream& ts, const CNode& node, const CEditorScene& scene) const;
	void doWriteEdge(QTextStream& ts, const CConnection& edge, const CEditorScene& scene) const;
};

