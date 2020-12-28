/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once

#include "IFileSerializer.h"


class CFileSerializerPlainDOT : public IFileSerializer
{
public:

	// reimp
	virtual QString description() const {
		return "Plain Text DOT Format";
	}

	virtual QString filters() const {
		return "Plain Text DOT/GraphViz Graphs (*.plain *.txt)";
	}

	virtual QString defaultFileExtension() const {
		return "txt";
	}

	virtual bool loadSupported() const {
		return true;
	}

	virtual bool load(const QString& fileName, CEditorScene& scene, QString* lastError = nullptr) const;

	virtual bool saveSupported() const {
		return false;
	}

	virtual bool save(const QString& fileName, CEditorScene& scene, QString* lastError = nullptr) const;
};



